#include <atomic>
#include <cstdint>

namespace crl {
using time = std::int64_t;
using profile_time = std::int64_t;

time current_value();
time compute_adjustment();
time now();
}  // namespace crl