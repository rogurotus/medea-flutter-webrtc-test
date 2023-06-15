#include "crl.h"

#include <chrono>

namespace crl {

time StartValue;
std::atomic<std::uint32_t> AdjustSeconds;

time current_value() {
  auto now = std::chrono::steady_clock::now();
  auto duration = now.time_since_epoch();
  auto millis =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  return time(millis);
}

time compute_adjustment() {
  return time(AdjustSeconds.load() * 1000);
}

time now() {
  const auto elapsed = current_value() - StartValue;
  return elapsed + compute_adjustment();
}

struct StaticInit {
  StaticInit();
};

StaticInit::StaticInit() {
  StartValue = current_value();
}

}  // namespace crl
