#include "PaymentSystem.h"
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <fstream>

#ifdef ANDROID
#include <android/log.h>
#define LOG_TAG "PaymentSystem"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__)
#define LOGW(...) printf(__VA_ARGS__)
#define LOGE(...) printf(__VA_ARGS__)
#endif

PaymentSystem::PaymentSystem() {
    // Initialize encryption key (in production, this would be securely generated)
    encryptionKey = "SecureKey123!@#$%^&*()_+";
    
    // Initialize fraud detection patterns
    fraudPatterns = {
        "rapid_transactions",
        "unusual_amounts",
        "multiple_failed_attempts",
        "suspicious_locations"
    };
    
    // Initialize payment gateway endpoints (mock URLs)
    gatewayEndpoints[PaymentMethod::CREDIT_CARD] = "https://api.stripe.com/v1/charges";
    gatewayEndpoints[PaymentMethod::PAYPAL] = "https://api.paypal.com/v1/payments";
    gatewayEndpoints[PaymentMethod::GOOGLE_PAY] = "https://pay.google.com/api/v1/process";
    gatewayEndpoints[PaymentMethod::APPLE_PAY] = "https://apple-pay-gateway.apple.com/paymentservices";
    
    // Initialize gateway status (all available by default)
    for (auto& [method, endpoint] : gatewayEndpoints) {
        gatewayStatus[method] = true;
    }
}

PaymentSystem::~PaymentSystem() {
    Shutdown();
}

bool PaymentSystem::Initialize() {
    LOGI("Initializing Payment System...\n");
    
    // Initialize default spend limits
    dailySpendLimits["default"] = 1000.0; // $1000 default daily limit
    
    LOGI("Payment System initialized successfully\n");
    return true;
}

void PaymentSystem::Shutdown() {
    LOGI("Shutting down Payment System...\n");
    
    // Clear sensitive data
    userWallets.clear();
    transactionHistory.clear();
    pendingTransactions.clear();
    userTransactionTimes.clear();
    dailySpendLimits.clear();
    
    // Clear encryption key
    encryptionKey.clear();
}

bool PaymentSystem::CreateWallet(const std::string& userId, Currency currency) {
    if (userWallets.find(userId) != userWallets.end()) {
        LOGW("Wallet already exists for user: %s\n", userId.c_str());
        return false;
    }
    
    WalletBalance wallet;
    wallet.balance = 0.0;
    wallet.currency = currency;
    wallet.lastUpdated = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    userWallets[userId] = wallet;
    
    LOGI("Created wallet for user: %s\n", userId.c_str());
    return true;
}

double PaymentSystem::GetBalance(const std::string& userId, Currency currency) {
    auto it = userWallets.find(userId);
    if (it == userWallets.end()) {
        return 0.0;
    }
    
    if (it->second.currency != currency) {
        // Convert currency if needed
        return ConvertCurrency(it->second.balance, it->second.currency, currency);
    }
    
    return it->second.balance;
}

bool PaymentSystem::AddFunds(const std::string& userId, double amount, const PaymentInfo& paymentInfo) {
    if (amount <= 0) {
        LOGE("Invalid amount for adding funds: %f\n", amount);
        return false;
    }
    
    // Validate payment info
    if (!ValidatePaymentInfo(paymentInfo)) {
        LOGE("Invalid payment information\n");
        return false;
    }
    
    // Fraud detection
    if (DetectFraud(userId, amount)) {
        LOGE("Fraud detected for user: %s, amount: %f\n", userId.c_str(), amount);
        TriggerFraudAlert(userId, "Suspicious add funds transaction");
        return false;
    }
    
    // Create transaction
    Transaction transaction;
    transaction.transactionId = GenerateTransactionId();
    transaction.userId = userId;
    transaction.amount = amount;
    transaction.currency = Currency::USD; // Default currency
    transaction.method = paymentInfo.method;
    transaction.status = TransactionStatus::PENDING;
    transaction.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    transaction.description = "Add funds to wallet";
    transaction.isRefundable = true;
    
    // Process with payment gateway
    if (ProcessWithGateway(transaction, paymentInfo)) {
        // Add funds to wallet
        auto& wallet = userWallets[userId];
        wallet.balance += amount;
        wallet.lastUpdated = transaction.timestamp;
        wallet.recentTransactions.push_back(transaction);
        
        // Keep only last 10 transactions in wallet
        if (wallet.recentTransactions.size() > 10) {
            wallet.recentTransactions.erase(wallet.recentTransactions.begin());
        }
        
        transaction.status = TransactionStatus::COMPLETED;
        transactionHistory.push_back(transaction);
        
        if (onTransactionComplete) {
            onTransactionComplete(transaction);
        }
        
        LOGI("Added funds successfully: User=%s, Amount=%f\n", userId.c_str(), amount);
        return true;
    } else {
        transaction.status = TransactionStatus::FAILED;
        transactionHistory.push_back(transaction);
        
        if (onTransactionFailed) {
            onTransactionFailed(transaction);
        }
        
        LOGE("Failed to add funds: User=%s, Amount=%f\n", userId.c_str(), amount);
        return false;
    }
}

