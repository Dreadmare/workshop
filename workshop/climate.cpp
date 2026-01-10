#include "Climate.h"
#include <unordered_map>

Climate::Climate(const std::string& type) : climateType(type) {}

void Climate::setClimate(const std::string& type) {
    climateType = type;
}

std::string Climate::getClimate() const {
    return climateType;
}

double Climate::getModifier() const {
    static const std::unordered_map<std::string, double> modifiers = {
        {"clear", 1.0},
        {"rainy", 0.9},
        {"hot", 0.92},
        {"cold", 0.88},
        {"snowy", 0.75},
        {"sandstorm", 0.5}
    };

    auto it = modifiers.find(climateType);
    if (it != modifiers.end()) {
        return it->second;
    }
    // default
    return 0.95;
}