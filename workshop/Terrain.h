#ifndef TERRAIN_H
#define TERRAIN_H

#include <string>
#include <map>
#include "Database_Manager.h"

class Terrain {
public:
    Terrain(const std::string& type = "plain", DatabaseManager* db = nullptr);

    void setTerrain(const std::string& type);
    double getModifier();
    std::string getTerrain() const;

private:
    std::string terrainType;
    DatabaseManager* db;

    static const std::map<std::string, double> defaultModifiers;
};

#endif