/* Nikolai Kholiavin, M3138 */

#ifndef BIG_INTEGER_H
#define BIG_INTEGER_H

#include <cstddef>
#include <iosfwd>
#include <cstdint>
#include <vector>
#include <string>
#include <functional>

struct big_integer
{
/* all public functions provide weak exception guarantee (invariant holds) */
private:
  // digit type -- only uint32_t supported because of division
  using place_t = uint32_t;
  using storage_t = std::vector<place_t>;

  // invariant:
  // sign -- highest bit in last place (1 -- negative)
  // data has the smallest size representing the same number in 2's complement form
  storage_t data;
  static constexpr int PLACE_BITS = std::numeric_limits<place_t>::digits;

public:
  big_integer();
  big_integer(const big_integer &other) = default;
  big_integer(int a);
  explicit big_integer(std::string const &str);
  ~big_integer();

  big_integer & operator=(const big_integer &other);

  big_integer & operator+=(const big_integer &rhs);
  big_integer & operator-=(const big_integer &rhs);
  big_integer & operator*=(const big_integer &rhs);
  big_integer & operator/=(const big_integer &rhs);
  big_integer & operator%=(const big_integer &rhs);

  big_integer & operator&=(const big_integer &rhs);
  big_integer & operator|=(const big_integer &rhs);
  big_integer & operator^=(const big_integer &rhs);

  big_integer & operator<<=(int rhs);
  big_integer & operator>>=(int rhs);

  big_integer operator+() const;
  big_integer operator-() const;
  big_integer operator~() const;

  big_integer & operator++();
  big_integer operator++(int);

  big_integer & operator--();
  big_integer operator--(int);

  friend bool operator==(const big_integer &a, const big_integer &b);
  friend bool operator!=(const big_integer &a, const big_integer &b);
  friend bool operator<(const big_integer &a, const big_integer &b);
  friend bool operator>(const big_integer &a, const big_integer &b);
  friend bool operator<=(const big_integer &a, const big_integer &b);
  friend bool operator>=(const big_integer &a, const big_integer &b);

  friend std::string to_string(const big_integer &a);

private:
  explicit big_integer(place_t place);

  /* Operators */
  big_integer & short_multiply(place_t rhs);
  big_integer & long_divide(const big_integer &rhs, big_integer &rem);
  big_integer & short_divide(place_t rhs, place_t &rem);
  big_integer & bit_shift(int bits);
  static int compare(const big_integer &l, const big_integer &r);

  /* Invariant-changing functions */
  // corrects sign & invariant
  big_integer & correct_sign_bit(bool expected_sign_bit, place_t carry = 0);
  // corrects invariant
  big_integer & shrink();
  // destroys invariant, inflating data
  void resize(size_t new_size);

  /* Non-invariant-changing function */
  bool make_absolute();
  big_integer & revert_sign(bool sign);

  int sign() const;
  bool sign_bit() const;

  size_t size() const;
  size_t unsigned_size() const;
  place_t default_place() const;
  place_t get_or_default(int64_t at) const;

  /* Useful iterating functions */
  using binary_operator = std::function<place_t (place_t, place_t)>;
  using unary_operator = std::function<place_t (place_t)>;
  using unary_consumer = std::function<void (place_t)>;

  void iterate(const big_integer &b, const binary_operator &action);
  void iterate(const unary_operator &action);
  void iterate_r(const unary_operator &action);
  void iterate(const unary_consumer &action) const;
  void iterate_r(const unary_consumer &action) const;
  big_integer & place_wise(const big_integer &b,
    const binary_operator &action);
};

big_integer operator+(big_integer a, const big_integer &b);
big_integer operator-(big_integer a, const big_integer &b);
big_integer operator*(big_integer a, const big_integer &b);
big_integer operator/(big_integer a, const big_integer &b);
big_integer operator%(big_integer a, const big_integer &b);

big_integer operator&(big_integer a, const big_integer &b);
big_integer operator|(big_integer a, const big_integer &b);
big_integer operator^(big_integer a, const big_integer &b);

big_integer operator<<(big_integer a, int b);
big_integer operator>>(big_integer a, int b);

bool operator==(const big_integer &a, const big_integer &b);
bool operator!=(const big_integer &a, const big_integer &b);
bool operator<(const big_integer &a, const big_integer &b);
bool operator>(const big_integer &a, const big_integer &b);
bool operator<=(const big_integer &a, const big_integer &b);
bool operator>=(const big_integer &a, const big_integer &b);

std::string to_string(const big_integer &a);
std::ostream & operator<<(std::ostream &s, const big_integer &a);

#endif // BIG_INTEGER_H
