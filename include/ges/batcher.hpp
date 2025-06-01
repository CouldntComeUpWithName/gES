#pragma once
#include "arena.hpp"

namespace ges {
  
  template<typename EventType>
  class batcher {
    friend class dispatcher;
  public:
    using event_type = EventType;
    using arena_type = arena;
    using size_type  = size_t;
    
  public:
    batcher(const batcher&) = default;
    batcher(batcher&&) = default;
    batcher& operator=(const batcher&) = default;
    batcher& operator=(batcher&&)noexcept = default;

    void push_back(event_type&& event)
    {
      arena_->construct<event_type>(std::forward<event_type>(event));
    }
    
    void push_back(const event_type& event)
    {
      arena_->construct<event_type>(event);
    }

    template<typename... Args>
    void emplace_back(Args&&... args)
    {
      arena_->construct<event_type>(std::forward<Args>(args)...);
    }
    
    template<typename Iterator>
    void insert(Iterator begin, Iterator end)
    {
      arena_->insert(begin, end);
    }
    
    void clear()
    {
      if constexpr(!std::is_trivially_destructible<event_type>::value)
      {
        const event_type* begin = (const event_type*)arena_->data();
        const event_type* end = begin + (arena_->size() / sizeof(event_type));
      
        for(auto it = begin; it != end; ++it)
        {
          it->~event_type();
        }
      }
      
      arena_->clear();
    }

    size_type size() const { return arena_->size() / sizeof(event_type); }
    
    void resize(size_type nsize) 
    { 
      arena_->resize(nsize * sizeof(event_type));
    }

    void reset() { arena_->reset(); }

  private:
    batcher(arena& arena)
      : arena_{&arena}
    { }

  private:
    arena_type* arena_;
  };
} // namespace ges