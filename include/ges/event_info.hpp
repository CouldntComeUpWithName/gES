#pragma once
#include <string_view>
#include <cstdint>

struct event_info {
  std::string_view name;
  uint32_t type = 0;
  uint32_t size = -1;
};
