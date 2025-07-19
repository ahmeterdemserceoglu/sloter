#include "DatabaseManager.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <openssl/sha.h>
#include <openssl/rand.h>

DatabaseManager::DatabaseManager() 
    : connection(nullptr)
    , isConnected(false)
    , maxConnections(10)
    , currentConnections(0)
{
    connectionPool.reserve(maxConnections);
}

DatabaseManager::~DatabaseManager() {
    Shutdown();
}

bool DatabaseManager::Initialize(const std::string& host, const std::string& port,
                                const std::string& database, const std::string& username,
                                const std::string& password) {
    
    // Build connection string
    std::stringstream ss;
    ss << "host=" << host 
       << " port=" << port 
       << " dbname=" << database 
       << " user=" << username 
       << " password=" << password
       << " connect_timeout=10"
       << " sslmode=prefer";
    
    connectionString = ss.str();
    
    // Test initial connection
    if (!InitializeConnection()) {
        std::cerr << "Failed to initialize database connection" << std::endl;
        return false;
    }
    
    // Prepare common statements
    if (!PrepareStatements()) {
        std::cerr << "Failed to prepare database statements" << std::endl;
        return false;
    }
    
    // Initialize connection pool
    for (int i = 0; i < maxConnections; i++) {
        PGconn* conn = PQconnectdb(connectionString.c_str());
        if (PQstatus(conn) == CONNECTION_OK) {
            connectionPool.push_back(conn);
        } else {
            PQfinish(conn);
            break;
        }
    }
    
    isConnected = true;
    std::cout << "Database initialized successfully with " << connectionPool.size() << " connections" << std::endl;
    return true;
}

void DatabaseManager::Shutdown() {
    isConnected = false;
    
    // Close all connections in pool
    for (auto conn : connectionPool) {
        if (conn) {
            PQfinish(conn);
        }
    }
    connectionPool.clear();
    
    // Close main connection
    if (connection) {
        PQfinish(connection);
        connection = nullptr;
    }
    
    std::cout << "Database shutdown completed" << std::endl;
}

bool DatabaseManager::InitializeConnection() {
    connection = PQconnectdb(connectionString.c_str());
    
    if (PQstatus(connection) != CONNECTION_OK) {
        std::cerr << "Connection to database failed: " << PQerrorMessage(connection) << std::endl;
        PQfinish(connection);
        connection = nullptr;
        return false;
    }
    
    return true;
}

PGconn* DatabaseManager::GetConnection() {
    if (!connectionPool.empty()) {
        PGconn* conn = connectionPool.back();
        connectionPool.pop_back();
        currentConnections++;
        
        // Test connection
        if (PQstatus(conn) != CONNECTION_OK) {
            PQfinish(conn);
            // Create new connection
            conn = PQconnectdb(connectionString.c_str());
            if (PQstatus(conn) != CONNECTION_OK) {
                PQfinish(conn);
                return nullptr;
            }
        }
        
        return conn;
    }
    
    // Create new connection if pool is empty
    PGconn* conn = PQconnectdb(connectionString.c_str());
    if (PQstatus(conn) == CONNECTION_OK) {
        currentConnections++;
        return conn;
    }
    
    PQfinish(conn);
    return nullptr;
}

void DatabaseManager::ReturnConnection(PGconn* conn) {
    if (conn && connectionPool.size() < maxConnections) {
        connectionPool.push_back(conn);
        currentConnections--;
    } else if (conn) {
        PQfinish(conn);
        currentConnections--;
    }
}

bool DatabaseManager::TestConnection() {
    PGconn* conn = GetConnection();
    if (!conn) return false;
    
    PGresult* result = PQexec(conn, "SELECT 1");
    bool success = (PQresultStatus(result) == PGRES_TUPLES_OK);
    
    PQclear(result);
    ReturnConnection(conn);
    
    return success;
}

