#define _CRT_SECURE_NO_WARNINGS
#define _WIN32_WINNT 0x0600

#ifndef UNICODE
#  define UNICODE
#  define _UNICODE
#endif

#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "Comctl32.lib")

#define LOG_FILE_TEMPLATE  L"%TEMP%\\bununlaac.log"
#define REG_BASE           L"*\\shell\\BununlaAc"
#define REG_SHELL          L"*\\shell\\BununlaAc\\shell"

// Modern renkler
#define CLR_PRIMARY        RGB(0, 120, 215)
#define CLR_PRIMARY_HOVER  RGB(0, 103, 192)
#define CLR_DANGER         RGB(196, 43, 28)
#define CLR_DANGER_HOVER   RGB(164, 38, 44)
#define CLR_BACKGROUND     RGB(243, 243, 243)
#define CLR_TEXT           RGB(32, 32, 32)
#define CLR_BORDER         RGB(218, 220, 224)

static WCHAR g_target[MAX_PATH];
static WCHAR g_safeName[64];
static WCHAR g_logPath[MAX_PATH];
static BOOL  g_exists = FALSE;

// Modern buton yapısı
typedef struct {
    HWND hwnd;
    BOOL isHovered;
    BOOL isPressed;
    COLORREF bgColor;
    COLORREF hoverColor;
} ModernButton;

static ModernButton g_buttons[10];
static int g_buttonCount = 0;

