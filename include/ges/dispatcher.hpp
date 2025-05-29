#pragma once
#include "event_info.hpp"
#include "event_queue.hpp"
#include "delegate.hpp"
#include "arena.hpp"

#include <unordered_map>
#include <vector>

namespace ges {

  class dispatcher {
    using self_type = dispatcher;
  public:
    dispatcher()
    {
      queue_.create();
    }
    
    template<typename EventType, auto func>
    self_type& listen()
    {
      using event_type = EventType;

      secure<event_type>().listeners.push_back(wrap<event_type, func>());
      return *this;
    }
    
    template<typename EventType, auto func, typename Instance>
    self_type& listen(Instance* instance)
    {
      using event_type = EventType;

      secure<event_type>().listeners.push_back(wrap<event_type, func>(instance));
      return *this;
    }

    template<typename EventType, typename Callable>
    self_type& listen(Callable callable)
    {
      using callable_type = Callable;
      using event_type = EventType;
      
      static_assert((std::is_empty_v<callable_type> || std::is_pointer_v<callable_type>), 
        "stateful functor objects are not supported yet");

      secure<event_type>().listeners.push_back(wrap<event_type>(callable));
      return *this;
    }

    template<typename EventType, typename Callable, typename Instance>
    self_type& listen(Callable callable, Instance* instance)
    {
      using callable_type = Callable;
      using event_type = EventType;

      static_assert(!std::is_member_function_pointer_v<callable_type>, "you can't dynamically bind member functions. "
        "Consider static linking using listen<typename EventType, auto func, typename Instance>(Instance*) instead");
      
      static_assert(std::is_empty_v<callable_type> || std::is_pointer_v<callable_type>, 
        "Only functor pointers, stateless functor objects and function pointers are allowed");

      secure<event_type>().listeners.push_back(wrap<event_type>(callable, instance));
      
      return *this;
    }

    template<typename EventType, auto func>
    bool erase()
    {
      using event_type = EventType;

      auto delegate = wrap<event_type, func>();

      return try_erase_one(delegate, mq::meta<event_type>().hash);
    }

    template<typename EventType, auto func, typename Instance>
    bool erase(Instance* instance)
    {
      using event_type = EventType;

      auto delegate = wrap<event_type, func>(instance);

      return try_erase_one(delegate, mq::meta<event_type>().hash);
    }

    template<typename EventType, typename Callable>
    bool erase(Callable callable)
    {
      using callable_type = Callable;
      using event_type    = EventType;

      auto delegate = wrap<event_type>(callable);
      
      return try_erase_one(delegate, mq::meta<event_type>().hash);
    }
    
    template<typename EventType, typename Callable, typename Instance>
    bool erase(Callable callable, Instance* instance)
    {
      using event_type    = EventType;
      using callable_type = Callable;

      auto delegate = wrap<event_type>(callable, instance);
      
      return try_erase_one(delegate, mq::meta<event_type>().hash);
    }
    
    template<typename EventType>
    void clear()
    {
      using event_type = EventType;
      
      auto iter = events_.find(mq::meta<event_type>().hash);

      if(iter == events_.end())
        return;
      
      auto& handlers = iter->second.listeners;
      handlers.clear();
    }

    template<typename EventType>
    bool contains()
    {
      using event_type = EventType;
      
      auto iter = events_.find(mq::meta<event_type>::hash);

      if(iter == events_.end())
        return false;

      auto& handlers = iter->second.listeners;

      return !handlers.empty();
    }

    template<typename EventType>
    bool has()
    {
      using event_type = EventType;

      auto iter = events_.find(mq::meta<event_type>::hash);
      
      return iter != events_.end();
    }

    template<typename EventType>
    void trigger(const EventType& event)
    {
      using event_type = EventType;
      
      auto iter = events_.find(mq::meta<event_type>().hash);
      
      if(iter == events_.end())
      {
        return;
      }
      
      auto& handlers = iter->second.listeners;

      if constexpr(std::is_trivially_destructible_v<event_type>)
      {
        for (auto i = handlers.size(); i; --i)
          handlers[i - 1u](&event);
      }
      else 
      {
        for (auto i = handlers.size(); i > 1; --i)
          handlers[i - 1u](&event);
      }
    }
    
    template<typename EventType, typename... Args>
    void enqueue(Args&&... args)
    {
      queue_.push<EventType>(std::forward<Args>(args)...);
    }

    template<typename EventType>
    void enqueue(EventType&& event)
    {
      queue_.push<EventType>(std::forward<EventType>(event));
    }

    template<typename EventType, typename... Args>
    void batch(Args&&... args)
    {
      auto& data = events_.at(mq::meta<EventType>::hash);

      data.pool.construct<EventType>(std::forward<Args>(args)...);
    }
    
