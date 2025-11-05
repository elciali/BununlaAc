# 🧩 BununlaAç 2025  
**Windows Sağ Tık Menüsüne Modern “Bununla Aç” Seçeneği Ekleyici**

![Platform](https://img.shields.io/badge/platform-Windows-blue)
![Language](https://img.shields.io/badge/language-C-blue)
![Build](https://img.shields.io/badge/build-MinGW%20%7C%20MSVC-orange)
![License](https://img.shields.io/badge/license-BununlaAç%202025%20License-blue)

---

## 📖 Proje Hakkında

**BununlaAç 2025**, Windows’un sağ tık (context menu) sistemine entegre olarak çalışan, modern görünümlü bir araçtır.  
Herhangi bir `.exe` programını “BununlaAç” menüsüne ekleyebilir, kolayca kaldırabilir veya menüyü tamamen yönetebilirsiniz.  

Bu araç:
- Herhangi bir çalıştırılabilir dosyayı menüye eklemenizi sağlar  
- Eklenen programları kolayca kaldırabilir  
- Menü kurulumunu veya kaldırmayı tek tıklamayla yapabilir  
- Tamamen **Windows API** ile yazılmıştır (Win32 C)  
- Modern butonlar ve sade kullanıcı arayüzü içerir  

---

## 🖼️ Ekran Görünümü

> Uygulama modern `Segoe UI` fontu, sade renk paleti ve `RoundedRect` butonlarla tasarlanmıştır.

*(isteğe bağlı olarak buraya `screenshot.png` ekleyebilirsin)*

---

## ⚙️ Özellikler

| Özellik | Açıklama |
|----------|-----------|
| 🔧 Menü Kur / Kaldır | “BununlaAç” ana menüsünü Windows kayıt defterine ekler veya kaldırır |
| ➕ Program Ekle | Seçilen `.exe` dosyasını alt menüye ekler |
| ➖ Program Sil | Menüyü veya programı kaldırır |
| 🪟 Modern UI | GDI+ ve Windows API ile oluşturulmuş sade arayüz |
| 🧠 Akıllı Güvenli İsim | Dosya adlarını otomatik olarak güvenli biçime dönüştürür |
| 📜 Günlük Kaydı | `%TEMP%\bununlaac.log` dosyasına log yazar |
| 🔁 Explorer Yeniden Başlatma | Menü değişikliklerinden sonra otomatik olarak explorer.exe’yi yeniler |

---

## 🧰 Derleme (Build)

### 🔹 MinGW (x86_64-w64-mingw32)
```bash
x86_64-w64-mingw32-gcc bununlaac.c -o bununlaac.exe \
  -lcomctl32 -lgdi32 -lshlwapi -ladvapi32
