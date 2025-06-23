#pragma once
#include "event_info.hpp"
#include "event_queue.hpp"
#include "delegate.hpp"
#include "batcher.hpp"
#include "viewer.hpp"

#include <unordered_map>
#include <vector>
#include <cassert>

namespace ges {

  class dispatcher {
    using self_type = dispatcher;
  public:
    dispatcher()
    {
      queue_.create();
    }

    template<typename EventType, auto func>
    self_type& listen_view()
    {
      static_assert(!is_viewer<EventType>::value, "expected T instead of ges::viewer<T>");

      using event_type = EventType;

      secure<event_type>().viewers.push_back(wrap_view<event_type, func>());
      return *this;
    }

    template<typename EventType, auto func>
    self_type& listen()
    {
      static_assert(!is_viewer<EventType>::value, "ges::viewer<T> can\'t be registered as event type");
      static_assert(!is_batcher<EventType>::value, "ges::batcher<T> can\'t be registered as event type");

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

      static_assert((std::is_pointer_v<callable_type> || std::is_empty_v<callable_type>),
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

      static_assert(std::is_pointer_v<callable_type> || std::is_empty_v<callable_type>,
        "Only functor pointers, stateless functor objects and function pointers are allowed");

      secure<event_type>().listeners.push_back(wrap<event_type>(callable, instance));

      return *this;
    }

    template<typename EventType, auto func>
    bool unlisten()
    {
      using event_type = EventType;

      auto delegate = wrap<event_type, func>();

      return try_erase_one(delegate, mq::meta<event_type>().hash);
    }

    template<typename EventType, auto func, typename Instance>
    bool unlisten(Instance* instance)
    {
      using event_type = EventType;

      auto delegate = wrap<event_type, func>(instance);

      return try_erase_one(delegate, mq::meta<event_type>().hash);
    }

    template<typename EventType, typename Callable>
    bool unlisten(Callable callable)
    {
      using callable_type = Callable;
      using event_type = EventType;

      auto delegate = wrap<event_type>(callable);

      return try_erase_one(delegate, mq::meta<event_type>().hash);
    }

    template<typename EventType, typename Callable, typename Instance>
    bool unlisten(Callable callable, Instance* instance)
    {
      using event_type = EventType;
      using callable_type = Callable;

      auto delegate = wrap<event_type>(callable, instance);

      return try_erase_one(delegate, mq::meta<event_type>().hash);
    }

    template<typename EventType>
    void clear()
    {
      using event_type = EventType;

      auto iter = events_.find(mq::meta<event_type>().hash);
      if (iter == events_.end())
        return;

      events_.erase(iter);
    }

    template<typename EventType>
    void trigger(const EventType& event)
    {
      using event_type = EventType;

      auto iter = events_.find(mq::meta<event_type>().hash);
      
      assert(iter != events_.end());

      auto& handlers = iter->second.listeners;

      if constexpr (std::is_trivially_destructible_v<event_type>)
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
    void emit_bus(Args&&... args)
    {
      queue_.push<EventType>(std::forward<Args>(args)...);
    }

    template<typename EventType>
    void emit_bus(EventType&& event)
    {
      queue_.push<EventType>(std::forward<EventType>(event));
    }

    template<typename EventType, typename... Args>
    void emit(Args&&... args)
    {
      auto& data = events_.at(mq::meta<EventType>::hash);

      data.pool.construct<EventType>(std::forward<Args>(args)...);
    }

    template<typename EventType>
    void emit(EventType&& event)
    {
      auto& data = events_.at(mq::meta<EventType>::hash);

      data.pool.construct<EventType>(std::forward<EventType>(event));
    }

    template<typename EventType>
    bool contains()
    {
      using event_type = EventType;

      auto iter = events_.find(mq::meta<event_type>::hash);

      return iter != events_.end();
    }

    template<typename EventType>
    viewer<EventType> view()
    {
      using event_type = EventType;
      auto iter = events_.find(mq::meta<event_type>::hash);

      if (iter == events_.end())
        return viewer<event_type>();

      auto& arena = iter->second.pool;

      auto size = arena.size() / sizeof(event_type);
      return viewer<event_type>((event_type*)arena.data(), size);
    }

    template<typename EventType>
    batcher<EventType> batch()
    {
      auto& arena = events_[mq::meta<EventType>::hash].pool;

      return batcher<EventType>(arena);
    }

    template<typename EventType>
    void run()
    {
      auto it = events_.find(mq::meta<EventType>::hash);

      assert(it != events_.end());

      auto& data = it->second;

      auto& pool = data.pool;
      const auto& handlers = data.listeners;

      if (pool.empty())
        return;

      for (auto& viewer : data.viewers)
      {
        viewer();
      }

#if 0
      if (!handlers.empty())
      {
        auto size = pool.size();
        for (size_t i = 0; i < size; i += data.info.size)
        {
          for (auto pos = handlers.size(); pos; --pos)
          {
            auto& handler = handlers[pos - 1u];
            const void* event = pool.get(i);
            handler(event);
          }
        }
      }

#else
      for (auto pos = handlers.size(); pos; --pos)
      {
        auto& handler = handlers[pos - 1u];
        auto size = pool.size();
        for (size_t i = 0; i < size; i += data.info.size)
        {
          const void* event = pool.get(i);
          handler(event);
        }
      }
#endif // 0
      pool.reset();
    }

    void run()
    {
      for (auto& [type, data] : events_)
      {
        auto& pool = data.pool;
        const auto& handlers = data.listeners;
        
        if (pool.empty())
          continue;

        for (auto& viewer : data.viewers)
        {
          viewer();
        }
        
      #if 1
        if (!handlers.empty())
        {
          auto size = pool.size();
          for (size_t i = 0; i < size; i += data.info.size)
          {
            for (auto pos = handlers.size(); pos; --pos)
            {
              auto& handler = handlers[pos - 1u];
              const void* event = pool.get(i);
              handler(event);
            }
          }
        }

      #else
        for (auto pos = handlers.size(); pos; --pos)
        {
          auto& handler = handlers[pos - 1u];
          auto size = pool.size();
          for (size_t i = 0; i < size; i += data.info.size)
          {
            const void* event = pool.get(i);
            handler(event);
          }
        }
      #endif // 0
        pool.reset();
      }
    }

    void run_bus()
    {
      while (!queue_.empty())
      {
        const uint32_t& type = queue_.check();

        auto& data = events_.at(type);

        const void* event = queue_.peek();

        auto& handlers = data.listeners;

        auto size = handlers.size();
        for (auto pos = size; pos; --pos)
        {
          auto& handler = handlers[pos - 1u];
          handler(event);
        }

        queue_.pop(data.info.size);
      }
      queue_.reset();
    }
    
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
    
    template<typename EventType, auto func>
    auto wrap_view()
    {
      auto* wrapper = +[] (dispatcher* self, void*, void*) mutable {
        func(self->view<EventType>());
      };

      return view_delegate {
        .handler    = wrapper,
        .dispatcher = this,
        .function   = func,
        .payload    = nullptr
      };
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
      std::vector<view_delegate> viewers;
      std::vector<event_delegate> listeners;
      arena pool;
    };
    
    std::unordered_map<uint32_t, event_data> events_;
    event_queue queue_;
  };
  

}// namespace ges