std::string DatabaseManager::EscapeString(const std::string& input) {
    if (!connection) return input;
    
    char* escaped = new char[input.length() * 2 + 1];
    PQescapeStringConn(connection, escaped, input.c_str(), input.length(), nullptr);
    
    std::string result(escaped);
    delete[] escaped;
    
    return result;
}

bool DatabaseManager::PrepareStatements() {
    // This would contain prepared statement definitions
    // For brevity, implementing basic functionality
    return true;
}

// User management implementations
bool DatabaseManager::CreateUser(const User& user) {
    PGconn* conn = GetConnection();
    if (!conn) return false;
    
    std::stringstream query;
    query << "INSERT INTO users (username, email, password_hash, salt, balance, daily_limit, "
          << "is_active, is_verified, device_fingerprint, two_factor_secret, is_two_factor_enabled) "
          << "VALUES ('" << EscapeString(user.username) << "', "
          << "'" << EscapeString(user.email) << "', "
          << "'" << EscapeString(user.passwordHash) << "', "
          << "'" << EscapeString(user.salt) << "', "
          << user.balance << ", " << user.dailyLimit << ", "
          << (user.isActive ? "true" : "false") << ", "
          << (user.isVerified ? "true" : "false") << ", "
          << "'" << EscapeString(user.deviceFingerprint) << "', "
          << "'" << EscapeString(user.twoFactorSecret) << "', "
          << (user.isTwoFactorEnabled ? "true" : "false") << ")";
    
    PGresult* result = PQexec(conn, query.str().c_str());
    bool success = (PQresultStatus(result) == PGRES_COMMAND_OK);
    
    if (!success) {
        std::cerr << "Create user failed: " << PQerrorMessage(conn) << std::endl;
    }
    
    PQclear(result);
    ReturnConnection(conn);
    
    return success;
}

User DatabaseManager::GetUserByUsername(const std::string& username) {
    User user = {};
    PGconn* conn = GetConnection();
    if (!conn) return user;
    
    std::stringstream query;
    query << "SELECT id, username, email, password_hash, salt, balance, daily_limit, "
          << "is_active, is_verified, created_at, updated_at, last_login, "
          << "failed_login_attempts, locked_until, device_fingerprint, "
          << "two_factor_secret, is_two_factor_enabled "
          << "FROM users WHERE username = '" << EscapeString(username) << "'";
    
    PGresult* result = PQexec(conn, query.str().c_str());
    
    if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0) {
        user = ParseUserResult(result, 0);
    }
    
    PQclear(result);
    ReturnConnection(conn);
    
    return user;
}

User DatabaseManager::ParseUserResult(PGresult* result, int row) {
    User user = {};
    
    user.id = std::atoi(PQgetvalue(result, row, 0));
    user.username = PQgetvalue(result, row, 1);
    user.email = PQgetvalue(result, row, 2);
    user.passwordHash = PQgetvalue(result, row, 3);
    user.salt = PQgetvalue(result, row, 4);
    user.balance = std::atof(PQgetvalue(result, row, 5));
    user.dailyLimit = std::atof(PQgetvalue(result, row, 6));
    user.isActive = (std::string(PQgetvalue(result, row, 7)) == "t");
    user.isVerified = (std::string(PQgetvalue(result, row, 8)) == "t");
    user.createdAt = PQgetvalue(result, row, 9);
    user.updatedAt = PQgetvalue(result, row, 10);
    user.lastLogin = PQgetvalue(result, row, 11);
    user.failedLoginAttempts = std::atoi(PQgetvalue(result, row, 12));
    user.lockedUntil = PQgetvalue(result, row, 13);
    user.deviceFingerprint = PQgetvalue(result, row, 14);
    user.twoFactorSecret = PQgetvalue(result, row, 15);
    user.isTwoFactorEnabled = (std::string(PQgetvalue(result, row, 16)) == "t");
    
    return user;
}

