#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>

#ifdef ANDROID
#include <GLES2/gl2.h>
#include <android/asset_manager.h>
#else
#include <GL/gl.h>
#endif

struct Color {
    float r, g, b, a;
    Color(float red = 1.0f, float green = 1.0f, float blue = 1.0f, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}
};

struct Vector2 {
    float x, y;
    Vector2(float x = 0.0f, float y = 0.0f) : x(x), y(y) {}
};

struct Rect {
    float x, y, width, height;
    Rect(float x = 0.0f, float y = 0.0f, float w = 0.0f, float h = 0.0f)
        : x(x), y(y), width(w), height(h) {}
};

enum class UIElement {
    BUTTON = 0,
    LABEL,
    SLOT_REEL,
    BALANCE_DISPLAY,
    BET_SELECTOR,
    WIN_DISPLAY,
    MENU_PANEL,
    SETTINGS_PANEL
};

struct UIButton {
    Rect bounds;
    std::string text;
    Color backgroundColor;
    Color textColor;
    bool isPressed;
    bool isEnabled;
    std::function<void()> onClick;
    GLuint textureId;
};

struct SlotReel {
    Rect bounds;
    std::vector<int> symbols;
    float spinSpeed;
    float currentOffset;
    bool isSpinning;
    GLuint symbolTextures[10]; // Max 10 different symbols
};

struct UILabel {
    Vector2 position;
    std::string text;
    Color color;
    float fontSize;
    bool isVisible;
};

class GameUI {
private:
    // Screen dimensions
    int screenWidth;
    int screenHeight;
    float aspectRatio;
    
    // UI Elements
    std::vector<UIButton> buttons;
    std::vector<UILabel> labels;
    std::vector<SlotReel> reels;
    
    // Game state display
    UILabel balanceLabel;
    UILabel betLabel;
    UILabel winLabel;
    UIButton spinButton;
    UIButton maxBetButton;
    UIButton autoSpinButton;
    
    // Menu system
    bool isMenuVisible;
    bool isSettingsVisible;
    std::vector<UIButton> menuButtons;
    std::vector<UIButton> settingsButtons;
    
    // Graphics resources
    GLuint shaderProgram;
    GLuint vertexBuffer;
    GLuint indexBuffer;
    std::vector<GLuint> textures;
    
    // Animation system
    struct Animation {
        float duration;
        float currentTime;
        std::function<void(float)> updateFunc;
        std::function<void()> completeFunc;
        bool isActive;
    };
    std::vector<Animation> activeAnimations;
    
    // Touch/Input handling
    Vector2 lastTouchPos;
    bool isTouching;
    
    // Rendering
    void InitializeGraphics();
    void LoadTextures();
    void CreateShaders();
    void RenderButton(const UIButton& button);
    void RenderLabel(const UILabel& label);
    void RenderSlotReel(const SlotReel& reel);
    void RenderBackground();
    void RenderEffects();
    
    // UI Layout
    void LayoutMainUI();
    void LayoutMenuUI();
    void LayoutSettingsUI();
    void UpdateLayout();
    
    // Animation
    void UpdateAnimations(float deltaTime);
    void StartSpinAnimation();
    void StartWinAnimation(double winAmount);
    void StartButtonPressAnimation(UIButton& button);
    
    // Input handling
    bool IsPointInRect(const Vector2& point, const Rect& rect);
    void HandleButtonPress(UIButton& button, const Vector2& touchPos);
    
public:
    GameUI();
    ~GameUI();
    
    bool Initialize();
    void Shutdown();
    void Render();
    void Update(float deltaTime);
    
    // Screen management
    void SetScreenSize(int width, int height);
    void SetOrientation(bool isLandscape);
    
    // Game state updates
    void UpdateBalance(double balance);
    void UpdateBet(double bet);
    void UpdateWin(double winAmount);
    void UpdateReels(const std::vector<std::vector<int>>& reelData);
    
    // UI state
    void ShowMenu(bool show);
    void ShowSettings(bool show);
    void SetSpinButtonEnabled(bool enabled);
    void SetAutoSpinActive(bool active);
    
    // Input events
    void OnTouchDown(float x, float y);
    void OnTouchUp(float x, float y);
    void OnTouchMove(float x, float y);
    void OnKeyPress(int keyCode);
    
    // Callbacks
    void SetSpinCallback(std::function<void()> callback);
    void SetBetChangeCallback(std::function<void(double)> callback);
    void SetMaxBetCallback(std::function<void()> callback);
    void SetAutoSpinCallback(std::function<void(bool)> callback);
    void SetMenuCallback(std::function<void(const std::string&)> callback);
    
    // Visual effects
    void ShowWinEffect(double amount);
    void ShowJackpotEffect();
    void ShowBonusEffect();
    void PlaySpinEffect();
    
    // Accessibility
    void SetFontSize(float size);
    void SetHighContrast(bool enabled);
    void SetColorBlindMode(bool enabled);
};