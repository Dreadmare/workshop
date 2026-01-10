#ifndef CLIMATE_H
#define CLIMATE_H

#include <string>

class Climate {
public:
    Climate(const std::string& type = "clear");

    void setClimate(const std::string& type);
    std::string getClimate() const;

    // We mark this 'const' because it currently uses a hardcoded map
    // rather than a database connection.
    double getModifier() const;

private:
    std::string climateType;
};

#endif