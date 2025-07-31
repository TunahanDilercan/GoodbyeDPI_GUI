# DPI Bypass Chrome Extension

Discord, Twitter ve diğer engellenen siteler için geliştirilmiş Chrome eklentisi.

## 🚀 Özellikler

### ✅ Desteklenen Siteler
- **Discord** (discord.com, discordapp.com)
- **Twitter/X** (twitter.com, x.com)
- **YouTube** (youtube.com, youtu.be)
- **Google Servisleri** (googleapis.com)
- **Özel siteler** (manuel ekleme)

### 🛡️ DPI Bypass Teknikleri
1. **DNS-over-HTTPS (DoH)**
   - Cloudflare (1.1.1.1)
   - Google (8.8.8.8)
   - Quad9 (9.9.9.9)
   - OpenDNS

2. **Header Manipulation**
   - User-Agent rotation
   - Custom headers
   - Request fragmentation

3. **WebSocket Bypass**
   - Discord real-time communication
   - URL fragmentation
   - Message chunking

4. **Network Request Modification**
   - Fetch API override
   - XMLHttpRequest interception
   - CORS header injection

## 📦 Kurulum

### Manuel Kurulum
1. Bu klasörü (`chrome_extension`) bilgisayarınıza indirin
2. Chrome tarayıcınızı açın
3. Adres çubuğuna `chrome://extensions/` yazın
4. Sağ üst köşeden "Geliştirici modu"nu aktifleştirin
5. "Paketlenmemiş uzantı yükle" butonuna tıklayın
6. `chrome_extension` klasörünü seçin
7. Eklenti yüklendi! 🎉

### Chrome Web Store (Gelecekte)
- Eklenti Chrome Web Store'a yüklenmeyi bekliyor

## 🎮 Kullanım

### Temel Kullanım
1. Chrome araç çubuğunda DPI Bypass ikonuna tıklayın
2. "Aktif" butonuna basarak eklentiyi etkinleştirin
3. Discord veya diğer sitelere normal şekilde erişin

### Gelişmiş Ayarlar
1. **DNS Sunucusu Değiştirme:**
   - Popup'ta DNS-over-HTTPS server seçin
   - Cloudflare en hızlı seçenektir

2. **Yeni Site Ekleme:**
   - "Korumalı Siteler" bölümüne domain girin
   - Örnek: `example.com`

3. **Test Etme:**
   - "Discord Bağlantısını Test Et" butonu ile test edin
   - "DNS Çözümlemeyi Test Et" ile DNS kontrolü yapın

## ⚙️ Teknik Detaylar

### Manifest V3
- Modern Chrome extension API'si kullanır
- Service Worker tabanlı background script
- Güvenli permissions yapısı

### Bypass Metodları

#### 1. DNS-over-HTTPS
```javascript
// DNS çözümlemesi HTTPS üzerinden yapılır
const dohUrl = 'https://cloudflare-dns.com/dns-query';
const response = await fetch(`${dohUrl}?name=discord.com&type=A`);
```

#### 2. Header Fragmentation
```javascript
// Header'lar parçalanarak DPI detection atlatılır
headers.push({
  name: 'Host',
  value: fragmentString(originalHost)
});
```

#### 3. WebSocket Bypass
```javascript
// WebSocket URL'leri modifiye edilir
const ws = new WebSocket(url + '?bypass=' + randomString());
```

### Desteklenen Domain'ler
- `discord.com` ve alt domain'leri
- `twitter.com`, `x.com`
- `youtube.com`, `youtu.be`
- `googleapis.com` ve alt domain'leri
- Manuel eklenen domain'ler

## 🔧 Geliştirme

### Dosya Yapısı
```
chrome_extension/
├── manifest.json          # Extension manifest
├── background.js          # Service worker
├── content.js            # Content script
├── popup.html            # Popup interface
├── popup.js              # Popup logic
├── icons/                # Extension icons
└── README.md             # Bu dosya
```

### Debug Etme
1. `chrome://extensions/` sayfasında eklentiye tıklayın
2. "background page" linkine tıklayın
3. Console'da log'ları görün

### Test Etme
```javascript
// Background script'te test
chrome.runtime.sendMessage({action: 'testDns', domain: 'discord.com'});
```

## 🛡️ Güvenlik

### Permissions
- `declarativeNetRequest`: Network request modification
- `storage`: Settings storage
- `tabs`: Tab information
- `webRequest`: Request interception
- `activeTab`: Current tab access

### Privacy
- Hiçbir kişisel veri toplanmaz
- Sadece network request'leri modifiye edilir
- DNS sorguları güvenli DoH sunucuları üzerinden yapılır

## 🚨 Sorun Giderme

### Yaygın Sorunlar

#### 1. Eklenti Çalışmıyor
- Extension'ların "Geliştirici modu"nda olduğundan emin olun
- Chrome'u yeniden başlatın
- Eklentiyi disable/enable yapın

#### 2. Discord Açılmıyor
- DNS sunucusunu değiştirmeyi deneyin
- "Discord Bağlantısını Test Et" butonunu kullanın
- Console'da hata mesajlarını kontrol edin

#### 3. Yavaş Bağlantı
- Cloudflare DNS sunucusunu seçin
- Gereksiz domain'leri listeden çıkarın
- Chrome cache'ini temizleyin

### Debug Komutları
```javascript
// Console'da çalıştırın:
chrome.runtime.sendMessage({action: 'getStatus'});
chrome.runtime.sendMessage({action: 'testDns', domain: 'discord.com'});
```

## 📊 İstatistikler

Extension popup'ında gösterilen:
- **Aktif Oturumlar**: Korumalı sitelerde açık tab sayısı
- **DNS Sorguları**: Toplam DNS çözümleme sayısı
- **Bypass Oranı**: Başarılı bypass yüzdesi

## 🔄 Güncellemeler

### Versiyon 1.0
- ✅ Temel DPI bypass
- ✅ DNS-over-HTTPS desteği
- ✅ Discord, Twitter, YouTube desteği
- ✅ Manual domain ekleme
- ✅ Real-time testing

### Gelecek Özellikler
- 🔄 Otomatik proxy detection
- 🔄 VPN integration
- 🔄 Advanced traffic analysis
- 🔄 Mobile browser support

## 📞 Destek

- **GitHub Issues**: Sorun bildirimi için
- **Discord**: Topluluk desteği
- **Email**: Özel destek talebi

## ⚖️ Lisans

MIT License - Detaylar için LICENSE dosyasına bakın.

## 🙏 Katkıda Bulunma

1. Fork edin
2. Feature branch oluşturun
3. Commit edin
4. Push edin
5. Pull Request açın

---

**Not**: Bu eklenti eğitim amaçlıdır. Yasal sorumluluk kullanıcıya aittir.
