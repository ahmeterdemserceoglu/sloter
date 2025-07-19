#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>
#include "../database/DatabaseManager.h"

enum class AuthResult {
    SUCCESS = 0,
    INVALID_CREDENTIALS,
    USER_NOT_FOUND,
    USER_LOCKED,
    USER_INACTIVE,
    EMAIL_NOT_VERIFIED,
    TWO_FACTOR_REQUIRED,
    INVALID_TWO_FACTOR,
    SESSION_EXPIRED,
    INVALID_SESSION,
    RATE_LIMITED,
    SECURITY_VIOLATION,
    DATABASE_ERROR,
    UNKNOWN_ERROR
};

enum class UserRole {
    GUEST = 0,
    PLAYER,
    VIP_PLAYER,
    MODERATOR,
    ADMIN,
    SUPER_ADMIN
};

struct AuthToken {
    std::string accessToken;
    std::string refreshToken;
    std::string tokenType;
    int expiresIn;
    std::string scope;
    std::string userId;
};

struct LoginRequest {
    std::string username;
    std::string password;
    std::string deviceFingerprint;
    std::string ipAddress;
    std::string userAgent;
    std::string twoFactorCode;
    bool rememberMe;
};

struct RegisterRequest {
    std::string username;
    std::string email;
    std::string password;
    std::string confirmPassword;
    std::string deviceFingerprint;
    std::string ipAddress;
    std::string userAgent;
    bool acceptTerms;
    std::string referralCode;
};

struct PasswordResetRequest {
    std::string email;
    std::string resetToken;
    std::string newPassword;
    std::string confirmPassword;
};

struct TwoFactorSetup {
    std::string secret;
    std::string qrCodeUrl;
    std::vector<std::string> backupCodes;
};

class AuthManager {
private:
    std::shared_ptr<DatabaseManager> database;
    
    // Security settings
    int maxFailedAttempts;
    int lockoutDurationMinutes;
    int sessionTimeoutMinutes;
    int refreshTokenTimeoutDays;
    bool requireEmailVerification;
    bool requireTwoFactor;
    
    // Rate limiting
    std::unordered_map<std::string, std::vector<uint64_t>> loginAttempts;
    std::unordered_map<std::string, std::vector<uint64_t>> registrationAttempts;
    
    // Password policy
    int minPasswordLength;
    bool requireUppercase;
    bool requireLowercase;
    bool requireNumbers;
    bool requireSpecialChars;
    std::vector<std::string> commonPasswords;
    
    // JWT settings
    std::string jwtSecret;
    std::string jwtIssuer;
    
    // Email settings
    std::string smtpServer;
    int smtpPort;
    std::string smtpUsername;
    std::string smtpPassword;
    
    // Internal methods
    std::string HashPassword(const std::string& password, const std::string& salt);
    std::string GenerateSalt();
    bool VerifyPassword(const std::string& password, const std::string& hash, const std::string& salt);
    bool ValidatePasswordPolicy(const std::string& password);
    bool IsRateLimited(const std::string& identifier, const std::string& action);
    void UpdateRateLimit(const std::string& identifier, const std::string& action);
    std::string GenerateJWT(int userId, const std::string& username, UserRole role);
    bool ValidateJWT(const std::string& token);
    std::string GenerateRandomToken(int length = 32);
    bool SendVerificationEmail(const std::string& email, const std::string& token);
    bool SendPasswordResetEmail(const std::string& email, const std::string& token);
    bool SendTwoFactorEmail(const std::string& email, const std::string& code);
    std::string GenerateTwoFactorSecret();
    bool ValidateTwoFactorCode(const std::string& secret, const std::string& code);
    std::string GenerateQRCodeUrl(const std::string& username, const std::string& secret);
    std::vector<std::string> GenerateBackupCodes();
    
public:
    AuthManager(std::shared_ptr<DatabaseManager> db);
    ~AuthManager();
    
    bool Initialize();
    void Shutdown();
    
