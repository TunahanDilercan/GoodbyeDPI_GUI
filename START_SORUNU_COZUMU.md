# 🚨 GoodbyeDPI Start Problemi Çözümü

## ❌ Tespit Edilen Sorun:
**StartService failed: 5 (ERROR_ACCESS_DENIED)**
- WinDivert sürücüsü Windows tarafından imzalanmadığı için yüklenmiyor
- Windows 10/11'de sürücü imza doğrulaması sorunu

## ✅ Çözüm Yöntemleri:

### Yöntem 1: Test Mode (Önerilen) 🏆
```batch
# Yönetici olarak cmd açın ve çalıştırın:
bcdedit /set testsigning on
# Bilgisayarı yeniden başlatın
```

### Yöntem 2: Geçici İmza Devre Dışı
1. **Bilgisayarı yeniden başlatın**
2. **F8 tuşuna basarak Advanced Boot Options açın**
3. **"Disable Driver Signature Enforcement" seçin**
4. **GoodbyeDPI'ı çalıştırın**

### Yöntem 3: PowerShell (Windows 11)
```powershell
# Yönetici PowerShell'de:
bcdedit /set nointegritychecks on
# Yeniden başlatın
```

## 🔧 Otomatik Çözüm:
1. `enable_testmode.bat` dosyasını yönetici olarak çalıştırın
2. Bilgisayarı yeniden başlatın
3. GoodbyeDPI'ı tekrar deneyin

## 📋 Test Adımları:
1. ✅ Program başlatıldı
2. ✅ GUI yüklendi  
3. ✅ Menü oluşturuldu
4. ✅ Autostart ayarlandı
5. ❌ **WinDivert driver yüklenemedi** ← Sorun burası

## 🎯 Sonuç:
Program çalışıyor ancak WinDivert sürücüsü imza problemi yüzünden başlatılamıyor. Yukarıdaki çözümlerden birini uygulayın.

## ⚠️ Dikkat:
Test mode etkinleştirildiğinde Windows'ta küçük bir "Test Mode" yazısı görünür. Bu normal ve zararsızdır.
