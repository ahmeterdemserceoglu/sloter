package com.slotmachine.game;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;
import android.widget.Toast;

public class MainActivity extends Activity {
    
    // Native library
    static {
        System.loadLibrary("SlotMachine");
    }
    
    // Native methods
    public native void nativeOnCreate();
    public native void nativeOnDestroy();
    public native void nativeOnPause();
    public native void nativeOnResume();
    public native void nativeOnTouch(float x, float y, int action);
    public native void nativeOnKeyPress(int keyCode);
    
    private GameView gameView;
    private SecurityManager securityManager;
    private PaymentManager paymentManager;
    
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        
        // Set full screen and landscape
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                           WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        
        // Hide navigation bar
        getWindow().getDecorView().setSystemUiVisibility(
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
            View.SYSTEM_UI_FLAG_FULLSCREEN |
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        );
        
        // Initialize security
        securityManager = new SecurityManager(this);
        if (!securityManager.initialize()) {
            Toast.makeText(this, "Security initialization failed", Toast.LENGTH_LONG).show();
            finish();
            return;
        }
        
        // Initialize payment system
        paymentManager = new PaymentManager(this);
        paymentManager.initialize();
        
        // Create game view
        gameView = new GameView(this);
        setContentView(gameView);
        
        // Initialize native code
        nativeOnCreate();
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        
        if (securityManager != null) {
            securityManager.shutdown();
        }
        
        if (paymentManager != null) {
            paymentManager.shutdown();
        }
        
        nativeOnDestroy();
    }
    
    @Override
    protected void onPause() {
        super.onPause();
        
        if (gameView != null) {
            gameView.onPause();
        }
        
        nativeOnPause();
    }
    
    @Override
    protected void onResume() {
        super.onResume();
        
        // Security check on resume
        if (securityManager != null && !securityManager.performSecurityCheck()) {
            Toast.makeText(this, "Security violation detected", Toast.LENGTH_LONG).show();
            finish();
            return;
        }
        
        if (gameView != null) {
            gameView.onResume();
        }
        
        nativeOnResume();
    }
    
    @Override
    public void onBackPressed() {
        // Handle back button - show exit confirmation
        showExitConfirmation();
    }
    
    private void showExitConfirmation() {
        // Show exit confirmation dialog
        new android.app.AlertDialog.Builder(this)
            .setTitle("Exit Game")
            .setMessage("Are you sure you want to exit?")
            .setPositiveButton("Yes", (dialog, which) -> finish())
            .setNegativeButton("No", null)
            .show();
    }
    
    // Payment callbacks
    public void onPaymentSuccess(String transactionId, double amount) {
        runOnUiThread(() -> {
            Toast.makeText(this, "Payment successful: $" + amount, Toast.LENGTH_SHORT).show();
            // Update game balance via JNI
            nativeOnPaymentSuccess(transactionId, amount);
        });
    }
    
    public void onPaymentFailed(String error) {
        runOnUiThread(() -> {
            Toast.makeText(this, "Payment failed: " + error, Toast.LENGTH_LONG).show();
            nativeOnPaymentFailed(error);
        });
    }
    
    // Security callbacks
    public void onSecurityViolation(String violation) {
        runOnUiThread(() -> {
            Toast.makeText(this, "Security violation: " + violation, Toast.LENGTH_LONG).show();
            finish();
        });
    }
    
    // Native callback methods
    public native void nativeOnPaymentSuccess(String transactionId, double amount);
    public native void nativeOnPaymentFailed(String error);
    
    // Utility methods for native code
    public void showToast(String message) {
        runOnUiThread(() -> Toast.makeText(this, message, Toast.LENGTH_SHORT).show());
    }
    
    public void startPaymentActivity(double amount) {
        Intent intent = new Intent(this, PaymentActivity.class);
        intent.putExtra("amount", amount);
        startActivityForResult(intent, 1001);
    }
    
    public void startSettingsActivity() {
        Intent intent = new Intent(this, SettingsActivity.class);
        startActivity(intent);
    }
    
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        
        if (requestCode == 1001) { // Payment activity
            if (resultCode == RESULT_OK && data != null) {
                String transactionId = data.getStringExtra("transactionId");
                double amount = data.getDoubleExtra("amount", 0.0);
                onPaymentSuccess(transactionId, amount);
            } else {
                String error = data != null ? data.getStringExtra("error") : "Payment cancelled";
                onPaymentFailed(error);
            }
        }
    }
}