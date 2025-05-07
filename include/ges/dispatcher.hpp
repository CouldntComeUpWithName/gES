#pragma once
#include "event_info.hpp"
#include "event_queue.hpp"
#include "delegate.hpp"

#include <unordered_map>
#include <vector>

#include "arena.hpp"

namespace ges {

  class dispatcher {
    using container_type = std::unordered_map<mq::shash_t, void>;
  public:
    dispatcher()
    {
      queue_.create();
    }
    
    template<typename EventType, auto func>
    void listen()
    {
      using event_type = EventType;

      auto* wrapper = wrap<event_type, func>();
      
      event_delegate delegate {
        .handler  = wrapper,
        .function = +func,
        .payload  = nullptr
      };

      secure<event_type>().listeners.push_back(std::move(delegate));
    }
    
    template<typename EventType, auto func, typename Instance>
    void listen(Instance* instance)
    {
      using event_type = EventType;
      using callable_type = decltype(func);

      auto* wrapper = wrap<event_type, func>(instance);

      event_delegate delegate {
        .handler  = wrapper,
        .function = nullptr,
        .payload  = instance
      };

      if constexpr(std::is_function_v<callable_type>)
      {
        delegate.function = +func;
      }
      else
      {
        delegate.function = wrapper;
      }

      secure<event_type>().listeners.push_back(std::move(delegate));
    }

    template<typename EventType, typename Callable>
    void listen(Callable&& callable)
    {
      using callable_type = std::remove_reference_t<std::decay_t<Callable>>;
      using event_type = EventType;
      
      static_assert((std::is_empty_v<callable_type> || std::is_pointer_v<callable_type>), "stateful functor objects are not supported yet");

      auto wrapper = wrap<event_type>(std::forward<Callable>(callable));
      
      event_delegate delegate = {};

      if constexpr (std::is_pointer_v<callable_type>)
      {
        delegate = {
          .handler  = wrapper,
          .function = callable,
        };
      }
      else if constexpr(std::is_empty_v<callable_type>)
      {
        delegate = {
          .handler  = wrapper,
          .function = wrapper,
        };
      }
      
      secure<event_type>().listeners.push_back(std::move(delegate));
    }

    template<typename EventType, typename Callable, typename Instance>
    void listen(Callable&& callable, Instance* instance)
    {

      using callable_type = std::remove_pointer_t<std::decay_t<Callable>>;
      using event_type = EventType;

      static_assert(!std::is_member_function_pointer_v<callable_type>, "you can't dynamically bind member functions. "
        "Consider static linking using listen<typename EventType, auto func, typename Instance>(Instance*) instead");
      
      static_assert(std::is_empty_v<callable_type> || std::is_pointer_v<callable_type>, 
        "Only functor pointers, stateless functor objects and function pointers are allowed");
      
      auto wrapper = wrap<event_type>(std::forward<Callable>(callable), instance);

      if constexpr (std::is_pointer_v<callable_type>)
      {
        event_delegate delegate{
          .handler = wrapper,
          .function = callable,
          .payload = instance,
        };

        secure<event_type>().listeners.push_back(std::move(delegate));
      }
      else if constexpr (std::is_empty_v<callable_type>)
      {
        event_delegate delegate {
          .handler  = wrapper,
          .function = wrapper,
          .payload  = instance,
        };

        secure<event_type>().listeners.push_back(std::move(delegate));
      }

    }

    template<typename EventType, auto func, typename Instance = void>
    bool erase()
    {
      using event_type = EventType;

      constexpr auto type = mq::meta<event_type>().hash;

      auto& handlers = events_[type].listeners;
      
      auto wrapper = wrap<event_type, func>();

      for (auto i = handlers.size(); i; i--)
      {
        if (func == handlers[i - 1u].function)
        {
          handlers.erase(handlers.begin() + i - 1u);
          return true;
        }
      }
      return false;
    }

    template<typename EventType, auto func, typename Instance>
    bool erase(Instance* instance)
    {
      using event_type = EventType;

      constexpr auto type = mq::meta<event_type>().hash;

      auto wrapper = wrap<event_type, func>(instance);

      auto& handlers = events_[type].listeners;

      for (auto i = handlers.size(); i; i--)
      {
        if (wrapper == handlers[i - 1u].function)
        {
          handlers.erase(handlers.begin() + i - 1u);
          return true;
        }
      }
      return false;
    }

    template<typename EventType, typename Callable, typename Instance = void>
    bool erase(Callable&& callable)
    {
      using callable_type = std::decay_t<Callable>;
      using event_type = EventType;

      constexpr auto type = mq::meta<event_type>().hash;

      auto wrapper = wrap<event_type>(std::forward<Callable>(callable));

      auto& handlers = events_[type].listeners;

      if constexpr(std::is_pointer_v<callable_type>)
      {
        for (auto i = handlers.size(); i; i--)
        {
          if (callable == handlers[i - 1u].function)
          {
            handlers.erase(handlers.begin() + i - 1u);
            return true;
          }
        }
      }
      if constexpr(std::is_empty_v<callable_type>)
      {
        for (auto i = handlers.size(); i; i--)
        {
          if (wrapper == handlers[i - 1u].function)
          {
            handlers.erase(handlers.begin() + i - 1u);
            return true;
          }
        }
      }
      
      return false;
    }
    
