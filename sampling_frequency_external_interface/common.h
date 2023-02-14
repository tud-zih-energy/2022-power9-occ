#pragma once

#include <cstdint>
#include <string>
#include <stdexcept>

enum class sample_source_t : uint64_t {
    none = 0,
    hwmon = 1,
    ping = 2,
    pong = 3,
    both_use_ping = 4,
    both_use_pong = 5,
};

std::string sample_source_to_str(const sample_source_t x) {
    switch(x) {
    case sample_source_t::none:
        return "none";
    case sample_source_t::hwmon:
        return "hwmon";
    case sample_source_t::ping:
        return "ping";
    case sample_source_t::pong:
        return "pong";
    case sample_source_t::both_use_ping:
        return "both_use_ping";
    case sample_source_t::both_use_pong:
        return "both_use_pong";
    default:
        throw std::runtime_error("unknown sample source type");
    }
}
