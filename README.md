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

<img width="500" height="420" alt="image" src="https://github.com/user-attachments/assets/193925c1-42c3-4523-a788-c14a5f55015b" />

<img width="400" height="200" alt="image" src="https://github.com/user-attachments/assets/2da48a79-261c-4aa9-9998-19c74dc20903" />

<img width="570" height="545" alt="image" src="https://github.com/user-attachments/assets/4e1451fa-a317-4ef5-90df-057f459648b4" />



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
windres bununlaac.rc -O coff -o bununlaac_res
x86_64-w64-mingw32-gcc -municode -mwindows bununlaac.c bununlaac_res. -o bununlaac.exe -lshlwapi -ladvapi32 -lshell32 -lcomctl32 -lgdi32