bool PaymentSystem::DeductFunds(const std::string& userId, double amount) {
    if (amount <= 0) {
        return false;
    }
    
    auto it = userWallets.find(userId);
    if (it == userWallets.end() || it->second.balance < amount) {
        LOGE("Insufficient funds for user: %s, requested: %f, available: %f\n", 
             userId.c_str(), amount, (it != userWallets.end()) ? it->second.balance : 0.0);
        return false;
    }
    
    // Create deduction transaction
    Transaction transaction;
    transaction.transactionId = GenerateTransactionId();
    transaction.userId = userId;
    transaction.amount = -amount; // Negative for deduction
    transaction.currency = Currency::USD;
    transaction.method = PaymentMethod::DEBIT_CARD; // Internal deduction
    transaction.status = TransactionStatus::COMPLETED;
    transaction.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    transaction.description = "Funds deducted for game play";
    transaction.isRefundable = false;
    
    // Deduct funds
    it->second.balance -= amount;
    it->second.lastUpdated = transaction.timestamp;
    it->second.recentTransactions.push_back(transaction);
    
    transactionHistory.push_back(transaction);
    
    LOGI("Deducted funds: User=%s, Amount=%f, Remaining=%f\n", 
         userId.c_str(), amount, it->second.balance);
    return true;
}

std::string PaymentSystem::ProcessPayment(const std::string& userId, double amount, 
                                        const PaymentInfo& paymentInfo, const std::string& description) {
    // Validate transaction
    if (!ValidateTransaction(userId, amount)) {
        return "";
    }
    
    // Create transaction
    Transaction transaction;
    transaction.transactionId = GenerateTransactionId();
    transaction.userId = userId;
    transaction.amount = amount;
    transaction.currency = Currency::USD;
    transaction.method = paymentInfo.method;
    transaction.status = TransactionStatus::PENDING;
    transaction.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    transaction.description = description;
    transaction.isRefundable = true;
    
    pendingTransactions[transaction.transactionId] = transaction;
    
    // Process asynchronously (in real implementation)
    if (ProcessWithGateway(transaction, paymentInfo)) {
        UpdateTransactionStatus(transaction.transactionId, TransactionStatus::COMPLETED);
        return transaction.transactionId;
    } else {
        UpdateTransactionStatus(transaction.transactionId, TransactionStatus::FAILED);
        return "";
    }
}

bool PaymentSystem::ValidatePaymentInfo(const PaymentInfo& paymentInfo) {
    // Basic validation
    if (paymentInfo.cardNumber.empty() || paymentInfo.holderName.empty()) {
        return false;
    }
    
    // Card number validation (simplified Luhn algorithm check)
    if (paymentInfo.method == PaymentMethod::CREDIT_CARD || 
        paymentInfo.method == PaymentMethod::DEBIT_CARD) {
        
        std::string cardNum = paymentInfo.cardNumber;
        cardNum.erase(std::remove_if(cardNum.begin(), cardNum.end(), ::isspace), cardNum.end());
        
        if (cardNum.length() < 13 || cardNum.length() > 19) {
            return false;
        }
        
        // Simple Luhn algorithm
        int sum = 0;
        bool alternate = false;
        for (int i = cardNum.length() - 1; i >= 0; i--) {
            int digit = cardNum[i] - '0';
            if (alternate) {
                digit *= 2;
                if (digit > 9) digit -= 9;
            }
            sum += digit;
            alternate = !alternate;
        }
        
        if (sum % 10 != 0) {
            return false;
        }
    }
    
    return true;
}

bool PaymentSystem::DetectFraud(const std::string& userId, double amount) {
    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    // Check transaction frequency
    auto& userTimes = userTransactionTimes[userId];
    userTimes.push_back(currentTime);
    
    // Remove transactions older than 1 hour
    userTimes.erase(std::remove_if(userTimes.begin(), userTimes.end(),
        [currentTime](uint64_t timestamp) {
            return currentTime - timestamp > 3600000; // 1 hour
        }), userTimes.end());
    
    // Check for rapid transactions (more than 10 in 1 hour)
    if (userTimes.size() > 10) {
        LOGW("Rapid transactions detected for user: %s\n", userId.c_str());
        return true;
    }
    
    // Check for unusual amounts
    if (amount > 5000.0) { // More than $5000
        LOGW("Large transaction amount detected: %f\n", amount);
        return true;
    }
    
    // Check daily spend limit
    double dailySpent = GetDailySpent(userId);
    double dailyLimit = GetDailySpendLimit(userId);
    
    if (dailySpent + amount > dailyLimit) {
        LOGW("Daily spend limit exceeded for user: %s\n", userId.c_str());
        return true;
    }
    
    return false;
}

