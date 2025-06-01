#pragma once

namespace ges {

  template<typename EventType>
  class viewer {
    friend class dispatcher;
  public:
    using event_type     = EventType;
    using value_type     = EventType;
    
    using iterator       = const event_type*;
    using const_iterator = const event_type*;
    
    using size_type      = size_t;
  
  public:
    viewer(const viewer&)            = default;
    viewer(viewer&&)                 = default;
    viewer& operator=(const viewer&) = default;
    viewer& operator=(viewer&&)      = default;

  public:
    iterator begin() { return base_; }
    iterator end() { return base_ + size_; }

    const_iterator begin() const { return base_; }
    const_iterator end() const { return base_ + size_; }
    
    bool empty() const { return size() == 0; }

    const event_type& at(size_t index) const { return *(base_ + index); }

    const event_type* data() const { return base_; }

    size_t size() const { return size_; }

  private:
    viewer(event_type* base, size_type size)
      : base_{base}, size_{size}
    { }
    viewer() = default;
    
  private:
    event_type* base_ = nullptr;
    size_type size_   = 0ull;
  };

} // namespace ges