    template<typename EventType, typename Callable, typename Instance>
    bool erase(Callable&& callable, Instance* instance)
    {
      using event_type = EventType;
      using callable_type = std::decay_t<Callable>;

      constexpr auto type = mq::meta<event_type>().hash;

      auto wrapper = wrap<event_type>(std::forward<Callable>(callable), instance);

      auto& handlers = events_[type].listeners;
      
      if constexpr (std::is_pointer_v<callable_type>)
      {
        for (auto i = handlers.size(); i; i--)
        {
          auto& handler = handlers[i - 1u];

          if (callable == handler.function && instance == handler.payload)
          {
            handlers.erase(handlers.begin() + i - 1u);
            return true;
          }
        }
      }
      else if constexpr(std::is_empty_v<callable_type>)
      {
        for (auto i = handlers.size(); i; i--)
        {
          auto& handler = handlers[i - 1u];

          if (wrapper == handler.function && instance == handler.payload)
          {
            handlers.erase(handlers.begin() + i - 1u);
            return true;
          }
        }
      }

      return false;
    }

    template<typename EventType>
    void trigger(const EventType& event)
    {
      using event_type = EventType;
      auto& handlers = events_.at(mq::meta<event_type>().hash).listeners;

      for (auto i = handlers.size(); i; --i)
        handlers[i - 1u](event);
    }
    
    template<typename EventType, typename... Args>
    void enqueue(Args&&... args)
    {
      queue_.push<EventType>(std::forward<Args>(args)...);
    }

    template<typename EventType, typename... Args>
    void batch(Args&&... args)
    {
      auto& data = events_.at(mq::meta<EventType>::hash);

      data.pool.construct<EventType>(std::forward<Args>(args)...);
    }
    
    void run()
    {
      for (auto& batch : events_)
      {
        auto& pool = batch.second.pool;
        auto& info = batch.second.info;
        auto& handlers = batch.second.listeners;

        auto size = pool.size();
        for (size_t i = 0; i < size; i += info.size)
        {
          for (auto pos = handlers.size(); pos; --pos)
          {
            auto& handler = handlers[pos - 1u];
            const void* event = pool.get(i);
            handler(event);
          }
        }

        pool.reset();
      }
      
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
    }
  private:
    
    template<typename EventType>
    auto& secure()
    {
      using event_type = EventType;
      constexpr auto type = mq::meta<event_type>().hash;

      if (!events_.contains(type))
      {
        auto& event = events_[type];
        
        event.info = event_info{
          .name = mq::meta<event_type>().name,
          .type = type,
          .size = sizeof(event_type)
        };

        if constexpr (!std::is_trivially_destructible_v<event_type>)
        {
          auto& listeners = event.listeners;

          if (listeners.empty())
          {
            event_delegate delegate {
              .handler = destructor<event_type>(),
              .function = destructor<event_type>(),
              .payload = nullptr
            };
            listeners.push_back(std::move(delegate));
          }
        }
        return event;
      }
      
      return events_.at(type);
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

    template<typename EventType, auto func, typename Instance = void>
    auto wrap()
    {
      return +[] (const void* event, void*, void*) mutable {
        func(*static_cast<const EventType*>(event));
      };
    }

    template<typename EventType, auto func, typename Instance>
    auto wrap(Instance* instance)
    {
      return +[](const void* event, void* fn, void* payload) {
        std::invoke(func, (Instance*)payload, *static_cast<const EventType*>(event));
      };
    }

    template<typename EventType, typename Callable, typename Instance = void>
    auto wrap(Callable&& callable)
    {
      using callable_type = std::decay_t<Callable>;
      using event_type    = EventType;
      
      if constexpr(std::is_pointer_v<callable_type>)
      {
        return +[](const void* event, void* fn, void*) {
          (*(callable_type)fn)(*static_cast<const event_type*>(event));
        };
      }
      else if constexpr(std::is_empty_v<callable_type>)
      {
        return +[](const void* event, void* fn, void*) {
          callable_type{}(*static_cast<const event_type*>(event));
        };
      }
    }
    
    template<typename EventType, typename Callable, typename Instance>
    auto wrap(Callable&& callable, Instance* instance)
    {
      using callable_type = std::decay_t<Callable>;
      using event_type    = EventType;

      if constexpr(std::is_function_v<callable_type>)
      {
        return +[] (const void* event, void* fn, void* payload){
          (*(callable_type*)fn)((Instance*)payload, *static_cast<const event_type*>(event));
        };
      }
      else if constexpr(std::is_empty_v<callable_type>)
      {
        return +[](const void* event, void* fn, void* payload) {
          callable_type{}((Instance*)payload, *static_cast<const event_type*>(event));
        };
      }
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
