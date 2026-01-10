#include "Database_Manager.h"
#include <iostream>

DatabaseManager::DatabaseManager() : conn(nullptr) {}

DatabaseManager::~DatabaseManager() {
    if (conn) mysql_close(conn);
}

bool DatabaseManager::connect(const std::string& host,
    const std::string& user,
    const std::string& pass,
    const std::string& db,
    unsigned int port) {
    conn = mysql_init(0);
    if (!conn) {
        std::cerr << "MySQL init failed\n";
        return false;
    }

    conn = mysql_real_connect(conn, host.c_str(), user.c_str(),
        pass.c_str(), db.c_str(), port, NULL, 0);
    if (conn) {
        std::cout << "Connected to MySQL database: " << db << std::endl;
        return true;
    }
    else {
        std::cerr << "MySQL connection failed: " << mysql_error(conn) << std::endl;
        return false;
    }
}

MYSQL* DatabaseManager::getConnection() { return conn; }
