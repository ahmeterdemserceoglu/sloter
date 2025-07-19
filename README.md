# Professional Slot Machine Game

Gelişmiş güvenlik ve ödeme sistemi ile donatılmış profesyonel slot makinesi oyunu.

## Özellikler

### 🎰 Oyun Özellikleri
- 5 makaralı, 25 ödeme hatlı slot makinesi
- 10 farklı sembol (Cherry, Lemon, Orange, Plum, Bell, Bar, Seven, Diamond, Wild, Scatter)
- Bonus oyunlar ve jackpot sistemi
- Otomatik spin özelliği
- Gerçekçi animasyonlar ve ses efektleri

### 🔒 Güvenlik Sistemi
- **Anti-Cheat Koruması**: Hız hilesi, bellek manipülasyonu tespiti
- **Cihaz Güvenliği**: Root, emülatör, debugger tespiti
- **Rate Limiting**: Hızlı işlem koruması
- **Fraud Detection**: Şüpheli aktivite tespiti
- **Memory Protection**: Bellek bütünlüğü kontrolü

### 💳 Ödeme Sistemi
- Çoklu ödeme yöntemi desteği (Kredi kartı, PayPal, Google Pay, Apple Pay)
- Güvenli cüzdan sistemi
- Günlük harcama limitleri
- Fraud detection ve risk yönetimi
- PCI DSS uyumlu güvenlik

### 🗄️ Veritabanı Sistemi
- **PostgreSQL** entegrasyonu
- Connection pooling ve prepared statements
- Transaction yönetimi ve audit logging
- Otomatik backup ve maintenance
- Güvenli veri şifreleme

### 🔐 Authentication Sistemi
- **JWT token** tabanlı kimlik doğrulama
- **Two-factor authentication** (2FA)
- Session yönetimi ve refresh tokens
- Password policy enforcement
- Email verification ve rate limiting

### 📱 Platform Desteği
- Android (API 21+)
- ARM64, ARMv7, x86, x86_64 mimarileri
- OpenGL ES 2.0 grafik desteği
- Dokunmatik ekran optimizasyonu

## Kurulum Gereksinimleri

### Ubuntu/Linux Geliştirme Ortamı
1. **Ubuntu 20.04+** (önerilen)
2. **PostgreSQL 12+** (veritabanı)
3. **CMake 3.18+** (build sistemi)
4. **GCC 9+** (C++ derleyici)
5. **OpenSSL** (güvenlik kütüphaneleri)
6. **libpq-dev** (PostgreSQL geliştirme kütüphaneleri)
7. **libjsoncpp-dev** (JSON işleme)
8. **libcurl4-openssl-dev** (HTTP istekleri)

### Android Geliştirme Ortamı (Opsiyonel)
1. **Android Studio** (4.2 veya üzeri)
2. **Android SDK** (API 21-34)
3. **Android NDK** (25.2.9519653)
4. **Java JDK** (8 veya üzeri)

### Sistem Gereksinimleri
- Ubuntu 20.04+ (64-bit)
- 4 GB RAM (minimum, 8 GB önerilen)
- 5 GB boş disk alanı
- PostgreSQL için ek 1 GB

## Ubuntu Kurulum Adımları

### 1. Sistem Hazırlığı
```bash
# Projeyi klonlayın
git clone [repository-url]
cd SlotMachine

# Bağımlılıkları yükleyin
chmod +x install_dependencies.sh
./install_dependencies.sh
```

### 2. Veritabanı Kurulumu
```bash
# PostgreSQL'i kurun ve yapılandırın
chmod +x database/setup_database.sh
./database/setup_database.sh

# Veritabanı bağlantısını test edin
psql -h localhost -U slotmachine_user -d slotmachine_db
```

### 3. Proje Build
```bash
# Ubuntu için build edin
chmod +x build_ubuntu.sh
./build_ubuntu.sh
```

### 4. Uygulamayı Çalıştırın
```bash
# PostgreSQL'in çalıştığından emin olun
sudo systemctl start postgresql

# Uygulamayı başlatın
./build/SlotMachine
```

## Android Build (Opsiyonel)

### 1. Android Studio Kurulumu
```bash
# Android Studio'yu indirin ve kurun
# https://developer.android.com/studio

# SDK Manager'dan şunları yükleyin:
- Android SDK Platform 34
- Android SDK Build-Tools 34.0.0
- Android NDK (Side by side) 25.2.9519653
- CMake 3.18.1
```

### 2. Android APK Build
```bash
# Android APK'yı build edin (Windows'ta)
build_android.bat
```

## Proje Yapısı

