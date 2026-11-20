#pragma once
// Henry: networked play for the co-op game (ADR NET-1, NET-3, NET-4 in docs/DECISIONS.md).
// The host runs the level and plays slot 0 (fire); the joiner sends input for slot 1 (water) and shows snapshots.
#include "elemental_coop/game.hpp"
#include "engine/networking.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>
namespace elemental_coop {
inline constexpr std::uint8_t protocol_version = 2;
inline constexpr std::size_t max_gates = 8;    // campaign levels have at most 2
inline constexpr std::size_t max_enemies = 16; // campaign levels have at most 5
enum class ControlKind : std::uint8_t { pause = 1, resume, restart, quit };

struct Hello { bool operator==(const Hello&) const = default; };
struct Welcome { bool operator==(const Welcome&) const = default; };   // joiner is always slot 1
struct Reject { bool operator==(const Reject&) const = default; };     // protocol version mismatch
struct NetInput {
    std::uint32_t tick{0};
    float move_x{0};                 // [-1, 1]
    std::uint8_t jump_count{0};      // +1 per press; a lost packet loses no jump
    std::uint8_t interact_count{0};  // +1 per lever press, same rule
    bool operator==(const NetInput&) const = default;
};
struct NetPlayer {
    float x{0}, y{0}, vx{0}, vy{0};
    bool grounded{false}, alive{false}, at_exit{false}, facing_left{false};
    bool operator==(const NetPlayer&) const = default;
};
struct NetGate {
    bool lever_on{false}, plate_occupied{false}, open{false};
    bool operator==(const NetGate&) const = default;
};
struct NetEnemy {
    float x{0}, y{0};
    bool facing_left{false};
    float animation_time{0};
    bool operator==(const NetEnemy&) const = default;
};
// Gates and enemies are listed in level-file order, so both machines agree on which is which.
struct NetSnapshot {
    std::uint32_t tick{0};
    RoundState round{RoundState::playing};
    std::uint8_t level{0}; // campaign index; the joiner loads the same level
    std::array<NetPlayer, player_count> players{};
    std::vector<NetGate> gates;     // at most max_gates
    std::vector<NetEnemy> enemies;  // at most max_enemies
    bool operator==(const NetSnapshot&) const = default;
};
struct Control { ControlKind kind{ControlKind::pause}; bool operator==(const Control&) const = default; };
using Message = std::variant<Hello, Welcome, Reject, NetInput, NetSnapshot, Control>;

std::vector<std::byte> encode(const Message& message);
// nullopt for anything malformed: wrong version, unknown type, wrong size, too many gates/enemies,
// out-of-range or non-finite values.
std::optional<Message> decode(std::span<const std::byte> bytes);

// What the host needs from the level it runs. GameNetLevel adapts Game; the UI adapts its DemoLevel.
class NetLevel {
public:
    virtual ~NetLevel() = default;
    virtual void tick(const PlayerInputs& inputs, float dt) = 0;
    virtual void interact() = 0; // a lever press; levels without levers do nothing
    virtual void restart() = 0;
    virtual NetSnapshot snapshot(std::uint32_t tick) const = 0;
};
NetSnapshot snapshot_of(const Game& game, std::uint32_t tick);
class GameNetLevel final : public NetLevel {
public:
    explicit GameNetLevel(Game& game) : game_(game) {}
    void tick(const PlayerInputs& inputs, float dt) override { game_.tick(inputs, dt); }
    void interact() override {}
    void restart() override { game_.restart(); }
    NetSnapshot snapshot(std::uint32_t tick) const override { return snapshot_of(game_, tick); }
private:
    Game& game_;
};

// Runs on the hosting machine. Call update() once per frame with this machine's input (slot 0).
class CoopHost {
public:
    CoopHost(NetLevel& level, engine::net::Session& session) : level_(level), session_(session) {}
    void update(const engine::InputFrame& local, float dt);
    void request(ControlKind kind); // local pause/resume/restart/quit, mirrored to the joiner
    bool client_ready() const { return ready_; }
    bool paused() const { return paused_; }
    std::uint32_t tick() const { return tick_; }
private:
    void handle(std::span<const std::byte> bytes);
    NetLevel& level_;
    engine::net::Session& session_;
    bool ready_{false};
    bool paused_{false};
    std::uint32_t tick_{0};
    std::uint32_t last_input_tick_{0};
    float remote_move_{0};
    std::uint8_t remote_jumps_{0};
    std::uint8_t remote_interacts_{0};
    bool remote_jump_{false};
    bool remote_interact_{false};
    bool quit_pending_{false};
};

// Runs on the joining machine. Show latest(); the UI copies it into its own level each frame.
class CoopClient {
public:
    explicit CoopClient(engine::net::Session& session) : session_(session) {}
    void update(const engine::InputFrame& local, bool interact = false); // sends this frame's input (slot 1)
    void request(ControlKind kind);
    bool ready() const { return ready_; }
    bool paused() const { return paused_; }
    bool rejected() const { return rejected_; }
    const std::optional<NetSnapshot>& latest() const { return latest_; }
private:
    engine::net::Session& session_;
    bool ready_{false};
    bool paused_{false};
    bool rejected_{false};
    std::uint32_t tick_{0};
    std::uint8_t jumps_{0};
    std::uint8_t interacts_{0};
    std::optional<NetSnapshot> latest_;
};
}
