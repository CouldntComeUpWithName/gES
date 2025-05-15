#pragma once

namespace ges {
  using handler_type = void(*)(const void*, void*, void*);

  struct event_delegate {
    
    friend bool operator==(const event_delegate&, const event_delegate&);
    
    inline void operator()(const void* event)
    {
      handler(event, function, payload);
    }

    handler_type handler;
    void* function;
    void* payload;
  };

  inline bool operator==(const event_delegate& lhs, const event_delegate& rhs)
  {
    return lhs.function == rhs.function && 
      lhs.payload == rhs.payload; 
  }
}
