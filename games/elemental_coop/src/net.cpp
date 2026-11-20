#include "elemental_coop/net.hpp"
#include <type_traits>
namespace elemental_coop {
namespace {
using engine::net::ByteReader;
using engine::net::ByteWriter;
using engine::net::Channel;
enum class Type : std::uint8_t { hello = 1, welcome, reject, input, snapshot, control };

void send(engine::net::Session& session, Channel channel, const Message& message) {
    session.send(channel, encode(message)); // false only when not connected; the next update notices
}
std::uint8_t flags_of(const NetPlayer& p) {
    return static_cast<std::uint8_t>((p.grounded ? 1 : 0) | (p.alive ? 2 : 0) | (p.at_exit ? 4 : 0));
}
std::optional<NetPlayer> read_player(ByteReader& in) {
    const auto x = in.f32(), y = in.f32(), vx = in.f32(), vy = in.f32();
    const auto flags = in.u8();
    if (!x || !y || !vx || !vy || !flags || *flags > 7) return std::nullopt;
    return NetPlayer{*x, *y, *vx, *vy, (*flags & 1) != 0, (*flags & 2) != 0, (*flags & 4) != 0};
}
}

std::vector<std::byte> encode(const Message& message) {
    ByteWriter out;
    out.u8(protocol_version);
    std::visit([&](const auto& m) {
        using T = std::decay_t<decltype(m)>;
        if constexpr (std::is_same_v<T, Hello>) out.u8(static_cast<std::uint8_t>(Type::hello));
        else if constexpr (std::is_same_v<T, Welcome>) out.u8(static_cast<std::uint8_t>(Type::welcome)).u8(1);
        else if constexpr (std::is_same_v<T, Reject>) out.u8(static_cast<std::uint8_t>(Type::reject)).u8(1);
        else if constexpr (std::is_same_v<T, NetInput>)
            out.u8(static_cast<std::uint8_t>(Type::input)).u32(m.tick).f32(m.move_x).u8(m.jump_count);
        else if constexpr (std::is_same_v<T, NetSnapshot>) {
            out.u8(static_cast<std::uint8_t>(Type::snapshot)).u32(m.tick).u8(static_cast<std::uint8_t>(m.round));
            for (const auto& p : m.players) out.f32(p.x).f32(p.y).f32(p.vx).f32(p.vy).u8(flags_of(p));
        } else out.u8(static_cast<std::uint8_t>(Type::control)).u8(static_cast<std::uint8_t>(m.kind));
    }, message);
    return out.bytes();
}

std::optional<Message> decode(std::span<const std::byte> bytes) {
    ByteReader in(bytes);
    const auto version = in.u8();
    const auto type = in.u8();
    if (!version || !type || *version != protocol_version) return std::nullopt;
    std::optional<Message> message;
    switch (static_cast<Type>(*type)) {
    case Type::hello: message = Hello{}; break;
    case Type::welcome: if (in.u8() == 1) message = Welcome{}; break;
    case Type::reject: if (in.u8() == 1) message = Reject{}; break;
    case Type::input: {
        const auto tick = in.u32();
        const auto move = in.f32();
        const auto jumps = in.u8();
        if (tick && move && jumps && *move >= -1 && *move <= 1) message = NetInput{*tick, *move, *jumps};
        break;
    }
    case Type::snapshot: {
        const auto tick = in.u32();
        const auto round = in.u8();
        const auto fire = read_player(in);
        const auto water = read_player(in);
        if (tick && round && *round <= static_cast<std::uint8_t>(RoundState::quit) && fire && water)
            message = NetSnapshot{*tick, static_cast<RoundState>(*round), {*fire, *water}};
        break;
    }
    case Type::control: {
        const auto kind = in.u8();
        if (kind && *kind >= 1 && *kind <= static_cast<std::uint8_t>(ControlKind::quit))
            message = Control{static_cast<ControlKind>(*kind)};
        break;
    }
    default: break;
    }
    if (!message || !in.done()) return std::nullopt;
    return message;
}

NetSnapshot snapshot_of(const Game& game, std::uint32_t tick) {
    NetSnapshot snapshot{tick, game.state(), {}};
    for (std::size_t i = 0; i < player_count; ++i) {
        const auto& p = game.player(i);
        const auto position = game.world().find(p.entity)->position;
        snapshot.players[i] = {position.x, position.y, p.body.velocity.x, p.body.velocity.y, p.grounded, p.alive, p.at_exit};
    }
    return snapshot;
}

void CoopHost::update(const engine::InputFrame& local, float dt) {
    for (const auto& event : session_.poll()) {
        if (event.kind == engine::net::Event::Kind::disconnected) ready_ = false;
        if (event.kind == engine::net::Event::Kind::message) handle(event.payload);
    }
    if (!ready_) return;
    if (!paused_) {
        PlayerInputs inputs{local, engine::InputFrame{remote_move_, remote_jump_, quit_pending_}};
        game_.tick(inputs, dt);
        remote_jump_ = false;
        quit_pending_ = false;
        ++tick_;
    }
    send(session_, Channel::unreliable, snapshot_of(game_, tick_)); // also while paused, so the joiner stays current
}

void CoopHost::handle(std::span<const std::byte> bytes) {
    const auto message = decode(bytes);
    if (!message) {
        // A different protocol version can only be told apart before decoding; tell that joiner why.
        if (!ready_ && !bytes.empty() && bytes[0] != std::byte{protocol_version}) send(session_, Channel::reliable, Reject{});
        return; // malformed: ignore, never change game state
    }
    if (std::holds_alternative<Hello>(*message)) {
        ready_ = true;
        last_input_tick_ = 0;
        remote_move_ = 0;
        remote_jumps_ = 0;
        remote_jump_ = false;
        quit_pending_ = false; // a quit from before this joiner arrived must not end their round
        send(session_, Channel::reliable, Welcome{});
        if (paused_) send(session_, Channel::reliable, Control{ControlKind::pause}); // new joiner learns the pause
        return;
    }
    if (!ready_) return;
    if (const auto* input = std::get_if<NetInput>(&*message); input && input->tick > last_input_tick_) {
        last_input_tick_ = input->tick;
        remote_move_ = input->move_x;
        if (input->jump_count != remote_jumps_) remote_jump_ = true;
        remote_jumps_ = input->jump_count;
    } else if (const auto* control = std::get_if<Control>(&*message)) {
        request(control->kind);
    }
}

void CoopHost::request(ControlKind kind) {
    if (kind == ControlKind::pause) paused_ = true;
    if (kind == ControlKind::resume) paused_ = false;
    if (kind == ControlKind::restart) game_.restart();
    if (kind == ControlKind::quit) quit_pending_ = true;
    if (ready_) send(session_, Channel::reliable, Control{kind});
}

void CoopClient::update(const engine::InputFrame& local) {
    for (const auto& event : session_.poll()) {
        if (event.kind == engine::net::Event::Kind::connected) send(session_, Channel::reliable, Hello{});
        if (event.kind == engine::net::Event::Kind::disconnected) ready_ = false;
        if (event.kind != engine::net::Event::Kind::message) continue;
        const auto message = decode(event.payload);
        if (!message) continue;
        if (std::holds_alternative<Welcome>(*message)) { // new session: the host's ticks and jump count restart
            ready_ = true;
            paused_ = false;
            tick_ = 0;
            jumps_ = 0;
            latest_.reset();
        }
        if (std::holds_alternative<Reject>(*message)) {
            rejected_ = true;
            session_.close();
            return;
        }
        if (const auto* snapshot = std::get_if<NetSnapshot>(&*message); snapshot && (!latest_ || snapshot->tick >= latest_->tick))
            latest_ = *snapshot; // equal tick: a paused host resends the same state
        if (const auto* control = std::get_if<Control>(&*message)) {
            if (control->kind == ControlKind::pause) paused_ = true;
            if (control->kind == ControlKind::resume) paused_ = false;
        }
    }
    if (!ready_) return;
    if (local.quit_requested) request(ControlKind::quit);
    if (local.primary_pressed) ++jumps_;
    send(session_, Channel::unreliable, NetInput{++tick_, local.move_x, jumps_});
}

void CoopClient::request(ControlKind kind) {
    if (ready_) send(session_, Channel::reliable, Control{kind});
}
}