/* ---------- log ---------- */
static void log_w(const WCHAR *fmt, ...) {
    FILE *fp = _wfopen(g_logPath, L"a, ccs=UTF-8");
    if (!fp) return;
    SYSTEMTIME st;
    GetLocalTime(&st);
    fwprintf(fp, L"[%04d-%02d-%02d %02d:%02d:%02d] ",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    va_list ap;
    va_start(ap, fmt);
    vfwprintf(fp, fmt, ap);
    va_end(ap);
    fputwc(L'\n', fp);
    fclose(fp);
}

/* ---------- güvenli isim ---------- */
static void make_safe_name(const WCHAR *in, WCHAR *out, size_t outChars) {
    WCHAR tmp[64];
    wcsncpy(tmp, in, 63);
    tmp[63] = 0;
    PathRemoveExtensionW(tmp);
    size_t i = 0, j = 0;
    while (tmp[i] && j + 1 < outChars) {
        WCHAR ch = tmp[i++];
        if (wcschr(L" ()&.;,[]@#$%'\"", ch)) ch = L'_';
        out[j++] = ch;
    }
    out[j] = 0;
}

/* ---------- registry var mı? ---------- */
static BOOL key_exists(const WCHAR *sub) {
    HKEY h;
    LONG rc = RegOpenKeyExW(HKEY_CLASSES_ROOT, sub, 0, KEY_READ, &h);
    if (rc == ERROR_SUCCESS) RegCloseKey(h);
    return rc == ERROR_SUCCESS;
}

/* ---------- mevcut kayıt ara ---------- */
static void check_existing(void) {
    WCHAR keyPath[256];
    wsprintfW(keyPath, REG_SHELL L"\\%s", g_safeName);
    g_exists = key_exists(keyPath);
}

/* ---------- pencere ortala ---------- */
static void CenterWindow(HWND hwnd, HWND hwndParent) {
    RECT rc, rcP;
    GetWindowRect(hwnd, &rc);
    if (!hwndParent) hwndParent = GetDesktopWindow();
    GetWindowRect(hwndParent, &rcP);
    SetWindowPos(hwnd, NULL,
                 rcP.left + ((rcP.right - rcP.left) - (rc.right - rc.left)) / 2,
                 rcP.top  + ((rcP.bottom - rcP.top) - (rc.bottom - rc.top)) / 2,
                 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

/* ---------- Explorer yeniden başlat ---------- */
static void restart_explorer(void) {
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    WCHAR killCmd[] = L"taskkill /f /im explorer.exe";
    WCHAR startCmd[] = L"C:\\Windows\\explorer.exe";

    // Explorer'ı kapat
    if (CreateProcessW(NULL, killCmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 3000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    Sleep(800);

    STARTUPINFOW si2 = {0};
    PROCESS_INFORMATION pi2 = {0};
    si2.cb = sizeof(si2);
    si2.dwFlags = STARTF_USESHOWWINDOW;
    si2.wShowWindow = SW_HIDE;

    CreateProcessW(startCmd, NULL, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si2, &pi2);
    if (pi2.hProcess) {
        CloseHandle(pi2.hProcess);
        CloseHandle(pi2.hThread);
    }
}

/* ---------- ana menü kurulu mu? ---------- */
static BOOL is_installed(void) {
    return key_exists(REG_BASE);
}

/* ---------- Modern Font Oluştur ---------- */
static HFONT CreateModernFont(int size, BOOL bold) {
    return CreateFontW(
        -MulDiv(size, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72),
        0, 0, 0,
        bold ? FW_SEMIBOLD : FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );
}

/* ---------- Modern Buton Çiz ---------- */
static void DrawModernButton(HDC hdc, ModernButton *btn, RECT *rc, const WCHAR *text) {
    // Arkaplan rengi
    COLORREF bgColor = btn->isPressed ? btn->hoverColor : 
                       (btn->isHovered ? btn->hoverColor : btn->bgColor);
    
    // Yuvarlatılmış köşe efekti için fırça oluştur
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    HPEN hPen = CreatePen(PS_SOLID, 0, btn->bgColor);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    
    // Yuvarlatılmış dikdörtgen
    RoundRect(hdc, rc->left, rc->top, rc->right, rc->bottom, 8, 8);
    
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);
    
    // Metin
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    HFONT hFont = CreateModernFont(10, FALSE);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    DrawTextW(hdc, text, -1, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

/* ---------- Buton Window Proc ---------- */
static LRESULT CALLBACK ModernButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ModernButton *btn = NULL;
    for (int i = 0; i < g_buttonCount; i++) {
        if (g_buttons[i].hwnd == hwnd) {
            btn = &g_buttons[i];
            break;
        }
    }
    
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            WCHAR text[64];
            GetWindowTextW(hwnd, text, 64);
            
            if (btn) {
                DrawModernButton(hdc, btn, &rc, text);
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            if (btn && !btn->isHovered) {
                btn->isHovered = TRUE;
                InvalidateRect(hwnd, NULL, TRUE);
                TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd, 0};
                TrackMouseEvent(&tme);
            }
            return 0;
        }
        
        case WM_MOUSELEAVE: {
            if (btn) {
                btn->isHovered = FALSE;
                InvalidateRect(hwnd, NULL, TRUE);
            }
            return 0;
        }
        
        case WM_LBUTTONDOWN: {
            if (btn) {
                btn->isPressed = TRUE;
                SetCapture(hwnd);
                InvalidateRect(hwnd, NULL, TRUE);
            }
            return 0;
        }
        
        case WM_LBUTTONUP: {
            if (btn) {
                btn->isPressed = FALSE;
                ReleaseCapture();
                InvalidateRect(hwnd, NULL, TRUE);
                
                RECT rc;
                GetClientRect(hwnd, &rc);
                POINT pt = {LOWORD(lParam), HIWORD(lParam)};
                if (PtInRect(&rc, pt)) {
                    SendMessageW(GetParent(hwnd), WM_COMMAND, GetDlgCtrlID(hwnd), (LPARAM)hwnd);
                }
            }
            return 0;
        }
        
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
    }
    
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* ---------- Modern Buton Oluştur ---------- */
static HWND CreateModernButton(HWND parent, const WCHAR *text, int x, int y, int w, int h, 
                               int id, COLORREF bgColor, COLORREF hoverColor) {
    static BOOL classRegistered = FALSE;
    if (!classRegistered) {
        WNDCLASSW wc = {0};
        wc.lpfnWndProc = ModernButtonProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"ModernButton";
        wc.hCursor = LoadCursor(NULL, IDC_HAND);
        wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        RegisterClassW(&wc);
        classRegistered = TRUE;
    }
    
    HWND hwnd = CreateWindowExW(
        0, L"ModernButton", text,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        x, y, w, h,
        parent, (HMENU)(INT_PTR)id,
        GetModuleHandle(NULL), NULL
    );
    
    if (hwnd && g_buttonCount < 10) {
        g_buttons[g_buttonCount].hwnd = hwnd;
        g_buttons[g_buttonCount].isHovered = FALSE;
        g_buttons[g_buttonCount].isPressed = FALSE;
        g_buttons[g_buttonCount].bgColor = bgColor;
        g_buttons[g_buttonCount].hoverColor = hoverColor;
        g_buttonCount++;
    }
    
    return hwnd;
}

/* ======================================================== */
/*                     DIALOG: EKLE/SİL                    */
/* ======================================================== */

static LRESULT CALLBACK AddRemoveDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static INT_PTR result = 0;
    
    switch (msg) {
        case WM_INITDIALOG:
            return TRUE;
            
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, CLR_TEXT);
            SetBkColor(hdcStatic, CLR_BACKGROUND);
            return (INT_PTR)CreateSolidBrush(CLR_BACKGROUND);
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            // Arkaplan
            HBRUSH hBrush = CreateSolidBrush(CLR_BACKGROUND);
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                result = g_exists ? 2 : 1;
                DestroyWindow(hwnd);
                return 0;
            }
            if (LOWORD(wParam) == IDCANCEL) {
                result = 0;
                DestroyWindow(hwnd);
                return 0;
            }
            break;
            
        case WM_CLOSE:
            result = 0;
            DestroyWindow(hwnd);
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage((int)result);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static INT_PTR show_add_remove_dialog(void) {
    g_buttonCount = 0;
    
    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = AddRemoveDlgProc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = L"BununlaAc_AddRemoveDlg";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(CLR_BACKGROUND);
    
    if (!RegisterClassW(&wc)) return 0;

    HWND hDlg = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        L"BununlaAc_AddRemoveDlg", L"BununlaAç",
        WS_POPUP | WS_VISIBLE,
        0, 0, 400, 200,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    
    if (!hDlg) return 0;
    
    // Gölge efekti
    SetLayeredWindowAttributes(hDlg, 0, 255, LWA_ALPHA);
    
    // Başlık
    HFONT hFontTitle = CreateModernFont(14, TRUE);
    HWND hTitle = CreateWindowExW(0, L"STATIC", L"Program Yönetimi",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        30, 25, 340, 30, hDlg, NULL, NULL, NULL);
    SendMessageW(hTitle, WM_SETFONT, (WPARAM)hFontTitle, TRUE);
    
    // Açıklama
    HFONT hFontNormal = CreateModernFont(10, FALSE);
    WCHAR text[128];
    wsprintfW(text, L"%s programını menüye eklemek veya kaldırmak ister misiniz?", g_safeName);
    HWND hDesc = CreateWindowExW(0, L"STATIC", text,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        30, 65, 340, 50, hDlg, NULL, NULL, NULL);
    SendMessageW(hDesc, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
    
    // Butonlar
    if (g_exists) {
        CreateModernButton(hDlg, L"Kaldır", 30, 135, 160, 40, IDOK, CLR_DANGER, CLR_DANGER_HOVER);
    } else {
        CreateModernButton(hDlg, L"Ekle", 30, 135, 160, 40, IDOK, CLR_PRIMARY, CLR_PRIMARY_HOVER);
    }
    CreateModernButton(hDlg, L"İptal", 210, 135, 160, 40, IDCANCEL, RGB(100, 100, 100), RGB(80, 80, 80));
    
    CenterWindow(hDlg, GetDesktopWindow());
    ShowWindow(hDlg, SW_SHOW);
    
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!IsWindow(hDlg)) break;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    
    return msg.wParam;
}

/* ======================================================== */
/*                    DIALOG: ANA MENÜ                     */
/* ======================================================== */

static LRESULT CALLBACK MainDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static INT_PTR result = 0;
    
    switch (msg) {
        case WM_INITDIALOG:
            return TRUE;
            
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, CLR_TEXT);
            SetBkColor(hdcStatic, CLR_BACKGROUND);
            return (INT_PTR)CreateSolidBrush(CLR_BACKGROUND);
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            // Arkaplan
            HBRUSH hBrush = CreateSolidBrush(CLR_BACKGROUND);
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);
            
            // Üst çubuk (accent color)
            RECT topBar = {0, 0, rc.right, 60};
            HBRUSH hAccent = CreateSolidBrush(CLR_PRIMARY);
            FillRect(hdc, &topBar, hAccent);
            DeleteObject(hAccent);
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_COMMAND:
            if (LOWORD(wParam) == 101) {
                result = 1;
                DestroyWindow(hwnd);
            } else if (LOWORD(wParam) == 102) {
                result = 2;
                DestroyWindow(hwnd);
            } else if (LOWORD(wParam) == IDCANCEL) {
                result = 0;
                DestroyWindow(hwnd);
            }
            break;
            
        case WM_CLOSE:
            result = 0;
            DestroyWindow(hwnd);
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage((int)result);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static INT_PTR main_dialog(void) {
    g_buttonCount = 0;
    BOOL installed = is_installed();

    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = MainDlgProc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = L"BununlaAc_MainDlg";
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(CLR_BACKGROUND);
    
    RegisterClassW(&wc);

    HWND hDlg = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        L"BununlaAc_MainDlg", L"BununlaAç 2025",
        WS_POPUP | WS_VISIBLE,
        0, 0, 500, 420,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    
    if (!hDlg) return 0;
    
    SetLayeredWindowAttributes(hDlg, 0, 255, LWA_ALPHA);

    // Başlık (beyaz metin üstte)
    HFONT hFontTitle = CreateModernFont(18, TRUE);
    HWND hTitle = CreateWindowExW(0, L"STATIC", L"BununlaAç 2025",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 15, 500, 30, hDlg, NULL, NULL, NULL);
    SendMessageW(hTitle, WM_SETFONT, (WPARAM)hFontTitle, TRUE);
    SetWindowLongPtrW(hTitle, GWLP_WNDPROC, (LONG_PTR)DefWindowProcW);
    
    // Alt başlık
    HFONT hFontSubtitle = CreateModernFont(10, FALSE);
    HWND hSubtitle = CreateWindowExW(0, L"STATIC", L"Windows Kontekst Menü Yöneticisi",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 45, 500, 20, hDlg, NULL, NULL, NULL);
    SendMessageW(hSubtitle, WM_SETFONT, (WPARAM)hFontSubtitle, TRUE);
    
    // Açıklama metni
    HFONT hFontNormal = CreateModernFont(10, FALSE);
    HWND hDesc = CreateWindowExW(0, L"STATIC",
        L"Bu uygulama, dosyalarınızı farklı programlarla açmak için\n"
        L"Windows sağ tık menüsüne özel bir \"BununlaAç\" seçeneği ekler.\n\n"
        L"• Herhangi bir .exe programını menüye ekleyebilirsiniz\n"
        L"• Eklediğiniz programları kolayca yönetebilirsiniz\n"
        L"• Menüden istediğiniz zaman kaldırabilirsiniz",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        40, 90, 420, 120, hDlg, NULL, NULL, NULL);
    SendMessageW(hDesc, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
    
    // İletişim bilgisi
    HWND hContact = CreateWindowExW(0, L"STATIC",
        L"Geliştirici: Ali ELÇİ\n"
        L"GitHub: github.com/elciali\n"
        L"E-posta: alielcitr@gmail.com",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        40, 230, 420, 70, hDlg, NULL, NULL, NULL);
    SendMessageW(hContact, WM_SETFONT, (WPARAM)hFontNormal, TRUE);

    // Butonlar
    int btnY = 320;
    if (!installed) {
        CreateModernButton(hDlg, L"Menüyü Kur", 150, btnY, 200, 45, 101, CLR_PRIMARY, CLR_PRIMARY_HOVER);
    }
    if (installed) {
        CreateModernButton(hDlg, L"Menüyü Kaldır", 150, btnY, 200, 45, 102, CLR_DANGER, CLR_DANGER_HOVER);
    }
    
    CreateModernButton(hDlg, L"Kapat", 175, btnY + 55, 150, 35, IDCANCEL, RGB(120, 120, 120), RGB(100, 100, 100));

    CenterWindow(hDlg, GetDesktopWindow());
    ShowWindow(hDlg, SW_SHOW);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!IsWindow(hDlg)) break;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return msg.wParam;
}

/* ======================================================== */
/*                      İŞLEVLER                            */
/* ======================================================== */

static void do_add(void) {
    WCHAR keyPath[256], dispName[64], icon[MAX_PATH], cmd[MAX_PATH * 2];
    const WCHAR* fileName = PathFindFileNameW(g_target);
    wcscpy(dispName, (WCHAR*)fileName);
    PathRemoveExtensionW(dispName);

    wcscpy(keyPath, REG_SHELL);
    wcscat(keyPath, L"\\");
    wcscat(keyPath, g_safeName);

    for (int i = 2; key_exists(keyPath); ++i) {
        wsprintfW(keyPath, REG_SHELL L"\\%s_%d", g_safeName, i);
    }

    HKEY hKey, hCmd;
    LONG rc = RegCreateKeyExW(HKEY_CLASSES_ROOT, keyPath, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    if (rc != ERROR_SUCCESS) goto err;

    RegSetValueExW(hKey, NULL, 0, REG_SZ,
                   (const BYTE*)dispName, (DWORD)((wcslen(dispName) + 1) * sizeof(WCHAR)));

    wsprintfW(icon, L"%s,0", g_target);
    RegSetValueExW(hKey, L"Icon", 0, REG_SZ,
                   (const BYTE*)icon, (DWORD)((wcslen(icon) + 1) * sizeof(WCHAR)));

    rc = RegCreateKeyExW(hKey, L"command", 0, NULL, 0, KEY_WRITE, NULL, &hCmd, NULL);
    if (rc != ERROR_SUCCESS) { RegCloseKey(hKey); goto err; }

    wsprintfW(cmd, L"\"%s\" \"%%1\"", g_target);
    RegSetValueExW(hCmd, NULL, 0, REG_SZ,
                   (const BYTE*)cmd, (DWORD)((wcslen(cmd) + 1) * sizeof(WCHAR)));

    RegCloseKey(hCmd);
    RegCloseKey(hKey);
    log_w(L"EKLE: %s", keyPath);
    MessageBoxW(NULL, dispName, L"✓ Program Eklendi", MB_ICONINFORMATION | MB_TOPMOST);
    return;
err:
    MessageBoxW(NULL, L"Kayıt eklenemedi! Yönetici hakları gerekebilir.", L"✗ Hata", MB_ICONERROR | MB_TOPMOST);
}

static void do_delete(void) {
    WCHAR keyPath[256];
    wsprintfW(keyPath, REG_SHELL L"\\%s", g_safeName);
    LONG rc = RegDeleteTreeW(HKEY_CLASSES_ROOT, keyPath);
    if (rc == ERROR_SUCCESS) {
        log_w(L"SIL: %s", keyPath);
        MessageBoxW(NULL, g_safeName, L"✓ Program Kaldırıldı", MB_ICONINFORMATION | MB_TOPMOST);
    } else {
        log_w(L"SIL hata %ld", rc);
        MessageBoxW(NULL, L"Silme başarısız! Yönetici hakları gerekebilir.", L"✗ Hata", MB_ICONERROR | MB_TOPMOST);
    }
}

static int do_install(void) {
    HKEY hKey, hShell, hAdd, hAddCmd;
    LONG rc;
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    log_w(L"INSTALL: Uygulama yolu: %s", exePath);

    rc = RegCreateKeyExW(HKEY_CLASSES_ROOT, REG_BASE, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    if (rc != ERROR_SUCCESS) goto err;

    RegSetValueExW(hKey, L"MUIVerb", 0, REG_SZ, (const BYTE*)L"BununlaAç", (DWORD)(11 * sizeof(WCHAR)));
    RegSetValueExW(hKey, L"Icon", 0, REG_SZ, (const BYTE*)exePath, (DWORD)((wcslen(exePath) + 1) * sizeof(WCHAR)));
    RegSetValueExW(hKey, L"SubCommands", 0, REG_SZ, (const BYTE*)L"", (DWORD)(1 * sizeof(WCHAR)));

    rc = RegCreateKeyExW(hKey, L"shell", 0, NULL, 0, KEY_WRITE, NULL, &hShell, NULL);
    if (rc != ERROR_SUCCESS) { RegCloseKey(hKey); goto err; }

    rc = RegCreateKeyExW(hShell, L"zzz_Ekle", 0, NULL, 0, KEY_WRITE, NULL, &hAdd, NULL);
    if (rc != ERROR_SUCCESS) { RegCloseKey(hShell); RegCloseKey(hKey); goto err; }

    RegSetValueExW(hAdd, NULL, 0, REG_SZ, (const BYTE*)L"➕ Program Ekle & Kaldır", (DWORD)(26 * sizeof(WCHAR)));
    RegSetValueExW(hAdd, L"Icon", 0, REG_SZ, (const BYTE*)L"shell32.dll,132", (DWORD)(16 * sizeof(WCHAR)));
	
    rc = RegCreateKeyExW(hAdd, L"command", 0, NULL, 0, KEY_WRITE, NULL, &hAddCmd, NULL);
    if (rc != ERROR_SUCCESS) { RegCloseKey(hAdd); RegCloseKey(hShell); RegCloseKey(hKey); goto err; }

    WCHAR cmdLine[MAX_PATH * 2];
    wsprintfW(cmdLine, L"\"%s\" \"%%1\"", exePath);
    RegSetValueExW(hAddCmd, NULL, 0, REG_SZ, (const BYTE*)cmdLine, (DWORD)((wcslen(cmdLine) + 1) * sizeof(WCHAR)));

    RegCloseKey(hAddCmd);
    RegCloseKey(hAdd);
    RegCloseKey(hShell);
    RegCloseKey(hKey);

    MessageBoxW(NULL,
                L"✓ Menü başarıyla kuruldu!\n\n"
                L"Artık herhangi bir dosyaya sağ tıklayarak\n"
                L"'BununlaAç > Program Ekle & Kaldır' seçeneğini kullanabilirsiniz.",
                L"Kurulum Tamamlandı", MB_ICONINFORMATION | MB_TOPMOST);
    log_w(L"INSTALL: ana menü ve Ekle komutu eklendi");
    return 0;
err:
    MessageBoxW(NULL, 
                L"Kayıt yazılamadı!\n\n"
                L"Lütfen programı yönetici olarak çalıştırın.",
                L"✗ Kurulum Hatası", MB_ICONERROR | MB_TOPMOST);
    log_w(L"INSTALL: Hata kodu %ld", rc);
    return 1;
}

static int do_uninstall(void) {
    LONG rc1 = RegDeleteTreeW(HKEY_CLASSES_ROOT, REG_BASE);
    LONG rc2 = RegDeleteTreeW(HKEY_CLASSES_ROOT, REG_SHELL);
    if (rc1 == ERROR_FILE_NOT_FOUND) rc1 = ERROR_SUCCESS;
    if (rc2 == ERROR_FILE_NOT_FOUND) rc2 = ERROR_SUCCESS;

    if (rc1 == ERROR_SUCCESS && rc2 == ERROR_SUCCESS) {
        MessageBoxW(NULL, 
                    L"✓ \"BununlaAç\" menüsü başarıyla kaldırıldı.\n\n"
                    L"Tüm eklediğiniz programlar menüden temizlendi.",
                    L"Kaldırma Tamamlandı", MB_ICONINFORMATION | MB_TOPMOST);
        log_w(L"UNINSTALL: Başarıyla kaldırıldı");
        return 0;
    } else {
        log_w(L"UNINSTALL: Hata - REG_BASE=%ld, REG_SHELL=%ld", rc1, rc2);
        MessageBoxW(NULL, 
                    L"Menü kaldırılamadı!\n\n"
                    L"Lütfen programı yönetici olarak çalıştırın.",
                    L"✗ Kaldırma Hatası", MB_ICONERROR | MB_TOPMOST);
        return 1;
    }
}

/* ======================================================== */
/*                      GİRİŞ NOKTASI                       */
/* ======================================================== */

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR lpCmdLine, int nShow) {
    ExpandEnvironmentStringsW(LOG_FILE_TEMPLATE, g_logPath, MAX_PATH);

    int argc;
    PWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) {
        log_w(L"HATA: CommandLineToArgvW");
        return 1;
    }

    // CLI modları
    if (argc >= 2) {
        if (wcscmp(argv[1], L"-install") == 0 || wcscmp(argv[1], L"/install") == 0) {
            LocalFree(argv);
            return do_install();
        }
        if (wcscmp(argv[1], L"-uninstall") == 0 || wcscmp(argv[1], L"/uninstall") == 0) {
            LocalFree(argv);
            return do_uninstall();
        }
    }

    // Dosya ile mod
    if (argc >= 2) {
        GetFullPathNameW(argv[1], MAX_PATH, g_target, NULL);
        LocalFree(argv);

        if (GetFileAttributesW(g_target) == INVALID_FILE_ATTRIBUTES) {
            MessageBoxW(NULL, L"Dosya bulunamadı!", L"✗ Hata", MB_ICONERROR | MB_TOPMOST);
            return 1;
        }
        if (wcsicmp(PathFindExtensionW(g_target), L".exe") != 0) {
            MessageBoxW(NULL, 
                        L"Sadece .exe (çalıştırılabilir) dosyalar desteklenir.\n\n"
                        L"Lütfen bir program (.exe) dosyası seçin.",
                        L"✗ Geçersiz Dosya Türü", MB_ICONERROR | MB_TOPMOST);
            return 1;
        }

        make_safe_name(PathFindFileNameW(g_target), g_safeName, _countof(g_safeName));
        check_existing();

        INT_PTR rc = show_add_remove_dialog();
        if (rc == 1) do_add();
        else if (rc == 2) do_delete();
        return 0;
    }

    // Ana menü
    LocalFree(argv);
    INT_PTR choice = main_dialog();
    if (choice == 1) {
        if (do_install() == 0) restart_explorer();
    } else if (choice == 2) {
        if (do_uninstall() == 0) restart_explorer();
    }
    return 0;
}