#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace engine::net::room_wire {

enum class Kind : std::uint8_t { create = 1, join, created, paired, rejected, data };
constexpr std::size_t packet_limit = 512; // Includes the room envelope byte.
constexpr std::size_t code_length = 10;
constexpr std::string_view alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";

inline std::vector<std::byte> packet(Kind kind, std::string_view body = {}) {
    std::vector<std::byte> bytes{static_cast<std::byte>(kind)};
    for (unsigned char character : body) bytes.push_back(static_cast<std::byte>(character));
    return bytes;
}

inline std::string read_text(std::span<const std::byte> body) {
    return {reinterpret_cast<const char*>(body.data()), body.size()};
}

inline std::string normalize_code(std::string code) {
    for (char& c : code) {
        if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
        if (alphabet.find(c) == std::string_view::npos) return {};
    }
    return code.size() == code_length ? code : std::string{};
}

}
