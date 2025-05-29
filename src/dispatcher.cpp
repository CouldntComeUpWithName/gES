#include <ges/dispatcher.hpp>

void ges::dispatcher::run()
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
    queue_.reset();
  }