    // Authentication
    AuthResult Login(const LoginRequest& request, AuthToken& token);
    AuthResult Register(const RegisterRequest& request, std::string& userId);
    AuthResult Logout(const std::string& sessionToken);
    AuthResult LogoutAll(int userId);
    AuthResult RefreshToken(const std::string& refreshToken, AuthToken& newToken);
    
    // Session management
    bool ValidateSession(const std::string& sessionToken);
    UserSession GetSessionInfo(const std::string& sessionToken);
    bool ExtendSession(const std::string& sessionToken);
    std::vector<UserSession> GetUserSessions(int userId);
    bool InvalidateSession(const std::string& sessionToken);
    
    // Password management
    AuthResult ChangePassword(int userId, const std::string& currentPassword, const std::string& newPassword);
    AuthResult RequestPasswordReset(const std::string& email);
    AuthResult ResetPassword(const PasswordResetRequest& request);
    bool ValidatePasswordStrength(const std::string& password);
    
    // Email verification
    AuthResult SendVerificationEmail(int userId);
    AuthResult VerifyEmail(const std::string& token);
    bool IsEmailVerified(int userId);
    
    // Two-factor authentication
    TwoFactorSetup SetupTwoFactor(int userId);
    AuthResult EnableTwoFactor(int userId, const std::string& code);
    AuthResult DisableTwoFactor(int userId, const std::string& password);
    AuthResult ValidateTwoFactor(int userId, const std::string& code);
    std::vector<std::string> RegenerateBackupCodes(int userId);
    bool ValidateBackupCode(int userId, const std::string& code);
    
    // User management
    User GetUserInfo(int userId);
    bool UpdateUserInfo(const User& user);
    bool DeactivateUser(int userId);
    bool ReactivateUser(int userId);
    bool DeleteUser(int userId);
    
    // Security
    bool CheckSecurityViolation(int userId, const std::string& action);
    void LogSecurityEvent(int userId, const std::string& eventType, const std::string& description, 
                         const std::string& ipAddress = "", const std::string& userAgent = "");
    bool IsUserLocked(int userId);
    bool UnlockUser(int userId);
    void LockUser(int userId, int durationMinutes = 0);
    
    // Device management
    bool RegisterDevice(int userId, const std::string& deviceFingerprint, const std::string& deviceInfo);
    bool IsDeviceRegistered(int userId, const std::string& deviceFingerprint);
    std::vector<std::string> GetUserDevices(int userId);
    bool RemoveDevice(int userId, const std::string& deviceFingerprint);
    
    // Role and permissions
    UserRole GetUserRole(int userId);
    bool SetUserRole(int userId, UserRole role);
    bool HasPermission(int userId, const std::string& permission);
    std::vector<std::string> GetUserPermissions(int userId);
    
    // Statistics and monitoring
    int GetActiveSessionCount();
    int GetFailedLoginCount(const std::string& timeframe = "24h");
    std::vector<SecurityEvent> GetRecentSecurityEvents(int limit = 100);
    std::unordered_map<std::string, int> GetLoginStats();
    
    // Configuration
    void SetPasswordPolicy(int minLength, bool requireUpper, bool requireLower, 
                          bool requireNumbers, bool requireSpecial);
    void SetSecuritySettings(int maxFailed, int lockoutMinutes, int sessionTimeout);
    void SetEmailSettings(const std::string& server, int port, const std::string& username, 
                         const std::string& password);
    void SetJWTSettings(const std::string& secret, const std::string& issuer);
    
    // Maintenance
    bool CleanupExpiredSessions();
    bool CleanupExpiredTokens();
    bool CleanupOldSecurityEvents(int daysToKeep = 30);
    
    // Callbacks
    void SetLoginCallback(std::function<void(int, const std::string&)> callback);
    void SetLogoutCallback(std::function<void(int, const std::string&)> callback);
    void SetSecurityViolationCallback(std::function<void(int, const std::string&)> callback);
    
private:
    std::function<void(int, const std::string&)> onLogin;
    std::function<void(int, const std::string&)> onLogout;
    std::function<void(int, const std::string&)> onSecurityViolation;
};