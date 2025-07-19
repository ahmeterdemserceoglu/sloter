#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <libpq-fe.h>
#include <json/json.h>

struct User {
    int id;
    std::string username;
    std::string email;
    std::string passwordHash;
    std::string salt;
    double balance;
    double dailyLimit;
    bool isActive;
    bool isVerified;
    std::string createdAt;
    std::string updatedAt;
    std::string lastLogin;
    int failedLoginAttempts;
    std::string lockedUntil;
    std::string deviceFingerprint;
    std::string twoFactorSecret;
    bool isTwoFactorEnabled;
};

struct UserSession {
    int id;
    int userId;
    std::string sessionToken;
    std::string refreshToken;
    std::string expiresAt;
    std::string createdAt;
    std::string ipAddress;
    std::string userAgent;
    bool isActive;
};

struct Transaction {
    int id;
    int userId;
    std::string transactionId;
    std::string type;
    double amount;
    std::string currency;
    std::string status;
    std::string paymentMethod;
    std::string gatewayResponse;
    std::string merchantReference;
    std::string description;
    std::string createdAt;
    std::string updatedAt;
    std::string processedAt;
};

struct GameSession {
    int id;
    int userId;
    std::string sessionId;
    std::string startTime;
    std::string endTime;
    double totalBet;
    double totalWin;
    int spinCount;
    std::string deviceInfo;
    std::string ipAddress;
};

struct GameSpin {
    int id;
    int userId;
    int sessionId;
    std::string spinId;
    double betAmount;
    double winAmount;
    std::string reelResult;
    std::string paylines;
    bool isBonus;
    bool isJackpot;
    std::string createdAt;
};

struct SecurityEvent {
    int id;
    int userId;
    std::string eventType;
    std::string severity;
    std::string description;
    std::string ipAddress;
    std::string userAgent;
    std::string deviceFingerprint;
    Json::Value additionalData;
    std::string createdAt;
    std::string resolvedAt;
    int resolvedBy;
};

class DatabaseManager {
private:
    PGconn* connection;
    std::string connectionString;
    bool isConnected;
    
    // Connection pool
    std::vector<PGconn*> connectionPool;
    int maxConnections;
    int currentConnections;
    
    // Query cache
    std::unordered_map<std::string, std::string> preparedStatements;
    
    // Internal methods
    bool InitializeConnection();
    void CloseConnection();
    PGconn* GetConnection();
    void ReturnConnection(PGconn* conn);
    bool ExecuteQuery(const std::string& query);
    PGresult* ExecuteQueryWithResult(const std::string& query);
    std::string EscapeString(const std::string& input);
    bool PrepareStatements();
    
    // Result parsing
    User ParseUserResult(PGresult* result, int row);
    UserSession ParseSessionResult(PGresult* result, int row);
    Transaction ParseTransactionResult(PGresult* result, int row);
    GameSession ParseGameSessionResult(PGresult* result, int row);
    GameSpin ParseGameSpinResult(PGresult* result, int row);
    SecurityEvent ParseSecurityEventResult(PGresult* result, int row);
    
public:
    DatabaseManager();
    ~DatabaseManager();
    
    // Connection management
    bool Initialize(const std::string& host = "localhost", 
                   const std::string& port = "5432",
                   const std::string& database = "slotmachine_db",
                   const std::string& username = "slotmachine_user",
                   const std::string& password = "SlotMachine2024!@#");
    void Shutdown();
    bool IsConnected() const { return isConnected; }
    bool TestConnection();
    
    // User management
    bool CreateUser(const User& user);
    User GetUser(int userId);
    User GetUserByUsername(const std::string& username);
    User GetUserByEmail(const std::string& email);
    bool UpdateUser(const User& user);
    bool DeleteUser(int userId);
    bool UpdateUserBalance(int userId, double newBalance);
    bool IncrementFailedLogins(int userId);
    bool ResetFailedLogins(int userId);
    bool LockUser(int userId, const std::string& lockUntil);
    bool UnlockUser(int userId);
    
    // Authentication
    bool ValidateUserCredentials(const std::string& username, const std::string& password);
    std::string CreateUserSession(int userId, const std::string& ipAddress, const std::string& userAgent);
    bool ValidateSession(const std::string& sessionToken);
    UserSession GetSession(const std::string& sessionToken);
    bool InvalidateSession(const std::string& sessionToken);
    bool InvalidateAllUserSessions(int userId);
    std::string RefreshSession(const std::string& refreshToken);
    
    // Transaction management
    bool CreateTransaction(const Transaction& transaction);
    Transaction GetTransaction(const std::string& transactionId);
    std::vector<Transaction> GetUserTransactions(int userId, int limit = 50);
    bool UpdateTransactionStatus(const std::string& transactionId, const std::string& status);
    double GetUserDailySpent(int userId);
    std::vector<Transaction> GetTransactionsByStatus(const std::string& status);
    
    // Game session management
    int CreateGameSession(const GameSession& session);
    bool UpdateGameSession(const GameSession& session);
    bool EndGameSession(int sessionId);
    GameSession GetGameSession(int sessionId);
    std::vector<GameSession> GetUserGameSessions(int userId, int limit = 10);
    
    // Game spin management
    bool CreateGameSpin(const GameSpin& spin);
    std::vector<GameSpin> GetSessionSpins(int sessionId);
    std::vector<GameSpin> GetUserSpins(int userId, int limit = 100);
    double GetUserTotalWinnings(int userId);
    double GetUserTotalBets(int userId);
    
    // Security event management
    bool LogSecurityEvent(const SecurityEvent& event);
    std::vector<SecurityEvent> GetSecurityEvents(int userId = 0, const std::string& eventType = "", int limit = 100);
    bool ResolveSecurityEvent(int eventId, int resolvedBy);
    int GetUnresolvedSecurityEventCount(int userId = 0);
    
    // Payment method management
    bool AddPaymentMethod(int userId, const std::string& methodType, const std::string& encryptedData, 
                         const std::string& lastFour = "", int expiryMonth = 0, int expiryYear = 0);
    bool RemovePaymentMethod(int paymentMethodId);
    bool SetDefaultPaymentMethod(int userId, int paymentMethodId);
    
    // Audit logging
    bool LogAuditEvent(int userId, const std::string& action, const std::string& tableName, 
                      int recordId, const Json::Value& oldValues, const Json::Value& newValues,
                      const std::string& ipAddress = "", const std::string& userAgent = "");
    
    // Statistics and reporting
    double GetTotalRevenue(const std::string& fromDate = "", const std::string& toDate = "");
    int GetActiveUserCount();
    int GetTotalUserCount();
    std::unordered_map<std::string, int> GetTransactionStats();
    std::unordered_map<std::string, int> GetSecurityEventStats();
    
    // Maintenance
    bool CleanupExpiredSessions();
    bool CleanupOldAuditLogs(int daysToKeep = 90);
    bool OptimizeDatabase();
    bool BackupDatabase(const std::string& backupPath);
    
    // Batch operations
    bool BatchCreateTransactions(const std::vector<Transaction>& transactions);
    bool BatchUpdateUserBalances(const std::unordered_map<int, double>& balanceUpdates);
};