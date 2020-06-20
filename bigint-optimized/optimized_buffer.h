/* Nikolai Kholiavin, M3138 */

#ifndef OPTIMIZED_BUFFER_H
#define OPTIMIZED_BUFFER_H

#include <cassert>
#include <algorithm>

namespace big_int_util
{
  /* Shared buffer (copy-on-write optimization) */
  template<typename place_t, size_t reserved>
  struct shared_buffer
  {
    size_t ref_count, capacity;
    place_t data[];

    static shared_buffer * allocate_buffer(size_t capacity)
    {
      // apply reserved space
      capacity += reserved;

      shared_buffer *self = static_cast<shared_buffer *>(operator new(sizeof(shared_buffer) +
        sizeof(place_t) * capacity));
      self->ref_count = 1;
      self->capacity = capacity;
      return self;
    }

    static shared_buffer * add_ref(shared_buffer *self)
    {
      self->ref_count++;
      return self;
    }

    static void release(shared_buffer *self)
    {
      if (self == nullptr)
        return;
      if (--self->ref_count == 0)
        operator delete(self);
    }
  };

  /* Buffer (small-object & copy-on-write optimizations) */
  template<typename place_t, size_t reserved>
  struct optimized_buffer
  {
  private:
    using shared_buffer_t = shared_buffer<place_t, reserved>;
    union
    {
      place_t static_data;
      shared_buffer_t *dynamic_data;
    };
    size_t size_ = 0;

    void allocate(size_t new_size, place_t default_val = place_t{0});
    void allocate(size_t new_size, const place_t *old_data, place_t default_val = place_t{0});
    void unshare_resize(size_t new_size = -1, place_t default_val = place_t{0});

  public:
    optimized_buffer(size_t size, place_t default_val);
    optimized_buffer(const std::vector<place_t> &vec);
    optimized_buffer(const optimized_buffer &other);

    optimized_buffer & operator=(const optimized_buffer &other);
    void swap(optimized_buffer &other);

    size_t size() const;
    void resize(size_t new_size, place_t default_val);

    place_t back() const;
    place_t & back();

    void push_back(place_t val);
    void pop_back();

    operator const place_t *() const;
    operator place_t *();
    const place_t * data() const;
    place_t * data();

    place_t * begin();
    const place_t * begin() const;
    place_t * end();
    const place_t * end() const;

    bool operator==(const optimized_buffer &other) const;
    bool operator!=(const optimized_buffer &other) const;

    ~optimized_buffer();
  };

  template<typename place_t, size_t reserved>
  void optimized_buffer<place_t, reserved>::allocate(size_t new_size, place_t default_val)
  {
    assert(new_size > 1);
    dynamic_data = shared_buffer_t::allocate_buffer(new_size);
    std::fill(dynamic_data->data, dynamic_data->data + new_size, default_val);
  }

  template<typename place_t, size_t reserved>
  void optimized_buffer<place_t, reserved>::allocate(size_t new_size, const place_t *old_data,
                                                     place_t default_val)
  {
    assert(new_size > 1);
    dynamic_data = shared_buffer_t::allocate_buffer(new_size);
    std::copy(old_data, old_data + std::min(size_, new_size), dynamic_data->data);
    if (new_size > size_)
      std::fill(dynamic_data->data + size_, dynamic_data->data + new_size, default_val);
  }

  template<typename place_t, size_t reserved>
  void optimized_buffer<place_t, reserved>::unshare_resize(size_t new_size, place_t default_val)
  {
    new_size = new_size == static_cast<size_t>(-1) ? size_ : new_size;
    assert(new_size != size_ || (size_ > 1 && dynamic_data->ref_count > 1));

    place_t buf, *old_data;
    shared_buffer_t *to_release = nullptr;
    if (size_ == 1)
    {
      buf = static_data;
      old_data = &buf;
    }
    else
    {
      old_data = dynamic_data->data;
      to_release = dynamic_data;
    }

    if (new_size == 1)
      static_data = *old_data;
    else
      allocate(std::max(size_ * 3 / 2, new_size), old_data, default_val);
    shared_buffer_t::release(to_release);
    size_ = new_size;
  }

  template<typename place_t, size_t reserved>
  optimized_buffer<place_t, reserved>::optimized_buffer(size_t size, place_t default_val)
  {
    assert(size > 0);
    if (size == 1)
      static_data = default_val;
    else
      allocate(size, default_val);
    size_ = size;
  }

  template<typename place_t, size_t reserved>
  optimized_buffer<place_t, reserved>::optimized_buffer(const std::vector<place_t> &vec) :
    size_(vec.size())
  {
    assert(!vec.empty());
    if (vec.size() == 1)
      static_data = vec[0];
    else
      allocate(vec.size(), vec.data());
  }

