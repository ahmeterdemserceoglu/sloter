@echo off
echo Building Slot Machine Android APK...

REM Check if Android SDK is installed
if not exist "%ANDROID_HOME%" (
    echo ERROR: ANDROID_HOME environment variable not set
    echo Please install Android Studio and set ANDROID_HOME
    pause
    exit /b 1
)

REM Check if NDK is installed
if not exist "%ANDROID_HOME%\ndk" (
    echo ERROR: Android NDK not found
    echo Please install Android NDK through Android Studio
    pause
    exit /b 1
)

REM Set NDK path
set ANDROID_NDK_HOME=%ANDROID_HOME%\ndk\25.2.9519653

REM Create build directory
if not exist "build" mkdir build
cd build

REM Configure CMake for Android
cmake .. ^
    -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK_HOME%\build\cmake\android.toolchain.cmake ^
    -DANDROID_ABI=arm64-v8a ^
    -DANDROID_PLATFORM=android-21 ^
    -DANDROID_STL=c++_shared ^
    -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed
    pause
    exit /b 1
)

REM Build native library
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo Native build failed
    pause
    exit /b 1
)

cd ..

REM Copy native library to Android project
if not exist "android\app\src\main\jniLibs\arm64-v8a" mkdir android\app\src\main\jniLibs\arm64-v8a
copy build\libSlotMachine.so android\app\src\main\jniLibs\arm64-v8a\

REM Build Android APK
cd android
call gradlew assembleRelease

if %ERRORLEVEL% neq 0 (
    echo Android build failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo BUILD SUCCESSFUL!
echo ========================================
echo APK Location: android\app\build\outputs\apk\release\app-release.apk
echo.
echo To install on device:
echo adb install android\app\build\outputs\apk\release\app-release.apk
echo.
pause