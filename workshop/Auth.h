#ifndef AUTH_H
#define AUTH_H

#include <string>
#include "Database_Manager.h"

class Auth {
public:
    enum class Role { USER, ADMIN };

    Auth(DatabaseManager* db);

    // Authentication
    bool login(std::string& loggedInUser, Role& loggedInRole);

    // Database operations
    bool registerUser(const std::string& username, const std::string& password, Role role = Role::USER);
    bool updateUser(const std::string& currentUser, Role currentRole,const std::string& targetUser, const std::string& newPassword);
    bool deleteUser(const std::string& currentUser, Role currentRole, const std::string& targetUser);
    bool getUserRole(const std::string& username, Role& role);

    bool verify(const std::string& username, const std::string& password);
    bool listAllUsers(const std::string& adminUser, Role adminRole);
    bool changeUserRole(const std::string& adminUser, Role adminRole, const std::string& targetUser, Role newRole);

    // Utility
    static std::string roleToString(Role role);
    static Role stringToRole(const std::string& roleStr);

    // Helper
    bool userExists(const std::string& username); 

private:
    DatabaseManager* db;
    void setEcho(bool enable);
};
#endif