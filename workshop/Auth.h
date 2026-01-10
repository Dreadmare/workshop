#ifndef AUTH_H
#define AUTH_H

#include <string>
#include <chrono>
#include "Database_Manager.h"

class Auth {
public:
    enum class Role { USER, ADMIN };

    Auth(DatabaseManager* db);

    bool login(std::string& loggedInUser);
    bool registerUser(const std::string& username, const std::string& password, Role role = Role::USER);

    bool verify(const std::string& username, const std::string& password);
    static std::string generateSalt(std::size_t length = 16);
    static std::string kdfHash(const std::string& password, const std::string& salt, int iterations = 100000);

private:
    DatabaseManager* db;

    static bool passwordPolicy(const std::string& pw);
    static bool security_compare(const std::string& a, const std::string& b);
};

#endif