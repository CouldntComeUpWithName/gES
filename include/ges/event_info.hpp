#pragma once
#include <string_view>
#include <cstdint>

namespace ges {
  
  struct event_info {
    std::string_view name;
    uint32_t type = 0;
    uint32_t size = 0;
  };

} // namespace ges