bool DatabaseManager::UpdateUserBalance(int userId, double newBalance) {
    PGconn* conn = GetConnection();
    if (!conn) return false;
    
    std::stringstream query;
    query << "UPDATE users SET balance = " << newBalance 
          << ", updated_at = CURRENT_TIMESTAMP WHERE id = " << userId;
    
    PGresult* result = PQexec(conn, query.str().c_str());
    bool success = (PQresultStatus(result) == PGRES_COMMAND_OK);
    
    PQclear(result);
    ReturnConnection(conn);
    
    return success;
}

// Session management
std::string DatabaseManager::CreateUserSession(int userId, const std::string& ipAddress, const std::string& userAgent) {
    PGconn* conn = GetConnection();
    if (!conn) return "";
    
    // Generate session tokens
    std::string sessionToken = GenerateRandomToken(64);
    std::string refreshToken = GenerateRandomToken(64);
    
    std::stringstream query;
    query << "INSERT INTO user_sessions (user_id, session_token, refresh_token, "
          << "expires_at, ip_address, user_agent) VALUES ("
          << userId << ", '" << sessionToken << "', '" << refreshToken << "', "
          << "CURRENT_TIMESTAMP + INTERVAL '24 hours', "
          << "'" << EscapeString(ipAddress) << "', "
          << "'" << EscapeString(userAgent) << "')";
    
    PGresult* result = PQexec(conn, query.str().c_str());
    bool success = (PQresultStatus(result) == PGRES_COMMAND_OK);
    
    PQclear(result);
    ReturnConnection(conn);
    
    return success ? sessionToken : "";
}

bool DatabaseManager::ValidateSession(const std::string& sessionToken) {
    PGconn* conn = GetConnection();
    if (!conn) return false;
    
    std::stringstream query;
    query << "SELECT id FROM user_sessions WHERE session_token = '" 
          << EscapeString(sessionToken) << "' AND expires_at > CURRENT_TIMESTAMP AND is_active = true";
    
    PGresult* result = PQexec(conn, query.str().c_str());
    bool valid = (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0);
    
    PQclear(result);
    ReturnConnection(conn);
    
    return valid;
}

// Transaction management
bool DatabaseManager::CreateTransaction(const Transaction& transaction) {
    PGconn* conn = GetConnection();
    if (!conn) return false;
    
    std::stringstream query;
    query << "INSERT INTO transactions (user_id, transaction_id, type, amount, currency, "
          << "status, payment_method, gateway_response, merchant_reference, description) "
          << "VALUES (" << transaction.userId << ", "
          << "'" << EscapeString(transaction.transactionId) << "', "
          << "'" << EscapeString(transaction.type) << "', "
          << transaction.amount << ", "
          << "'" << EscapeString(transaction.currency) << "', "
          << "'" << EscapeString(transaction.status) << "', "
          << "'" << EscapeString(transaction.paymentMethod) << "', "
          << "'" << EscapeString(transaction.gatewayResponse) << "', "
          << "'" << EscapeString(transaction.merchantReference) << "', "
          << "'" << EscapeString(transaction.description) << "')";
    
    PGresult* result = PQexec(conn, query.str().c_str());
    bool success = (PQresultStatus(result) == PGRES_COMMAND_OK);
    
    PQclear(result);
    ReturnConnection(conn);
    
    return success;
}

// Security event logging
bool DatabaseManager::LogSecurityEvent(const SecurityEvent& event) {
    PGconn* conn = GetConnection();
    if (!conn) return false;
    
    std::stringstream query;
    query << "INSERT INTO security_events (user_id, event_type, severity, description, "
          << "ip_address, user_agent, device_fingerprint) VALUES ("
          << event.userId << ", "
          << "'" << EscapeString(event.eventType) << "', "
          << "'" << EscapeString(event.severity) << "', "
          << "'" << EscapeString(event.description) << "', "
          << "'" << EscapeString(event.ipAddress) << "', "
          << "'" << EscapeString(event.userAgent) << "', "
          << "'" << EscapeString(event.deviceFingerprint) << "')";
    
    PGresult* result = PQexec(conn, query.str().c_str());
    bool success = (PQresultStatus(result) == PGRES_COMMAND_OK);
    
    PQclear(result);
    ReturnConnection(conn);
    
    return success;
}

