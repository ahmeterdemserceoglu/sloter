#include "core/GameEngine.h"
#include "ui/GameUI.h"
#include "security/SecurityManager.h"
#include "payment/PaymentSystem.h"
#ifdef USE_DATABASE
#include "database/DatabaseManager.h"
#include "auth/AuthManager.h"
#endif
#include <iostream>
#include <memory>

#ifdef ANDROID
#include <android/log.h>
#include <android_native_app_glue.h>
#define LOG_TAG "SlotMachine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__)
#endif

class SlotMachineApp {
private:
    std::unique_ptr<GameEngine> gameEngine;
    std::unique_ptr<GameUI> gameUI;
    std::unique_ptr<SecurityManager> security;
    std::unique_ptr<PaymentSystem> payment;
#ifdef USE_DATABASE
    std::shared_ptr<DatabaseManager> database;
    std::unique_ptr<AuthManager> auth;
#endif
    bool isRunning;

public:
    SlotMachineApp() : isRunning(false) {}

    bool Initialize() {
        LOGI("Initializing Slot Machine App...\n");
        
#ifdef USE_DATABASE
        // Initialize database first
        database = std::make_shared<DatabaseManager>();
        if (!database->Initialize()) {
            LOGI("Failed to initialize database system\n");
            return false;
        }

        // Initialize authentication system
        auth = std::make_unique<AuthManager>(database);
        if (!auth->Initialize()) {
            LOGI("Failed to initialize authentication system\n");
            return false;
        }
#endif

        // Initialize security system
        security = std::make_unique<SecurityManager>();
        if (!security->Initialize()) {
            LOGI("Failed to initialize security system\n");
            return false;
        }

        // Initialize payment system
        payment = std::make_unique<PaymentSystem>();
        if (!payment->Initialize()) {
            LOGI("Failed to initialize payment system\n");
            return false;
        }

        // Initialize game engine
        gameEngine = std::make_unique<GameEngine>();
        if (!gameEngine->Initialize()) {
            LOGI("Failed to initialize game engine\n");
            return false;
        }

        // Initialize UI
        gameUI = std::make_unique<GameUI>();
        if (!gameUI->Initialize()) {
            LOGI("Failed to initialize game UI\n");
            return false;
        }

        isRunning = true;
        LOGI("Slot Machine App initialized successfully!\n");
        return true;
    }

    void Run() {
        while (isRunning) {
            // Security checks
            if (!security->PerformSecurityCheck()) {
                LOGI("Security violation detected! Shutting down...\n");
                break;
            }

            // Update game logic
            gameEngine->Update();
            
            // Render UI
            gameUI->Render();
            
            // Handle input
            HandleInput();
        }
    }

    void HandleInput() {
        // Input handling will be implemented based on platform
        // Android: Touch events
        // Desktop: Keyboard/Mouse events
    }

    void Shutdown() {
        LOGI("Shutting down Slot Machine App...\n");
        isRunning = false;
        
        if (gameUI) gameUI->Shutdown();
        if (gameEngine) gameEngine->Shutdown();
        if (payment) payment->Shutdown();
        if (security) security->Shutdown();
#ifdef USE_DATABASE
        if (auth) auth->Shutdown();
        if (database) database->Shutdown();
#endif
    }
};

#ifdef ANDROID
void android_main(struct android_app* state) {
    SlotMachineApp app;
    
    if (app.Initialize()) {
        app.Run();
    }
    
    app.Shutdown();
}
#else
int main() {
    SlotMachineApp app;
    
    if (app.Initialize()) {
        app.Run();
    }
    
    app.Shutdown();
    return 0;
}
#endif