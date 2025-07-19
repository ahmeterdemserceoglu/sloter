#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

enum class PaymentMethod {
    CREDIT_CARD = 0,
    DEBIT_CARD,
    PAYPAL,
    GOOGLE_PAY,
    APPLE_PAY,
    CRYPTO,
    BANK_TRANSFER,
    GIFT_CARD
};

enum class TransactionStatus {
    PENDING = 0,
    PROCESSING,
    COMPLETED,
    FAILED,
    CANCELLED,
    REFUNDED,
    DISPUTED
};

enum class Currency {
    USD = 0,
    EUR,
    GBP,
    JPY,
    CAD,
    AUD,
    BTC,
    ETH
};

struct PaymentInfo {
    std::string cardNumber;      // Encrypted
    std::string expiryDate;
    std::string cvv;            // Encrypted
    std::string holderName;
    std::string billingAddress;
    PaymentMethod method;
};

struct Transaction {
    std::string transactionId;
    std::string userId;
    double amount;
    Currency currency;
    PaymentMethod method;
    TransactionStatus status;
    uint64_t timestamp;
    std::string description;
    std::string merchantReference;
    std::string gatewayResponse;
    bool isRefundable;
};

struct WalletBalance {
    double balance;
    Currency currency;
    uint64_t lastUpdated;
    std::vector<Transaction> recentTransactions;
};

class PaymentSystem {
private:
    // Wallet management
    std::unordered_map<std::string, WalletBalance> userWallets;
    
    // Transaction management
    std::vector<Transaction> transactionHistory;
    std::unordered_map<std::string, Transaction> pendingTransactions;
    
    // Security
    std::string encryptionKey;
    std::vector<std::string> fraudPatterns;
    
    // Payment gateways
    std::unordered_map<PaymentMethod, std::string> gatewayEndpoints;
    std::unordered_map<PaymentMethod, bool> gatewayStatus;
    
    // Fraud detection
    std::unordered_map<std::string, std::vector<uint64_t>> userTransactionTimes;
    std::unordered_map<std::string, double> dailySpendLimits;
    
    // Callbacks
    std::function<void(const Transaction&)> onTransactionComplete;
    std::function<void(const Transaction&)> onTransactionFailed;
    
    // Internal methods
    std::string GenerateTransactionId();
    std::string EncryptSensitiveData(const std::string& data);
    std::string DecryptSensitiveData(const std::string& encryptedData);
    bool ValidatePaymentInfo(const PaymentInfo& paymentInfo);
    bool DetectFraud(const std::string& userId, double amount);
    bool ProcessWithGateway(Transaction& transaction, const PaymentInfo& paymentInfo);
    void UpdateTransactionStatus(const std::string& transactionId, TransactionStatus status);
    double ConvertCurrency(double amount, Currency from, Currency to);
    
public:
    PaymentSystem();
    ~PaymentSystem();
    
    bool Initialize();
    void Shutdown();
    
    // Wallet operations
    bool CreateWallet(const std::string& userId, Currency currency = Currency::USD);
    double GetBalance(const std::string& userId, Currency currency = Currency::USD);
    bool AddFunds(const std::string& userId, double amount, const PaymentInfo& paymentInfo);
    bool DeductFunds(const std::string& userId, double amount);
    bool TransferFunds(const std::string& fromUserId, const std::string& toUserId, double amount);
    
    // Transaction operations
    std::string ProcessPayment(const std::string& userId, double amount, 
                              const PaymentInfo& paymentInfo, const std::string& description);
    bool RefundTransaction(const std::string& transactionId, double amount = 0.0);
    Transaction GetTransaction(const std::string& transactionId);
    std::vector<Transaction> GetUserTransactions(const std::string& userId, int limit = 50);
    
    // Security and validation
    bool ValidateTransaction(const std::string& userId, double amount);
    void SetDailySpendLimit(const std::string& userId, double limit);
    double GetDailySpendLimit(const std::string& userId);
    double GetDailySpent(const std::string& userId);
    
    // Payment methods
    bool AddPaymentMethod(const std::string& userId, const PaymentInfo& paymentInfo);
    bool RemovePaymentMethod(const std::string& userId, const std::string& methodId);
    std::vector<PaymentInfo> GetPaymentMethods(const std::string& userId);
    
    // Gateway management
    bool IsGatewayAvailable(PaymentMethod method);
    void SetGatewayStatus(PaymentMethod method, bool isAvailable);
    
    // Callbacks
    void SetTransactionCompleteCallback(std::function<void(const Transaction&)> callback);
    void SetTransactionFailedCallback(std::function<void(const Transaction&)> callback);
    
    // Reporting
    double GetTotalRevenue(uint64_t fromTimestamp = 0, uint64_t toTimestamp = 0);
    std::vector<Transaction> GetTransactionsByStatus(TransactionStatus status);
    std::unordered_map<PaymentMethod, int> GetPaymentMethodStats();
    
    // Compliance and audit
    std::vector<Transaction> GetAuditTrail(const std::string& userId);
    bool ExportTransactionData(const std::string& filePath, uint64_t fromTimestamp, uint64_t toTimestamp);
    
    // Emergency functions
    bool FreezeUserAccount(const std::string& userId);
    bool UnfreezeUserAccount(const std::string& userId);
    void TriggerFraudAlert(const std::string& userId, const std::string& reason);
};