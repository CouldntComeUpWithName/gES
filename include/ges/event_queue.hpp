#pragma once
#include "core.hpp"
#include <metaq.hpp>

namespace ges {
  
  class event_queue {
    friend class dispatcher;
  public:
    static constexpr auto PAGE_SIZE = 4096ULL * 256;
    static constexpr auto MAX_SIZE = PAGE_SIZE / 4ULL;
    
    template<typename EventType, typename... Args>
    void push(Args&&... args)
    {
      static_assert(sizeof(EventType) <= MAX_SIZE);
      using event_type = EventType;

      constexpr auto size = sizeof(event_type) + sizeof(mq::shash_t);

      byte* slot = acquire(size);

      auto* type = ::new(slot) mq::shash_t(mq::meta<event_type>().hash);
      
      void* event = slot + sizeof(mq::shash_t);

      ::new(event) event_type(std::forward<Args>(args)...);
    }

  private:
    const mq::shash_t& check()
    {
      return *(mq::shash_t*)pointer;
    }

    const void* peek()
    {
      return pointer + sizeof(mq::shash_t);
    }

    void pop(size_t offset)
    {
      pointer += sizeof(mq::shash_t) + offset;
    }

    bool empty() const
    {
      return pointer >= pool + size;
    }
    
    void reset() 
    { 
      size = 0;
      pointer = pool;
    }

    void create()
    {
      pool = new byte[PAGE_SIZE];
      pointer = pool;
      size = 0;
    }
    
    //void reserve();

    void release()
    {
      if(pool)
        delete pool;
      
      size = 0;
    }

    byte* acquire(size_t sz)
    {
      assert(sz + size < PAGE_SIZE);

      byte* slot = &pool[size];
      size += sz;
      
      return slot;
    }

    event_queue() = default;
    
    ~event_queue()
    {
      release();
    }

  private:
    byte* pool = nullptr;
    byte* pointer = nullptr;
    size_t size = 0;
    size_t capacity = 0;
  };

} // namespace ges