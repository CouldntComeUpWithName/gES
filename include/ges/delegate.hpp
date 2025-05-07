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

  bool operator==(const event_delegate& left, const event_delegate& right)
  {
    return left.function == right.function && 
      left.payload == right.payload; 
  }
}
