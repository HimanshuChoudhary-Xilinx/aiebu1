#ifndef AIEBU_COMMON_REGEX_WRAPPER_H_
#define AIEBU_COMMON_REGEX_WRAPPER_H_

/**
 * @file regex_wrapper.h
 * @brief Unified regex API wrapper supporting both std::regex and boost::regex
 * 
 * This wrapper allows transparent switching between std::regex and boost::regex
 * based on platform requirements:
 * - Ubuntu 22.04 and earlier: uses std::regex (better integration with system)
 * - Ubuntu 24.04+: uses boost::regex (header-only, no packaging issues)
 * - Windows: uses boost::regex (consistent with Boost ecosystem)
 * 
 * The wrapper is controlled by the USE_BOOST_REGEX CMake option.
 */

#ifdef USE_BOOST_REGEX
  #include <boost/regex.hpp>
#else
  #include <regex>
#endif

#include <atomic>
#include <chrono>
#include <cstdint>
#include <utility>

namespace aiebu {

// Cumulative wall time for aiebu::regex_match (thread-safe). Reset at asm parse entry; reported when parse completes.

inline std::atomic<std::uint64_t>&
regex_match_cumulative_nanoseconds_storage()
{
  static std::atomic<std::uint64_t> ns{0};
  return ns;
}

inline void
reset_regex_match_cumulative_time()
{
  //regex_match_cumulative_nanoseconds_storage().store(0, std::memory_order_relaxed);
}

inline std::uint64_t
get_regex_match_cumulative_nanoseconds()
{
  return regex_match_cumulative_nanoseconds_storage().load(std::memory_order_relaxed);
}
/*
template<typename... Args>
bool
regex_match(Args&&... args)
{
  using clock = std::chrono::steady_clock;
  const auto t0 = clock::now();
#ifdef USE_BOOST_REGEX
  const bool ok = boost::regex_match(std::forward<Args>(args)...);
#else
  const bool ok = std::regex_match(std::forward<Args>(args)...);
#endif
  const auto t1 = clock::now();
  const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
  if (elapsed > 0)
    regex_match_cumulative_nanoseconds_storage().fetch_add(
        static_cast<std::uint64_t>(elapsed), std::memory_order_relaxed);
  return ok;
}
*/
#ifdef USE_BOOST_REGEX
  // Use Boost.Regex
  using regex = boost::regex;
  using smatch = boost::smatch;
  using cmatch = boost::cmatch;
  using sregex_iterator = boost::sregex_iterator;
  using regex_error = boost::regex_error;
  
  // Import functions into aiebu namespace
  using boost::regex_match;
  using boost::regex_search;
  using boost::regex_replace;
  
  // Regex constants
  namespace regex_constants {
    using namespace boost::regex_constants;
  }
  
#else
  // Use std::regex
  using regex = std::regex;
  using smatch = std::smatch;
  using cmatch = std::cmatch;
  using sregex_iterator = std::sregex_iterator;
  using regex_error = std::regex_error;
  // Import functions into aiebu namespace
  using std::regex_match;
  using std::regex_search;
  using std::regex_replace;
  
  // Regex constants
  namespace regex_constants {
    using namespace std::regex_constants;
  }
  
#endif

} // namespace aiebu

#endif // AIEBU_COMMON_REGEX_WRAPPER_H_