    template<typename EventType>
    void batch(EventType&& event)
    {
      auto& data = events_.at(mq::meta<EventType>::hash);

      data.pool.construct<EventType>(std::forward<EventType>(event));
    }
    
    void run();
    
  private:
    template<typename EventType>
    auto& secure()
    {
      using event_type = EventType;
      constexpr auto type = mq::meta<event_type>().hash;

      auto iter = events_.find(type);
      if (iter == events_.end())
      {
        auto& event_data = events_[type];
        
        event_data.info = event_info {
          .name = mq::meta<event_type>().name,
          .type = type,
          .size = sizeof(event_type)
        };

        if constexpr (!std::is_trivially_destructible_v<event_type>)
        {
          auto& listeners = event_data.listeners;

          if (listeners.empty())
          {
            event_delegate destroy {
              .handler  = destructor<event_type>(),
              .function = destructor<event_type>(),
              .payload  = nullptr
            };
            listeners.push_back(destroy);
          }
        }
        return event_data;
      }
      
      return iter->second;
    }
    
    template<typename EventType>
    auto destructor()
    {
      using event_type = EventType;
      
      return +[] (const void* event, void*, void*) {
        const event_type* e = static_cast<const event_type*>(event);
        e->~event_type();
      };
    }

    template<typename EventType, auto func>
    auto wrap()
    {
      auto* wrapper = +[] (const void* event, void*, void*) mutable {
        func(*static_cast<const EventType*>(event));
      };

      return event_delegate {
        .handler  = wrapper,
        .function = func,
        .payload  = nullptr
      };
    }

    template<typename EventType, auto func, typename Instance>
    auto wrap(Instance* instance)
    {
      using callable_type = decltype(func);

      if constexpr(std::is_member_function_pointer<callable_type>::value)
      {
        auto* wrapper = +[](const void* event, void* fn, void* payload) {
          ((Instance*)payload->*func)(*static_cast<const EventType*>(event));
        };

        return event_delegate {
          .handler  = wrapper,
          .function = wrapper,
          .payload  = instance
        };
      }
      else 
      {
        auto* wrapper = +[](const void* event, void* fn, void* payload) {
          func((Instance*)payload, *static_cast<const EventType*>(event));
        };

        return event_delegate {
          .handler  = wrapper,
          .function = func,
          .payload  = instance
        };
      }
    }

    template<typename EventType, typename Callable>
    auto wrap(Callable callable)
    {
      using callable_type = Callable;
      using event_type    = EventType;
      
      if constexpr(std::is_pointer_v<callable_type>)
      {
        auto* wrapper = +[](const void* event, void* fn, void*) {
          (*(callable_type)fn)(*static_cast<const event_type*>(event));
        };
        
        return event_delegate {
          .handler  = wrapper,
          .function = callable,
          .payload  = nullptr
        };
      }
      else if constexpr(std::is_empty_v<callable_type>)
      {
        auto* wrapper = +[](const void* event, void* fn, void*) {
          callable_type{}(*static_cast<const event_type*>(event));
        };

        return event_delegate {
          .handler  = wrapper,
          .function = wrapper,
          .payload  = nullptr
        };
      }
    }
    
    template<typename EventType, typename Callable, typename Instance>
    auto wrap(Callable callable, Instance* instance)
    {
      using callable_type = Callable;
      using event_type    = EventType;

      if constexpr (std::is_pointer_v<callable_type>)
      {
        auto* wrapper = +[](const void* event, void* fn, void* payload) {
          (*(callable_type)fn)((Instance*)payload, *static_cast<const event_type*>(event));
        };

        return event_delegate {
          .handler  = wrapper,
          .function = callable,
          .payload  = instance
        };
      }
      else if constexpr (std::is_empty_v<callable_type>)
      {
        auto* wrapper = +[](const void* event, void* fn, void* payload) {
          callable_type{}((Instance*)payload, *static_cast<const event_type*>(event));
        };

        return event_delegate {
          .handler  = wrapper,
          .function = wrapper,
          .payload  = instance
        };
      }
    }

    bool try_erase_one(const event_delegate& delegate, mq::shash_t type)
    {
      auto iter = events_.find(type);

      if (iter == events_.end())
        return false;

      auto& handlers = iter->second.listeners;

      for (auto i = handlers.size(); i; i--)
      {
        const auto& handler = handlers[i - 1u];

        if (delegate == handler)
        {
          handlers.erase(handlers.begin() + i - 1u);
          return true;
        }
      }
      
      return false;
    }

  private:
    struct event_data {
      event_info info;
      std::vector<event_delegate> listeners;
      arena pool;
    };
    
    std::unordered_map<uint32_t, event_data> events_;
    event_queue queue_;
  };
  

}// namespace ges