  template<typename place_t, size_t reserved>
  optimized_buffer<place_t, reserved>::optimized_buffer(const optimized_buffer &other) :
    size_(other.size_)
  {
    if (size_ == 1)
      static_data = other.static_data;
    else
      dynamic_data = shared_buffer_t::add_ref(other.dynamic_data);
  }

  template<typename place_t, size_t reserved>
  optimized_buffer<place_t, reserved> &
    optimized_buffer<place_t, reserved>::operator=(const optimized_buffer &other)
  {
    if (this == &other)
      return *this;
    optimized_buffer(other).swap(*this);
    return *this;
  }
  template<typename place_t, size_t reserved>
  void optimized_buffer<place_t, reserved>::swap(optimized_buffer &other)
  {
    if (this == &other)
      return;

    if (size_ == 1)
    {
      if (other.size_ == 1)
        std::swap(static_data, other.static_data);
      else
      {
        shared_buffer_t *tmp = other.dynamic_data;
        other.static_data = static_data;
        dynamic_data = tmp;
      }
    }
    else
      if (other.size_ == 1)
      {
        place_t tmp = other.static_data;
        other.dynamic_data = dynamic_data;
        static_data = tmp;
      }
      else
        std::swap(dynamic_data, other.dynamic_data);
    std::swap(size_, other.size_);
  }

  template<typename place_t, size_t reserved>
  size_t optimized_buffer<place_t, reserved>::size() const
  {
    return size_;
  }
  template<typename place_t, size_t reserved>
  void optimized_buffer<place_t, reserved>::resize(size_t new_size, place_t default_val)
  {
    // only inflate
    assert(new_size >= size_);
    if (new_size == size_)
      return;
    if (size_ == 1 || dynamic_data->ref_count > 1 || new_size > dynamic_data->capacity)
      unshare_resize(new_size, default_val);
    else
    {
      std::fill(dynamic_data->data + size_, dynamic_data->data + new_size, default_val);
      size_ = new_size;
    }
  }

  template<typename place_t, size_t reserved>
  place_t optimized_buffer<place_t, reserved>::back() const
  {
    return size_ == 1 ? static_data : dynamic_data->data[size_ - 1];
  }
  template<typename place_t, size_t reserved>
  place_t & optimized_buffer<place_t, reserved>::back()
  {
    if (size_ == 1)
      return static_data;
    if (dynamic_data->ref_count != 1)
      unshare_resize();
    return dynamic_data->data[size_ - 1];
  }

  template<typename place_t, size_t reserved>
  void optimized_buffer<place_t, reserved>::push_back(place_t val)
  {
    resize(size_ + 1, val);
  }
  template<typename place_t, size_t reserved>
  void optimized_buffer<place_t, reserved>::pop_back()
  {
    assert(size_ > 1);
    if (size_ == 2)
      unshare_resize(1);
    else
      size_--;
  }

  template<typename place_t, size_t reserved>
  optimized_buffer<place_t, reserved>::operator const place_t *() const
  {
    return data();
  }
  template<typename place_t, size_t reserved>
  optimized_buffer<place_t, reserved>::operator place_t *()
  {
    return data();
  }
  template<typename place_t, size_t reserved>
  const place_t * optimized_buffer<place_t, reserved>::data() const
  {
    return size_ == 1 ? &static_data : dynamic_data->data;
  }
  template<typename place_t, size_t reserved>
  place_t * optimized_buffer<place_t, reserved>::data()
  {
    if (size_ > 1 && dynamic_data->ref_count > 1)
      unshare_resize();
    return size_ == 1 ? &static_data : dynamic_data->data;
  }

  template<typename place_t, size_t reserved>
  place_t * optimized_buffer<place_t, reserved>::begin()
  {
    return data();
  }
  template<typename place_t, size_t reserved>
  const place_t * optimized_buffer<place_t, reserved>::begin() const
  {
    return data();
  }
  template<typename place_t, size_t reserved>
  place_t * optimized_buffer<place_t, reserved>::end()
  {
    return begin() + size_;
  }
  template<typename place_t, size_t reserved>
  const place_t * optimized_buffer<place_t, reserved>::end() const
  {
    return begin() + size_;
  }

  template<typename place_t, size_t reserved>
  bool optimized_buffer<place_t, reserved>::operator==(const optimized_buffer &other) const
  {
    if (this == &other)
      return true;
    if (size_ != other.size_)
      return false;
    if (size_ == 1)
      return static_data == other.static_data;
    return std::equal(dynamic_data->data, dynamic_data->data + size_,
      other.dynamic_data->data);
  }
  template<typename place_t, size_t reserved>
  bool optimized_buffer<place_t, reserved>::operator!=(const optimized_buffer &other) const
  {
    return !operator==(other);
  }

  template<typename place_t, size_t reserved>
  optimized_buffer<place_t, reserved>::~optimized_buffer()
  {
    if (size_ > 1)
      shared_buffer_t::release(dynamic_data);
  }
}

#endif // OPTIMIZED_BUFFER_H
