#pragma once

namespace ges {

  struct event_delegate {
    using handler_type = void(*)(const void*, void*, void*);
    
    friend bool operator==(const event_delegate&, const event_delegate&);
    
    inline void operator()(const void* event) const
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

  struct view_delegate {
    using handler_type = void(*)(class dispatcher*, void*, void*);
    
    friend bool operator==(const view_delegate& lhs, const view_delegate& rhs)
    {
      return lhs.function == rhs.function &&
        lhs.payload == rhs.payload; 
    }
    
    inline void operator()() const
    {
      handler(dispatcher, function, payload);
    }

    handler_type handler;
    dispatcher* dispatcher;
    void* function;
    void* payload;
  };
}