// Utility function to generate random tokens
std::string DatabaseManager::GenerateRandomToken(int length) {
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string result;
    result.reserve(length);
    
    unsigned char buffer[length];
    if (RAND_bytes(buffer, length) != 1) {
        // Fallback to less secure method
        srand(time(nullptr));
        for (int i = 0; i < length; i++) {
            buffer[i] = rand() % 256;
        }
    }
    
    for (int i = 0; i < length; i++) {
        result += charset[buffer[i] % (sizeof(charset) - 1)];
    }
    
    return result;
}

// Game session management
int DatabaseManager::CreateGameSession(const GameSession& session) {
    PGconn* conn = GetConnection();
    if (!conn) return 0;
    
    std::stringstream query;
    query << "INSERT INTO game_sessions (user_id, session_id, device_info, ip_address) "
          << "VALUES (" << session.userId << ", "
          << "'" << EscapeString(session.sessionId) << "', "
          << "'" << EscapeString(session.deviceInfo) << "', "
          << "'" << EscapeString(session.ipAddress) << "') RETURNING id";
    
    PGresult* result = PQexec(conn, query.str().c_str());
    int sessionId = 0;
    
    if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0) {
        sessionId = std::atoi(PQgetvalue(result, 0, 0));
    }
    
    PQclear(result);
    ReturnConnection(conn);
    
    return sessionId;
}

bool DatabaseManager::CreateGameSpin(const GameSpin& spin) {
    PGconn* conn = GetConnection();
    if (!conn) return false;
    
    std::stringstream query;
    query << "INSERT INTO game_spins (user_id, session_id, spin_id, bet_amount, "
          << "win_amount, reel_result, paylines, is_bonus, is_jackpot) VALUES ("
          << spin.userId << ", " << spin.sessionId << ", "
          << "'" << EscapeString(spin.spinId) << "', "
          << spin.betAmount << ", " << spin.winAmount << ", "
          << "'" << EscapeString(spin.reelResult) << "', "
          << "'" << EscapeString(spin.paylines) << "', "
          << (spin.isBonus ? "true" : "false") << ", "
          << (spin.isJackpot ? "true" : "false") << ")";
    
    PGresult* result = PQexec(conn, query.str().c_str());
    bool success = (PQresultStatus(result) == PGRES_COMMAND_OK);
    
    PQclear(result);
    ReturnConnection(conn);
    
    return success;
}

// Statistics
double DatabaseManager::GetTotalRevenue(const std::string& fromDate, const std::string& toDate) {
    PGconn* conn = GetConnection();
    if (!conn) return 0.0;
    
    std::stringstream query;
    query << "SELECT COALESCE(SUM(amount), 0) FROM transactions "
          << "WHERE type = 'deposit' AND status = 'completed'";
    
    if (!fromDate.empty()) {
        query << " AND created_at >= '" << EscapeString(fromDate) << "'";
    }
    if (!toDate.empty()) {
        query << " AND created_at <= '" << EscapeString(toDate) << "'";
    }
    
    PGresult* result = PQexec(conn, query.str().c_str());
    double revenue = 0.0;
    
    if (PQresultStatus(result) == PGRES_TUPLES_OK && PQntuples(result) > 0) {
        revenue = std::atof(PQgetvalue(result, 0, 0));
    }
    
    PQclear(result);
    ReturnConnection(conn);
    
    return revenue;
}

// Maintenance functions
bool DatabaseManager::CleanupExpiredSessions() {
    PGconn* conn = GetConnection();
    if (!conn) return false;
    
    std::string query = "DELETE FROM user_sessions WHERE expires_at < CURRENT_TIMESTAMP";
    
    PGresult* result = PQexec(conn, query.c_str());
    bool success = (PQresultStatus(result) == PGRES_COMMAND_OK);
    
    PQclear(result);
    ReturnConnection(conn);
    
    return success;
}