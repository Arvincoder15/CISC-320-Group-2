#pragma once
// Henry: networked play for the co-op game (ADR NET-1, NET-3, NET-4 in docs/DECISIONS.md).
// The host runs Game and plays slot 0 (fire); the joiner sends input for slot 1 (water) and draws snapshots.
#include "elemental_coop/game.hpp"
#include "engine/networking.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>
namespace elemental_coop {
inline constexpr std::uint8_t protocol_version = 1;
enum class ControlKind : std::uint8_t { pause = 1, resume, restart, quit };

struct Hello { bool operator==(const Hello&) const = default; };
struct Welcome { bool operator==(const Welcome&) const = default; };   // joiner is always slot 1
struct Reject { bool operator==(const Reject&) const = default; };     // protocol version mismatch
struct NetInput {
    std::uint32_t tick{0};
    float move_x{0};             // [-1, 1]
    std::uint8_t jump_count{0};  // +1 per press; a lost packet loses no jump
    bool operator==(const NetInput&) const = default;
};
struct NetPlayer {
    float x{0}, y{0}, vx{0}, vy{0};
    bool grounded{false}, alive{false}, at_exit{false};
    bool operator==(const NetPlayer&) const = default;
};
// shortcut: no plates/doors because Game has none yet; add them (by level-file index) when Blake adds them.
struct NetSnapshot {
    std::uint32_t tick{0};
    RoundState round{RoundState::playing};
    std::array<NetPlayer, player_count> players{};
    bool operator==(const NetSnapshot&) const = default;
};
struct Control { ControlKind kind{ControlKind::pause}; bool operator==(const Control&) const = default; };
using Message = std::variant<Hello, Welcome, Reject, NetInput, NetSnapshot, Control>;

std::vector<std::byte> encode(const Message& message);
// nullopt for anything malformed: wrong version, unknown type, wrong size, out-of-range or non-finite values.
std::optional<Message> decode(std::span<const std::byte> bytes);
NetSnapshot snapshot_of(const Game& game, std::uint32_t tick);

// Runs on the hosting machine. Call update() once per frame with this machine's input (slot 0).
class CoopHost {
public:
    CoopHost(Game& game, engine::net::Session& session) : game_(game), session_(session) {}
    void update(const engine::InputFrame& local, float dt);
    void request(ControlKind kind); // local pause/resume/restart/quit, mirrored to the joiner
    bool client_ready() const { return ready_; }
    bool paused() const { return paused_; }
    std::uint32_t tick() const { return tick_; }
private:
    void handle(std::span<const std::byte> bytes);
    Game& game_;
    engine::net::Session& session_;
    bool ready_{false};
    bool paused_{false};
    std::uint32_t tick_{0};
    std::uint32_t last_input_tick_{0};
    float remote_move_{0};
    std::uint8_t remote_jumps_{0};
    bool remote_jump_{false};
    bool quit_pending_{false};
};

// Runs on the joining machine. Draw from latest() until Game::apply_snapshot exists.
// shortcut: client draws from latest() instead of Game::apply_snapshot (Blake, pending); switch when it exists.
class CoopClient {
public:
    explicit CoopClient(engine::net::Session& session) : session_(session) {}
    void update(const engine::InputFrame& local); // sends this frame's input (slot 1)
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
    std::optional<NetSnapshot> latest_;
};
}
