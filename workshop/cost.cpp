#include "Cost.h"
#include <algorithm>

static constexpr double FUEL_PRICE_PER_L = 2.0;   // RM per liter
static constexpr double OVERHEAD_FACTOR = 1.10;  // overhead

double Cost::calculate(double kmPerL) const {
    if (kmPerL <= 0.0) return 0.0;
    double costPerKm = (1.0 / kmPerL) * FUEL_PRICE_PER_L * OVERHEAD_FACTOR;
    return std::clamp(costPerKm, 0.0, 100.0);
}