bool PaymentSystem::ProcessWithGateway(Transaction& transaction, const PaymentInfo& paymentInfo) {
    // Mock payment gateway processing
    // In a real implementation, this would make HTTP requests to payment gateways
    
    if (!IsGatewayAvailable(paymentInfo.method)) {
        transaction.gatewayResponse = "Gateway unavailable";
        return false;
    }
    
    // Simulate processing time and success rate
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    int successRate = 95; // 95% success rate
    bool success = dis(gen) <= successRate;
    
    if (success) {
        transaction.gatewayResponse = "Payment processed successfully";
        transaction.merchantReference = "MERCH_" + GenerateTransactionId();
        return true;
    } else {
        transaction.gatewayResponse = "Payment declined by issuer";
        return false;
    }
}

std::string PaymentSystem::GenerateTransactionId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "TXN_";
    
    for (int i = 0; i < 16; i++) {
        ss << std::hex << dis(gen);
    }
    
    return ss.str();
}

double PaymentSystem::ConvertCurrency(double amount, Currency from, Currency to) {
    // Mock currency conversion rates
    std::unordered_map<Currency, double> usdRates = {
        {Currency::USD, 1.0},
        {Currency::EUR, 0.85},
        {Currency::GBP, 0.73},
        {Currency::JPY, 110.0},
        {Currency::CAD, 1.25},
        {Currency::AUD, 1.35}
    };
    
    if (from == to) return amount;
    
    // Convert to USD first, then to target currency
    double usdAmount = amount / usdRates[from];
    return usdAmount * usdRates[to];
}

bool PaymentSystem::ValidateTransaction(const std::string& userId, double amount) {
    if (amount <= 0) return false;
    if (userId.empty()) return false;
    
    // Check if user wallet exists
    if (userWallets.find(userId) == userWallets.end()) {
        CreateWallet(userId);
    }
    
    return !DetectFraud(userId, amount);
}

void PaymentSystem::SetDailySpendLimit(const std::string& userId, double limit) {
    dailySpendLimits[userId] = limit;
}

double PaymentSystem::GetDailySpendLimit(const std::string& userId) {
    auto it = dailySpendLimits.find(userId);
    if (it != dailySpendLimits.end()) {
        return it->second;
    }
    return dailySpendLimits["default"];
}

double PaymentSystem::GetDailySpent(const std::string& userId) {
    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    
    auto dayStart = currentTime - (currentTime % 86400000); // Start of current day
    
    double totalSpent = 0.0;
    for (const auto& transaction : transactionHistory) {
        if (transaction.userId == userId && 
            transaction.timestamp >= dayStart && 
            transaction.amount > 0 &&
            transaction.status == TransactionStatus::COMPLETED) {
            totalSpent += transaction.amount;
        }
    }
    
    return totalSpent;
}

bool PaymentSystem::IsGatewayAvailable(PaymentMethod method) {
    auto it = gatewayStatus.find(method);
    return it != gatewayStatus.end() && it->second;
}

void PaymentSystem::UpdateTransactionStatus(const std::string& transactionId, TransactionStatus status) {
    auto it = pendingTransactions.find(transactionId);
    if (it != pendingTransactions.end()) {
        it->second.status = status;
        transactionHistory.push_back(it->second);
        pendingTransactions.erase(it);
    }
}

void PaymentSystem::TriggerFraudAlert(const std::string& userId, const std::string& reason) {
    LOGE("FRAUD ALERT: User=%s, Reason=%s\n", userId.c_str(), reason.c_str());
    
    // In a real implementation, this would:
    // - Send alert to fraud monitoring system
    // - Temporarily freeze the account
    // - Log detailed information for investigation
}

Transaction PaymentSystem::GetTransaction(const std::string& transactionId) {
    // Check pending transactions first
    auto pendingIt = pendingTransactions.find(transactionId);
    if (pendingIt != pendingTransactions.end()) {
        return pendingIt->second;
    }
    
    // Check transaction history
    for (const auto& transaction : transactionHistory) {
        if (transaction.transactionId == transactionId) {
            return transaction;
        }
    }
    
    // Return empty transaction if not found
    return Transaction{};
}

std::vector<Transaction> PaymentSystem::GetUserTransactions(const std::string& userId, int limit) {
    std::vector<Transaction> userTransactions;
    
    for (const auto& transaction : transactionHistory) {
        if (transaction.userId == userId) {
            userTransactions.push_back(transaction);
        }
    }
    
    // Sort by timestamp (newest first)
    std::sort(userTransactions.begin(), userTransactions.end(),
        [](const Transaction& a, const Transaction& b) {
            return a.timestamp > b.timestamp;
        });
    
    // Limit results
    if (limit > 0 && userTransactions.size() > static_cast<size_t>(limit)) {
        userTransactions.resize(limit);
    }
    
    return userTransactions;
}