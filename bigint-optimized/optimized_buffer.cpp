/* Nikolai Kholiavin, M3138 */
#include "optimized_buffer.h"

using namespace big_int_util;

void optimized_buffer::allocate(size_t new_size, uint32_t default_val, const uint32_t *old_data, size_t old_size)
{
  set_is_dynamic_data();
  dynamic_data = shared_buffer_t::allocate_buffer(new_size > old_size ? std::max(old_size * 3 / 2, new_size) : new_size);
  if (old_data != nullptr)
  {
    std::copy_n(old_data, std::min(old_size, new_size), dynamic_data->data);
  }
  if (new_size > old_size)
  {
    std::fill(dynamic_data->data + old_size, dynamic_data->data + new_size, default_val);
  }
  set_size(new_size);
}

void optimized_buffer::unshare(size_t new_size, uint32_t default_val)
{
  new_size = new_size == static_cast<size_t>(-1) ? size() : new_size;
  assert(is_dynamic_data() && (new_size > size() || dynamic_data->ref_count > 1));

  shared_buffer_t *old_data = dynamic_data;
  allocate(new_size, default_val, old_data->data, size());
  shared_buffer_t::release(old_data);
}

void optimized_buffer::static_inflate(size_t new_size, uint32_t default_val)
{
  assert(is_static_data() && new_size > STATIC_BUFFER_SIZE);

  uint32_t buffer[STATIC_BUFFER_SIZE];
  std::copy_n(static_data, STATIC_BUFFER_SIZE, buffer);
  allocate(new_size, default_val, buffer, size());
}

optimized_buffer::optimized_buffer(size_t size, uint32_t default_val)
{
  if (size <= STATIC_BUFFER_SIZE)
  {
    set_is_static_data();
    std::fill_n(static_data, size, default_val);
    set_size(size);
  }
  else
  {
    allocate(size, default_val);
  }
}

optimized_buffer::optimized_buffer(const std::vector<uint32_t> &vec)
{
  if (vec.size() <= STATIC_BUFFER_SIZE)
  {
    set_is_static_data();
    std::copy_n(vec.begin(), vec.size(), static_data);
    set_size(vec.size());
  }
  else
  {
    allocate(vec.size(), 0, vec.data(), vec.size());
  }
}

optimized_buffer::optimized_buffer(const optimized_buffer &other) : size_(other.size_)
{
  if (other.is_static_data())
  {
    std::copy_n(other.static_data, other.size(), static_data);
  }
  else
  {
    dynamic_data = shared_buffer_t::add_ref(other.dynamic_data);
  }
}

optimized_buffer & optimized_buffer::operator=(const optimized_buffer &other)
{
  if (this == &other)
  {
    return *this;
  }
  optimized_buffer(other).swap(*this);
  return *this;
}

void optimized_buffer::swap_static_dynamic(optimized_buffer &other)
{
  assert(is_static_data());
  shared_buffer_t *tmp = other.dynamic_data;
  std::copy_n(static_data, size(), other.static_data);
  dynamic_data = tmp;
}

void optimized_buffer::swap(optimized_buffer &other)
{
  if (this == &other)
  {
    return;
  }

  if (is_static_data())
  {
    if (other.is_static_data())
    {
      std::swap(static_data, other.static_data);
    }
    else
    {
      swap_static_dynamic(other);
    }
  }
  else if (other.is_static_data())
  {
    other.swap_static_dynamic(*this);
  }
  else
  {
    std::swap(dynamic_data, other.dynamic_data);
  }
  std::swap(size_, other.size_);
}

void optimized_buffer::resize(size_t new_size, uint32_t default_val)
{
  // only inflate
  new_size = std::max(size(), new_size);

  if (new_size == size())
  {
    return;
  }
  if (is_static_data() && new_size > STATIC_BUFFER_SIZE)
  {
    static_inflate(new_size, default_val);
  }
  else if (is_dynamic_data() && (dynamic_data->ref_count > 1 || new_size > dynamic_data->capacity))
  {
    unshare(new_size, default_val);
  }
  else
  {
    uint32_t *data = is_static_data() ? static_data : dynamic_data->data;
    std::fill(data + size(), data + new_size, default_val);
    set_size(new_size);
  }
}

uint32_t optimized_buffer::back() const
{
  return data()[size() - 1];
}

uint32_t & optimized_buffer::back()
{
  if (is_static_data())
  {
    return static_data[size() - 1];
  }
  if (dynamic_data->ref_count > 1)
  {
    unshare();
  }
  return dynamic_data->data[size() - 1];
}

void optimized_buffer::push_back(uint32_t val)
{
  resize(size() + 1, val);
}

void optimized_buffer::pop_back()
{
  set_size(size() - 1);
}

optimized_buffer::operator const uint32_t *() const
{
  return data();
}

optimized_buffer::operator uint32_t *()
{
  return data();
}

const uint32_t * optimized_buffer::data() const
{
  return is_static_data() ? static_data : dynamic_data->data;
}

uint32_t * optimized_buffer::data()
{
  if (is_dynamic_data() && dynamic_data->ref_count > 1)
  {
    unshare();
  }
  return is_static_data() ? static_data : dynamic_data->data;
}

optimized_buffer::iterator optimized_buffer::begin()
{
  return data();
}

optimized_buffer::const_iterator optimized_buffer::begin() const
{
  return data();
}

optimized_buffer::iterator optimized_buffer::end()
{
  return begin() + size();
}

optimized_buffer::const_iterator optimized_buffer::end() const
{
  return begin() + size();
}

bool optimized_buffer::operator==(const optimized_buffer &other) const
{
  if (this == &other)
  {
    return true;
  }
  if (size() != other.size())
  {
    return false;
  }
  return std::equal(begin(), end(), other.begin());
}

bool optimized_buffer::operator!=(const optimized_buffer &other) const
{
  return !operator==(other);
}

optimized_buffer::~optimized_buffer()
{
  if (is_dynamic_data())
  {
    shared_buffer_t::release(dynamic_data);
  }
}
