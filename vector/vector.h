/* Kholiavin Nikolai, M3138 */

#ifndef VECTOR_H
#define VECTOR_H

#include <cstring>
#include <algorithm>

template<typename T>
struct vector
{
  typedef T * iterator;
  typedef const T * const_iterator;

  vector() = default;                     // O(1) nothrow
  vector(const vector &);                 // O(N) strong
  vector & operator=(const vector &other); // O(N) strong

  ~vector();                              // O(N) nothrow

  T & operator[](size_t i);               // O(1) nothrow
  const T & operator[](size_t i) const;   // O(1) nothrow

  T * data();                             // O(1) nothrow
  const T * data() const;                 // O(1) nothrow
  size_t size() const;                    // O(1) nothrow

  T & front();                            // O(1) nothrow
  const T & front() const;                // O(1) nothrow

  T & back();                             // O(1) nothrow
  const T & back() const;                 // O(1) nothrow
  void push_back(const T &);              // O(1)* strong
  void pop_back();                        // O(1) nothrow

  bool empty() const;                     // O(1) nothrow

  size_t capacity() const;                // O(1) nothrow
  void reserve(size_t);                   // O(N) strong
  void shrink_to_fit();                   // O(N) strong

  void clear();                           // O(N) nothrow

  void swap(vector &);                    // O(1) nothrow

  iterator begin();                       // O(1) nothrow
  iterator end();                         // O(1) nothrow

  const_iterator begin() const;           // O(1) nothrow
  const_iterator end() const;             // O(1) nothrow

  iterator insert(const_iterator pos, const T &); // O(N) weak
  iterator erase(const_iterator pos);     // O(N) weak
  iterator erase(const_iterator first, const_iterator last); // O(N) weak

private:
  void ensure_capacity(size_t new_capacity);
  void new_buffer(size_t new_capacity);

private:
  T *data_ = nullptr;
  size_t size_ = 0;
  size_t capacity_ = 0;

  static void destroy_at(T *data, size_t at);
  static void destroy_n(T *data, size_t begin, size_t end);
  static void copy_construct_at(T *data, size_t at, const T &other);
  static void copy_construct_n(T *data, size_t begin, size_t end, T *other, size_t other_begin);
};

template<typename T>
void vector<T>::destroy_at(T *data, size_t at)
{
  data[at].~T();
}

template<typename T>
void vector<T>::destroy_n(T *data, size_t begin, size_t end)
{
  for (size_t i = end; i > begin; i--)
    destroy_at(data, i - 1);
}

template<typename T>
void vector<T>::copy_construct_at(T *data, size_t at, const T &other)
{
  new(data + at) T(other);
}

template<typename T>
void vector<T>::copy_construct_n(T *data, size_t begin, size_t end, T *other, size_t other_begin)
{
  size_t i = 0;

  try
  {
    for (i = 0; i < end - begin; i++)
      copy_construct_at(data, begin + i, other[other_begin + i]);
  }
  catch (...)
  {
    destroy_n(data, begin, begin + i);
    throw;
  }
}

template<typename T>
void vector<T>::new_buffer(size_t new_capacity)
{
  new_capacity = std::max(new_capacity, size_);
  T *new_data;
  if (new_capacity == 0)
    new_data = nullptr;
  else
  {
    new_data = static_cast<T *>(operator new(new_capacity * sizeof(T)));
    try
    {
      copy_construct_n(new_data, 0, size_, data_, 0);
    }
    catch (...)
    {
      operator delete(new_data);
      throw;
    }
  }
  destroy_n(data_, 0, size_);
  operator delete(data_);
  data_ = new_data;
  capacity_ = new_capacity;
}

template<typename T>
void vector<T>::ensure_capacity(size_t new_capacity)
{
  if (capacity_ >= new_capacity)
    return;
  new_buffer(std::max(capacity_ * 3 / 2, new_capacity));
}

template<typename T>
vector<T>::vector(const vector &other)
{
  new_buffer(other.size_);
  copy_construct_n(data_, 0, other.size_, other.data_, 0);
  size_ = other.size_;
}

template<typename T>
vector<T> & vector<T>::operator=(const vector &other) {
  if (this != &other)
    vector(other).swap(*this);
  return *this;
}

template<typename T>
vector<T>::~vector()
{
  clear();
  operator delete(data_);
}

template<typename T>
T & vector<T>::operator[](size_t i)
{
  return data_[i];
}

template<typename T>
const T & vector<T>::operator[](size_t i) const
{
  return data_[i];
}

template<typename T>
T * vector<T>::data()
{
  return data_;
}

template<typename T>
const T * vector<T>::data() const
{
  return data_;
}

template<typename T>
size_t vector<T>::size() const
{
  return size_;
}

template<typename T>
T & vector<T>::front()
{
  return data_[0];
}

template<typename T>
const T & vector<T>::front() const
{
  return data_[0];
}

template<typename T>
T & vector<T>::back()
{
  return data_[size_ - 1];
}

template<typename T>
const T & vector<T>::back() const
{
  return data_[size_ - 1];
}

template<typename T>
void vector<T>::push_back(const T &other)
{
  if (size_ < capacity_)
  {
    copy_construct_at(data_, size_, other);
    ++size_;
  }
  else
  {
    T buf = other;
    ensure_capacity(size_ + 1);
    copy_construct_at(data_, size_, buf);
    ++size_;
  }
}

template<typename T>
void vector<T>::pop_back()
{
  destroy_at(data_, --size_);
}

template<typename T>
bool vector<T>::empty() const
{
  return size_ == 0;
}

template<typename T>
size_t vector<T>::capacity() const
{
  return capacity_;
}

template<typename T>
void vector<T>::reserve(size_t new_capacity)
{
  ensure_capacity(new_capacity);
}

template<typename T>
void vector<T>::shrink_to_fit()
{
  if (capacity_ != size_)
    new_buffer(size_);
}

template<typename T>
void vector<T>::clear()
{
  destroy_n(data_, 0, size_);
  size_ = 0;
}

template<typename T>
void vector<T>::swap(vector &other)
{
  std::swap(data_, other.data_);
  std::swap(size_, other.size_);
  std::swap(capacity_, other.capacity_);
}

template<typename T>
typename vector<T>::iterator vector<T>::begin()
{
  return data_;
}

template<typename T>
typename vector<T>::iterator vector<T>::end()
{
  return data_ + size_;
}

template<typename T>
typename vector<T>::const_iterator vector<T>::begin() const
{
  return data_;
}

template<typename T>
typename vector<T>::const_iterator vector<T>::end() const
{
  return data_ + size_;
}

template<typename T>
typename vector<T>::iterator vector<T>::insert(const_iterator pos, const T &val)
{
  size_t before = static_cast<size_t>(pos - begin());

  push_back(val);
  for (size_t i = size_ - 1; i > before; i--)
    std::swap(data_[i - 1], data_[i]);

  return begin() + before;
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(const_iterator pos)
{
  return erase(pos, pos + 1);
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(const_iterator first, const_iterator last)
{
  size_t l = static_cast<size_t>(first - begin()), r = static_cast<size_t>(last - begin());
  size_t dst, src;
  for (dst = l, src = r; src < size_; dst++, src++)
    std::swap(data_[dst], data_[src]);
  destroy_n(data_, dst, size_);
  size_ -= r - l;
  return begin() + l;
}

#endif // VECTOR_H