```
SlotMachine/
├── src/
│   ├── core/              # Oyun motoru
│   │   ├── GameEngine.h
│   │   └── GameEngine.cpp
│   ├── security/          # Güvenlik sistemi
│   │   ├── SecurityManager.h
│   │   └── SecurityManager.cpp
│   ├── payment/           # Ödeme sistemi
│   │   ├── PaymentSystem.h
│   │   └── PaymentSystem.cpp
│   ├── ui/               # Kullanıcı arayüzü
│   │   ├── GameUI.h
│   │   └── GameUI.cpp
│   └── main.cpp          # Ana uygulama
├── android/              # Android projesi
│   ├── app/
│   │   ├── build.gradle
│   │   └── src/main/
│   │       ├── AndroidManifest.xml
│   │       └── java/com/slotmachine/game/
├── assets/               # Oyun varlıkları
├── build/                # Build dosyaları
├── CMakeLists.txt        # CMake yapılandırması
├── build_android.bat     # Build scripti
└── README.md
```

## Güvenlik Özellikleri

### Anti-Cheat Sistemi
- **Memory Integrity**: Bellek manipülasyonu tespiti
- **Speed Hack Detection**: Hız hilesi koruması
- **Pattern Analysis**: Şüpheli oyun kalıpları tespiti
- **Rate Limiting**: İşlem hızı sınırlaması

### Cihaz Güvenliği
- **Root Detection**: Root edilmiş cihaz tespiti
- **Emulator Detection**: Emülatör tespiti
- **Debugger Detection**: Debugger bağlantısı tespiti
- **Device Fingerprinting**: Cihaz parmak izi oluşturma

### Ödeme Güvenliği
- **Encryption**: Hassas veri şifreleme
- **Fraud Detection**: Dolandırıcılık tespiti
- **Transaction Monitoring**: İşlem izleme
- **Secure Storage**: Güvenli veri saklama

## API Referansı

### GameEngine
```cpp
// Oyun döndürme
SpinResult Spin(double betAmount);

// Bakiye yönetimi
void SetBalance(double balance);
double GetBalance() const;

// Güvenlik kontrolü
bool IsSecure() const;
```

### SecurityManager
```cpp
// Ana güvenlik kontrolü
bool PerformSecurityCheck();

// Cihaz güvenliği
bool IsDeviceSecure() const;

// Güvenlik ihlali raporu
void ReportSecurityBreach(const std::string& details);
```

### PaymentSystem
```cpp
// Ödeme işlemi
std::string ProcessPayment(const std::string& userId, double amount, 
                          const PaymentInfo& paymentInfo);

// Bakiye yönetimi
bool AddFunds(const std::string& userId, double amount);
double GetBalance(const std::string& userId);
```

## Performans Optimizasyonu

### Grafik Optimizasyonu
- OpenGL ES 2.0 kullanımı
- Texture atlasing
- Batch rendering
- Frustum culling

### Bellek Yönetimi
- Object pooling
- Smart pointer kullanımı
- Memory leak prevention
- Garbage collection optimization

## Test ve Debug

### Unit Testler
```bash
# C++ testlerini çalıştır
cd build
ctest --verbose
```

### Android Testler
```bash
# Android testlerini çalıştır
cd android
./gradlew test
./gradlew connectedAndroidTest
```

### Debug Build
```bash
# Debug APK oluştur
cd android
./gradlew assembleDebug
```

## Deployment

### Release Build
```bash
# Release APK oluştur
build_android.bat

# APK imzalama (production için)
jarsigner -verbose -sigalg SHA1withRSA -digestalg SHA1 -keystore my-release-key.keystore app-release-unsigned.apk alias_name
zipalign -v 4 app-release-unsigned.apk app-release.apk
```

### Play Store Yayını
1. Google Play Console'da uygulama oluşturun
2. APK'yı yükleyin
3. Store listing bilgilerini doldurun
4. İçerik derecelendirmesi alın
5. Uygulamayı yayınlayın

## Sorun Giderme

### Yaygın Sorunlar

**Build Hatası: NDK bulunamadı**
```bash
# NDK yolunu kontrol edin
echo %ANDROID_NDK_HOME%

# NDK'yı yeniden yükleyin
# Android Studio > SDK Manager > SDK Tools > NDK
```

**Gradle Build Hatası**
```bash
# Gradle cache'i temizleyin
cd android
./gradlew clean

# Gradle wrapper'ı güncelleyin
./gradlew wrapper --gradle-version 7.6
```

**APK Yükleme Hatası**
```bash
# USB Debugging etkin mi kontrol edin
adb devices

# APK'yı yeniden yükleyin
adb uninstall com.slotmachine.game
adb install app-release.apk
```

## Lisans

Bu proje MIT lisansı altında lisanslanmıştır. Detaylar için LICENSE dosyasına bakın.

## Destek

Teknik destek için:
- GitHub Issues: [repository-url]/issues
- Email: support@slotmachine.com
- Dokümantasyon: [docs-url]

## Katkıda Bulunma

1. Fork edin
2. Feature branch oluşturun (`git checkout -b feature/AmazingFeature`)
3. Commit edin (`git commit -m 'Add some AmazingFeature'`)
4. Push edin (`git push origin feature/AmazingFeature`)
5. Pull Request oluşturun