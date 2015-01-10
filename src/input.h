#ifndef C4_INPUT_H
#define C4_INPUT_H

#include <type_traits>
#include <stdexcept>
#include <vector>

namespace c4 {

struct input_error : std::runtime_error {
  using runtime_error::runtime_error;
};

class input {
public:
  input(const char* name);
  input(const input&) = delete;
  input& operator=(const input&) = delete;
  input(input&&) noexcept = default;
  // works on clang. gcc bug?
  input& operator=(input&& o) /* noexcept */ = default; 
  ~input() noexcept;

  const char* begin() const { return begin_; }
  const char* end() const { return end_; }
  const char* name() const { return name_; }
private:
  std::vector<char> input_;
  const char* name_;
  const char* begin_;
  const char* end_;
};

static_assert(std::is_nothrow_move_constructible<input>::value, "type-specification violation");
static_assert(std::is_nothrow_move_assignable<input>::value, "type-specification violation");
static_assert(std::is_nothrow_destructible<input>::value, "type-specification violation");

} // c4
#endif /* C4_INPUT_H */
