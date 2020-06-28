/* Nikolai Kholiavin, M3138 */

#ifndef OPTIMIZED_BUFFER_H
#define OPTIMIZED_BUFFER_H

#include <cassert>
#include <algorithm>
#include <cstdint>
#include <vector>
#include <limits>

namespace big_int_util
{
  /* Shared buffer (copy-on-write optimization) */
  template<typename place_t>
  struct shared_buffer
  {
    size_t ref_count;
    size_t capacity;
    place_t data[];

    static shared_buffer * allocate_buffer(size_t capacity)
    {
      shared_buffer *self = static_cast<shared_buffer *>(operator new(sizeof(shared_buffer) +
                                                                      sizeof(place_t) * capacity));
      self->ref_count = 1;
      self->capacity = capacity;
      return self;
    }

    shared_buffer * add_ref()
    {
      ref_count++;
      return this;
    }

    bool is_unique()
    {
      return ref_count == 1;
    }

    static void release(shared_buffer *self)
    {
      if (self == nullptr)
      {
        return;
      }
      if (--self->ref_count == 0)
      {
        operator delete(self);
      }
    }
  };

  /* uint32_t buffer (small-object & copy-on-write optimizations) */
  class optimized_buffer
  {
  private:
    using shared_buffer_t = shared_buffer<uint32_t>;

    static constexpr size_t STATIC_BUFFER_SIZE = sizeof(shared_buffer_t *) / sizeof(uint32_t);
    static constexpr size_t STATE_MASK = size_t{1} << (std::numeric_limits<size_t>::digits - 1);

    union
    {
      uint32_t static_data[STATIC_BUFFER_SIZE];
      shared_buffer_t *dynamic_data;
    };
    size_t size_ = 0;

    bool is_dynamic_data() const
    {
      return size_ & STATE_MASK;
    }

    bool is_static_data() const
    {
      return !is_dynamic_data();
    }

    void set_is_static_data()
    {
      size_ &= ~STATE_MASK;
    }

    void set_is_dynamic_data()
    {
      size_ |= STATE_MASK;
    }

    void set_size(size_t new_size)
    {
      size_ = new_size | (size_ & STATE_MASK);
    }

    void allocate(size_t new_size, uint32_t default_val = 0, const uint32_t *old_data = nullptr, size_t old_size = 0);
    void unshare(size_t new_size, uint32_t default_val = 0);
    void unshare();
    void ensure_unique();
    void static_inflate(size_t new_size, uint32_t default_val = 0);
    void swap_static_dynamic_data(optimized_buffer &other);

  public:
    using iterator = uint32_t *;
    using const_iterator = const uint32_t *;

    optimized_buffer(size_t size, uint32_t default_val);
    optimized_buffer(const std::vector<uint32_t> &vec);
    optimized_buffer(const optimized_buffer &other);

    optimized_buffer & operator=(const optimized_buffer &other);
    void swap(optimized_buffer &other);

    size_t size() const
    {
      return size_ & ~STATE_MASK;
    }

    void resize(size_t new_size, uint32_t default_val = 0);

    uint32_t back() const;
    uint32_t & back();

    void push_back(uint32_t val);
    void pop_back();

    operator const uint32_t *() const;
    operator uint32_t *();
    const uint32_t * data() const;
    uint32_t * data();

    iterator begin();
    const_iterator begin() const;
    iterator end();
    const_iterator end() const;

    bool operator==(const optimized_buffer &other) const;
    bool operator!=(const optimized_buffer &other) const;

    ~optimized_buffer();
  };
}

#endif // OPTIMIZED_BUFFER_H
