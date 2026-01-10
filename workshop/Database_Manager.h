#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H
#include <mysql.h>
#include <string>
using std::string;

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();

    bool connect(const string& host,
        const string& user,
        const string& pass,
        const string& db,
        unsigned int port);

    MYSQL* getConnection();

private:
    MYSQL* conn;
};

#endif
