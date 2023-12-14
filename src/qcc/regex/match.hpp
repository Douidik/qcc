#ifndef QCC_REGEX_MATCH_HPP
#define QCC_REGEX_MATCH_HPP

#include "common.hpp"

namespace qcc::regex {

struct Match {
  std::string_view expr;
  size_t index;

  std::string_view view() const;
  std::string_view next() const;
  const char *begin() const;
  const char *end() const;
  operator bool() const;
};

} // namespace bee::regex

#endif
