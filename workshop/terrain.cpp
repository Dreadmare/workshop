#include "Terrain.h"
#include <iostream>
#include <cstring>

const std::map<std::string, double> Terrain::defaultModifiers = {
    {"plain", 1.0},
    {"mountain", 0.8},
    {"desert", 0.85},
    {"urban", 0.95},
    {"forest", 0.75},
    {"swamp", 0.8},
    {"mud", 0.7}
};

Terrain::Terrain(const std::string& type, DatabaseManager* db)
    : terrainType(type), db(db) {
}

void Terrain::setTerrain(const std::string& type) {
    terrainType = type;
}

std::string Terrain::getTerrain() const {
    return terrainType;
}

double Terrain::getModifier() {
    if (!db || !db->getConnection()) {
        auto it = defaultModifiers.find(terrainType);
        return (it != defaultModifiers.end()) ? it->second : 0.9;
    }

    double modifier = 0.9;
    MYSQL_STMT* stmt = mysql_stmt_init(db->getConnection());
    const char* sql = "SELECT terrain_mod FROM terrain_data WHERE terrain_name = ? LIMIT 1";

    if (mysql_stmt_prepare(stmt, sql, strlen(sql)) == 0) {
        MYSQL_BIND b_in[1];
        memset(b_in, 0, sizeof(b_in));
        b_in[0].buffer_type = MYSQL_TYPE_STRING;
        b_in[0].buffer = (char*)terrainType.c_str();
        b_in[0].buffer_length = terrainType.length();

        mysql_stmt_bind_param(stmt, b_in);
        mysql_stmt_execute(stmt);

        MYSQL_BIND b_out[1];
        memset(b_out, 0, sizeof(b_out));
        b_out[0].buffer_type = MYSQL_TYPE_DOUBLE;
        b_out[0].buffer = (char*)&modifier;

        mysql_stmt_bind_result(stmt, b_out);

        if (mysql_stmt_fetch(stmt) != 0) {
            auto it = defaultModifiers.find(terrainType);
            modifier = (it != defaultModifiers.end()) ? it->second : 0.9;
        }
    }

    mysql_stmt_close(stmt);
    return modifier;
}