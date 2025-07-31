# GoodbyeDPI GUI v2.3.4

**Türkiye için özelleştirilmiş DPI atlama GUI uygulaması**

## Özellikler
- ✅ **System Tray Arayüzü** - Sistem tepsisinde çalışır
- ✅ **10 Farklı Script** - Türkiye ve diğer ülkeler için optimize edilmiş
- ✅ **Otomatik Başlatma** - Windows başlangıcında otomatik çalışma
- ✅ **JSON Konfigürasyon** - Kullanıcı dostu ayar kaydetme
- ✅ **Thread-Safe** - Güvenli script değiştirme
- ✅ **WinDivert Otomatik Yönetimi** - Manuel servis yönetimi gerektirmez

## Sistem Gereksinimleri
- Windows 7/8/10/11 (64-bit)
- **Yönetici yetkileri** gereklidir
- Test modu etkinleştirilmeli (ilk çalıştırmada otomatik)

## Kurulum ve Kullanım

### 1. İlk Kurulum
1. **goodbyedpi_gui.exe** dosyasını istediğiniz klasöre çıkarın
2. **Yönetici olarak çalıştırın** (sağ tık → "Yönetici olarak çalıştır")
3. İlk çalıştırmada test modu otomatik etkinleştirilir
4. Sistem tepsisine ikon eklenir

### 2. Kullanım
1. **System Tray İkonu** üzerine **sağ tık**
2. Menüden istediğiniz **script'i seçin**:
   - **[TR] Alternative 4** - Varsayılan seçim (önerilen)
   - **[TR] Alternative 3** - TTL tabanlı
   - **[TR] DNS Redirect** - DNS yönlendirmeli
   - Diğer alternatifler...
3. Script seçtikten sonra **otomatik başlar**
4. **Stop** ile durdurabilirsiniz

### 3. Script Açıklamaları
- **[TR] Alternative 4**: `-5` modu, DNS yönlendirme (77.88.8.8:1253)
- **[TR] Alternative 3**: `--set-ttl 3`, DNS yönlendirme
- **[TR] Alternative 2**: `-2` modu, DNS yönlendirme  
- **[TR] DNS Redirect**: Basit DNS yönlendirme
- **Russia Blacklist**: Rusya blacklist'i ile çalışır
- **Any Country**: Genel amaçlı modlar

## Önemli Notlar

### ⚠️ Güvenlik
- **Mutlaka yönetici olarak çalıştırın**
- Antivirüs programları false-positive verebilir (normal)
- Windows Defender'da istisna eklemeniz gerekebilir

### 🔧 Sorun Giderme
- **"WinDivert bulunamadı"** → Dosyaları aynı klasörde tutun
- **"Test modu etkinleştirilemedi"** → Manuel: `bcdedit /set testsigning on`
- **Script değiştirme sorunu** → 3 saniye bekleyip tekrar deneyin
- **Çökme problemi** → Log dosyalarını kontrol edin

### 📋 Log Dosyaları
- `goodbyedpi_log.txt` - Normal işlemler
- `goodbyedpi_error.log` - Hata mesajları
- `last_selected.json` - Son seçilen script

## Sürüm Geçmişi

### v2.3.4 (31 Temmuz 2025)
- ✅ Stop fonksiyonu düzeltildi
- ✅ Script switching crash sorunu çözüldü
- ✅ Thread-safe menü yönetimi
- ✅ JSON config sistemi eklendi
- ✅ Registry bağımlılığı kaldırıldı
- ✅ Race condition önlemleri
- ✅ İyileştirilmiş hata handling

### v2.3.3
- ✅ Temel GUI implementasyonu
- ✅ System tray entegrasyonu
- ✅ 10 script desteği

## Teknik Detaylar
- **Derleme**: MinGW-w64 (GCC)
- **UI Framework**: Win32 API
- **Thread Model**: Single-threaded GUI, multi-threaded DPI engine
- **Config Format**: JSON (utf-8)
- **Driver**: WinDivert 2.2

## Lisans
Bu proje orijinal GoodbyeDPI projesinin GUI versiyonudur.
- Orijinal proje: https://github.com/ValdikSS/GoodbyeDPI
- GUI geliştirici: @TunahanDilercan

## Destek
- 🐛 Bug report: GitHub Issues
- 💬 Soru/destek: GitHub Discussions
- 📧 İletişim: GitHub profili

---
**Not**: Bu uygulama sadece eğitim ve araştırma amaçlıdır. Yerel yasalara uygun kullanımdan kullanıcı sorumludur.
