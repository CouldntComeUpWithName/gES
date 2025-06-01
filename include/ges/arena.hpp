#pragma once
#include "core.hpp"
#include <cstring>

namespace ges {
  // a type erased container
  class arena {
  public:
    arena() = default;
  
    arena(const arena& other)
    {
      _copy(other);
    }

    arena(arena&& other) noexcept
    {
      _move(std::move(other));
    }

    arena& operator=(const arena& other)
    {
      if(this == &other)
      {
        return *this;
      }

      _copy(other);
    }

    arena& operator=(arena&& other) noexcept
    {
      if(this == &other)
      {
        return *this;
      }

      _move(std::move(other));
    }

    template<typename T, typename... Args>
    T* construct(Args&&... args)
    {
      if(sizeof(T) + size() > capacity())
      {
        reserve(sizeof(T) * empty() + (capacity() * 2));
      }

      size_ += sizeof(T);
      return ::new(size_ - sizeof(T)) T{std::forward<Args>(args)...};
    }

    template<typename Iterator>
    void insert(Iterator begin, Iterator end)
    {
      using value_t = Iterator::value_type;
      
      for(; begin != end; ++begin)
      {
        construct<value_t>(*begin);
      }
    }

    void* data() { return data_; }
    const void* data() const { return data_; }

    template<typename T = void>
    T* get(size_t pos)
    {
      return reinterpret_cast<T*>(data_ + pos);
    }
    
    template<typename T = void>
    const T* get(size_t pos) const
    {
      return reinterpret_cast<T*>(data_ + pos);
    }

    void clear()
    {
      if(data_)
      {
        delete[] data_;
        
        data_ = nullptr;
        size_ = nullptr;
        capacity_ = nullptr;
      }
    }
    
    bool empty()
    {
      return !size();
    }

    void reserve(size_t ncapacity)
    {
      if(capacity() < ncapacity)
      {
        byte* new_data = new byte[ncapacity];
        size_t sz = size();
        
        _memcopy(new_data, data_, sz);
        
        data_ = new_data;
        size_ = data_ + sz;
        capacity_ = data_ + ncapacity;
      }
    }

    void reset()
    {
      size_ = data_;
    }
    
    void resize(size_t nsize)
    {
      if(nsize > capacity())
      {
        reserve(nsize);
      }
      size_ = data_ + nsize;
    }

    // returns capacity in bytes
    size_t capacity() const { return static_cast<size_t>(capacity_ - data_); }
    
    // returns size in bytes
    size_t size() const { return static_cast<size_t>(size_ - data_); }

  private:
    void _memcopy(byte* dst, byte* src, size_t size)
    {
      std::memcpy(dst, src, size);
    }
      
    void _copy(const arena& src)
    {
      if(src.capacity() > capacity())
      {
        clear();
        data_ = new byte[src.capacity()];
      }
      
      _memcopy(data_, src.data_, src.size());
      
      size_     = data_ + src.size();
      capacity_ = data_ + src.capacity();
    }

    void _move(arena&& other) noexcept
    {
      this->clear();
      
      data_ = other.data_;
      size_ = other.size_;
      capacity_ = other.capacity_;

      other.data_ = nullptr;
      other.size_ = nullptr;
      other.capacity_ = nullptr;
    }

  private:
    byte* data_     = nullptr;
    byte* size_     = nullptr;
    byte* capacity_ = nullptr;
  };
}
