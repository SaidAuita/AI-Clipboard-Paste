#include "ClipboardPastePlugin.h"
#include "ConfigManager.h"
#include <cstdio>
#include <windows.h>
#include <shellapi.h>

#define PLUGIN_VERSION_STR     "v1.25"

#define IDC_STATIC_TITLE       100
#define IDC_STATIC_VERSION     101
#define IDC_BTN_COPY           102
#define IDC_BTN_PASTE_IN_PLACE 103
#define IDC_BTN_PASTE_CENTER   104
#define IDC_BTN_PASTE_ALL_ARTBOARDS 105
#define IDC_BTN_CLEAR          106
#define IDC_STATIC_STATUS      107
#define IDC_BTN_SETTINGS       108
#define IDC_STATIC_LINK_AUTHOR 109
#define IDC_STATIC_LINK_TOOLS  110
#define IDC_BTN_REFLECT_VERT   111
#define IDC_BTN_REFLECT_HORIZ  112

// Tabs
#define IDC_TAB_CLIPBOARD      120
#define IDC_TAB_OFFSET         121

// Tab 2: Offset Clone controls
#define IDC_STATIC_OFFSET_INFO 130
#define IDC_STATIC_OFFSET_MOD_HEADER 142
#define IDC_BTN_DIR_1          131
#define IDC_BTN_DIR_2          132
#define IDC_BTN_DIR_3          133
#define IDC_BTN_DIR_4          134
#define IDC_BTN_DIR_5          135
#define IDC_BTN_DIR_6          136
#define IDC_BTN_DIR_7          137
#define IDC_BTN_DIR_8          138
#define IDC_BTN_DIR_9          139
#define IDC_STATIC_OFFSET_STATUS 140
#define IDC_BTN_OFFSET_SETTINGS 141

static HWND g_hStaticHeaderTitle = NULL;
static HWND g_hStaticHeaderVersion = NULL;

// Tab Switch Buttons
static HWND g_hTabBtnClipboard = NULL;
static HWND g_hTabBtnOffset = NULL;

// Tab 0 (Clipboard) controls
static HWND g_hBtnCopy = NULL;
static HWND g_hBtnPasteInPlace = NULL;
static HWND g_hBtnPasteCenter = NULL;
static HWND g_hBtnPasteAllArtboards = NULL;
static HWND g_hBtnReflectVert = NULL;
static HWND g_hBtnReflectHoriz = NULL;
static HWND g_hBtnClear = NULL;
static HWND g_hBtnSettings = NULL;
static HWND g_hStaticStatus = NULL;

// Tab 1 (Offset Clone) controls
static HWND g_hStaticOffsetInfo = NULL;
static HWND g_hStaticOffsetModHeader = NULL;
static HWND g_hBtnDir[10] = { NULL };
static HWND g_hStaticOffsetStatus = NULL;
static HWND g_hBtnOffsetSettings = NULL;

static const wchar_t* g_dirPrefixes[10] = {
    L"",
    L"",        // 1: угловая кнопка - чисто цифра/буква
    L"v ",      // 2: вниз
    L"",        // 3: угловая кнопка - чисто цифра/буква
    L"< ",      // 4: влево
    L"\x2022 ", // 5: • центр
    L"> ",      // 6: вправо
    L"",        // 7: угловая кнопка - чисто цифра/буква
    L"^ ",      // 8: вверх
    L""         // 9: угловая кнопка - чисто цифра/буква
};

// Footer
static HWND g_hStaticLinkAuthor = NULL;
static HWND g_hStaticLinkTools = NULL;

static HFONT g_hUIFont = NULL;
static HFONT g_hHeaderFont = NULL;
static HFONT g_hSmallFont = NULL;
static HBRUSH g_hPanelBgBrush = NULL;
static float g_appliedFontScale = 0.0f;
static WNDPROC g_oldPanelProc = NULL;
static WNDPROC g_oldButtonProc = NULL;
static WNDPROC g_oldStatusProc = NULL;
static HHOOK g_hKeyHook = NULL;

static float GetPanelScale(HWND hwnd) {
    float aiScale = 1.0f;
    if (sAIUser && sAIUser->GetAppScaleFactor) {
        AIReal s = sAIUser->GetAppScaleFactor();
        if (s >= 0.5f && s <= 5.0f) {
            aiScale = (float)s;
        }
    }

    float winDpiScale = 1.0f;
    if (hwnd) {
        HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
        if (hUser32) {
            typedef UINT (WINAPI *GetDpiForWindowProc)(HWND);
            GetDpiForWindowProc pGetDpi = (GetDpiForWindowProc)GetProcAddress(hUser32, "GetDpiForWindow");
            if (pGetDpi) {
                UINT dpi = pGetDpi(hwnd);
                if (dpi > 0) winDpiScale = (float)dpi / 96.0f;
            }
        }
        if (winDpiScale <= 0.1f) {
            HDC hdc = GetDC(hwnd);
            if (hdc) {
                int dpi = GetDeviceCaps(hdc, LOGPIXELSY);
                ReleaseDC(hwnd, hdc);
                if (dpi > 0) winDpiScale = (float)dpi / 96.0f;
            }
        }
    }

    float finalScale = (std::max)(aiScale, winDpiScale);
    if (finalScale < 0.75f) finalScale = 0.75f;
    if (finalScale > 4.0f)  finalScale = 4.0f;
    return finalScale;
}

static void EnsureFontsForScale(HWND hwnd, float scale) {
    if (std::abs(g_appliedFontScale - scale) < 0.01f && g_hUIFont) return;
    g_appliedFontScale = scale;

    if (g_hUIFont)     { DeleteObject(g_hUIFont);     g_hUIFont = NULL; }
    if (g_hHeaderFont) { DeleteObject(g_hHeaderFont); g_hHeaderFont = NULL; }
    if (g_hSmallFont)  { DeleteObject(g_hSmallFont);  g_hSmallFont = NULL; }

    int szUI     = -(int)(11.0f * scale + 0.5f);
    int szHeader = -(int)(13.0f * scale + 0.5f);
    int szSmall  = -(int)(10.0f * scale + 0.5f);

    g_hUIFont = CreateFontA(szUI, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_hHeaderFont = CreateFontA(szHeader, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    g_hSmallFont = CreateFontA(szSmall, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    if (hwnd && IsWindow(hwnd)) {
        if (g_hStaticHeaderTitle) SendMessage(g_hStaticHeaderTitle, WM_SETFONT, (WPARAM)g_hHeaderFont, TRUE);
        if (g_hStaticHeaderVersion) SendMessage(g_hStaticHeaderVersion, WM_SETFONT, (WPARAM)g_hSmallFont, TRUE);

        if (g_hTabBtnClipboard) SendMessage(g_hTabBtnClipboard, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        if (g_hTabBtnOffset) SendMessage(g_hTabBtnOffset, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);

        if (g_hBtnCopy) SendMessage(g_hBtnCopy, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        if (g_hBtnPasteInPlace) SendMessage(g_hBtnPasteInPlace, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        if (g_hBtnPasteCenter) SendMessage(g_hBtnPasteCenter, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        if (g_hBtnPasteAllArtboards) SendMessage(g_hBtnPasteAllArtboards, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        if (g_hBtnReflectVert) SendMessage(g_hBtnReflectVert, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        if (g_hBtnReflectHoriz) SendMessage(g_hBtnReflectHoriz, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        if (g_hBtnClear) SendMessage(g_hBtnClear, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        if (g_hBtnSettings) SendMessage(g_hBtnSettings, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        if (g_hStaticStatus) SendMessage(g_hStaticStatus, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);

        if (g_hStaticOffsetInfo) SendMessage(g_hStaticOffsetInfo, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        if (g_hStaticOffsetModHeader) SendMessage(g_hStaticOffsetModHeader, WM_SETFONT, (WPARAM)g_hHeaderFont, TRUE);
        for (int i = 1; i <= 9; ++i) {
            if (g_hBtnDir[i]) SendMessage(g_hBtnDir[i], WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        }
        if (g_hStaticOffsetStatus) SendMessage(g_hStaticOffsetStatus, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        if (g_hBtnOffsetSettings) SendMessage(g_hBtnOffsetSettings, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);

        if (g_hStaticLinkAuthor) SendMessage(g_hStaticLinkAuthor, WM_SETFONT, (WPARAM)g_hSmallFont, TRUE);
        if (g_hStaticLinkTools) SendMessage(g_hStaticLinkTools, WM_SETFONT, (WPARAM)g_hSmallFont, TRUE);
    }
}

struct UIThemeColors {
    bool isDark;
    COLORREF panelBg;
    COLORREF textPrimary;
    COLORREF textSecondary;
    COLORREF btnBg;
    COLORREF btnBgHover;
    COLORREF btnBgPressed;
    COLORREF btnBorder;
    COLORREF btnText;
    COLORREF accentBg;
    COLORREF accentText;
    COLORREF statusBg;
    COLORREF statusBorder;
    COLORREF statusText;
    COLORREF linkColor;
};

static UIThemeColors GetCurrentThemeColors() {
    UIThemeColors c;
    bool isDark = true;
    if (sAIUITheme) {
        isDark = sAIUITheme->IsUIThemeDark() != 0;
    }
    c.isDark = isDark;

    if (isDark) {
        // Dark theme: matches native Illustrator panel (Properties, Edit Artboards)
        c.panelBg       = RGB(53, 53, 53);      // #353535
        c.textPrimary   = RGB(235, 235, 235);   // #ebebeb
        c.textSecondary = RGB(160, 160, 160);   // #a0a0a0
        c.btnBg         = RGB(68, 68, 68);      // #444444 (like Edit Artboards)
        c.btnBgHover    = RGB(84, 84, 84);      // #545454
        c.btnBgPressed  = RGB(50, 50, 50);      // #323232
        c.btnBorder     = RGB(92, 92, 92);      // #5c5c5c
        c.btnText       = RGB(235, 235, 235);   // #ebebeb
        c.accentBg      = RGB(20, 115, 230);    // #1473e6 Adobe Blue
        c.accentText    = RGB(255, 255, 255);
        c.statusBg      = RGB(42, 42, 42);      // #2a2a2a
        c.statusBorder  = RGB(70, 70, 70);      // #464646
        c.statusText    = RGB(215, 215, 215);   // #d7d7d7
        c.linkColor     = RGB(140, 185, 235);   // #8cb9eb
    } else {
        // Light theme: matches native Illustrator panel (Properties, Edit Artboards)
        c.panelBg       = RGB(240, 240, 240);   // #f0f0f0
        c.textPrimary   = RGB(32, 32, 32);      // #202020
        c.textSecondary = RGB(115, 115, 115);   // #737373
        c.btnBg         = RGB(255, 255, 255);   // #ffffff (like Edit Artboards)
        c.btnBgHover    = RGB(240, 240, 240);   // #f0f0f0
        c.btnBgPressed  = RGB(220, 220, 220);   // #dcdcdc
        c.btnBorder     = RGB(175, 175, 175);   // #afafaf
        c.btnText       = RGB(32, 32, 32);      // #202020
        c.accentBg      = RGB(14, 100, 215);    // #0e64d7
        c.accentText    = RGB(255, 255, 255);
        c.statusBg      = RGB(230, 230, 230);   // #e6e6e6
        c.statusBorder  = RGB(195, 195, 195);   // #c3c3c3
        c.statusText    = RGB(35, 35, 35);      // #232323
        c.linkColor     = RGB(30, 100, 200);    // #1e64c8
    }

    if (sAIUITheme) {
        AIUIThemeColor colBg;
        if (sAIUITheme->GetUIThemeColor(kAIUIThemeSelectorPanel, kAIUIComponentColorBackground, colBg) == kNoErr) {
            c.panelBg = RGB((int)(colBg.red * 255.0f + 0.5f),
                            (int)(colBg.green * 255.0f + 0.5f),
                            (int)(colBg.blue * 255.0f + 0.5f));
        }
        AIUIThemeColor colText;
        if (sAIUITheme->GetUIThemeColor(kAIUIThemeSelectorPanel, kAIUIComponentColorText, colText) == kNoErr) {
            c.textPrimary = RGB((int)(colText.red * 255.0f + 0.5f),
                                (int)(colText.green * 255.0f + 0.5f),
                                (int)(colText.blue * 255.0f + 0.5f));
            c.btnText = c.textPrimary;
        }
    }

    return c;
}

static LRESULT CALLBACK CustomButtonProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_MOUSEMOVE: {
            bool isHover = GetPropA(hwnd, "CP_Hover") != NULL;
            if (!isHover) {
                SetPropA(hwnd, "CP_Hover", (HANDLE)1);
                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
                TrackMouseEvent(&tme);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }
        case WM_MOUSELEAVE: {
            RemovePropA(hwnd, "CP_Hover");
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rc;
            GetClientRect(hwnd, &rc);

            UIThemeColors theme = GetCurrentThemeColors();

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBM = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
            HGDIOBJ oldBM = SelectObject(memDC, memBM);

            bool isEnabled = IsWindowEnabled(hwnd) != FALSE;
            bool isPushed = (SendMessage(hwnd, BM_GETSTATE, 0, 0) & BST_PUSHED) != 0;
            bool isHover = GetPropA(hwnd, "CP_Hover") != NULL;

            bool isClipboardTab = (hwnd == g_hTabBtnClipboard);
            bool isOffsetTab = (hwnd == g_hTabBtnOffset);
            bool isTabSelected = (isClipboardTab && ClipboardPastePlugin::GetInstance().GetActiveTab() == 0) ||
                                 (isOffsetTab && ClipboardPastePlugin::GetInstance().GetActiveTab() == 1);

            COLORREF fillCol;
            COLORREF borderCol;
            COLORREF textCol;

            if (isTabSelected) {
                fillCol = theme.accentBg;
                borderCol = theme.accentBg;
                textCol = theme.accentText;
            } else {
                fillCol = !isEnabled ? theme.panelBg : (isPushed ? theme.btnBgPressed : (isHover ? theme.btnBgHover : theme.btnBg));
                borderCol = !isEnabled ? theme.statusBorder : theme.btnBorder;
                textCol = !isEnabled ? theme.textSecondary : theme.btnText;
            }

            HBRUSH hPanelBg = CreateSolidBrush(theme.panelBg);
            FillRect(memDC, &rc, hPanelBg);
            DeleteObject(hPanelBg);

            HPEN hPen = CreatePen(PS_SOLID, 1, borderCol);
            HBRUSH hBrush = CreateSolidBrush(fillCol);
            HGDIOBJ oldPen = SelectObject(memDC, hPen);
            HGDIOBJ oldBrush = SelectObject(memDC, hBrush);

            float scale = GetPanelScale(hwnd);
            int corner = (int)(3.0f * scale + 0.5f);
            if (corner < 2) corner = 2;
            RoundRect(memDC, rc.left, rc.top, rc.right, rc.bottom, corner * 2, corner * 2);

            SelectObject(memDC, oldPen);
            SelectObject(memDC, oldBrush);
            DeleteObject(hPen);
            DeleteObject(hBrush);

            wchar_t wtext[128] = {0};
            GetWindowTextW(hwnd, wtext, sizeof(wtext)/sizeof(wchar_t));

            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, textCol);
            SelectObject(memDC, g_hUIFont);

            RECT textRc = rc;
            if (isPushed && !isTabSelected) {
                OffsetRect(&textRc, 0, 1);
            }
            DrawTextW(memDC, wtext, -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            BitBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBM);
            DeleteObject(memBM);
            DeleteDC(memDC);

            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_NCDESTROY: {
            RemovePropA(hwnd, "CP_Hover");
            break;
        }
    }

    if (g_oldButtonProc) {
        return CallWindowProc(g_oldButtonProc, hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK CustomStatusProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_ERASEBKGND) return 1;
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        UIThemeColors theme = GetCurrentThemeColors();

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
        HGDIOBJ oldBM = SelectObject(memDC, memBM);

        HBRUSH hPanelBg = CreateSolidBrush(theme.panelBg);
        FillRect(memDC, &rc, hPanelBg);
        DeleteObject(hPanelBg);

        HPEN hPen = CreatePen(PS_SOLID, 1, theme.statusBorder);
        HBRUSH hBrush = CreateSolidBrush(theme.statusBg);
        HGDIOBJ oldPen = SelectObject(memDC, hPen);
        HGDIOBJ oldBrush = SelectObject(memDC, hBrush);

        float scale = GetPanelScale(hwnd);
        int corner = (int)(3.0f * scale + 0.5f);
        if (corner < 2) corner = 2;
        RoundRect(memDC, rc.left, rc.top, rc.right, rc.bottom, corner * 2, corner * 2);

        SelectObject(memDC, oldPen);
        SelectObject(memDC, oldBrush);
        DeleteObject(hPen);
        DeleteObject(hBrush);

        char text[256] = {0};
        GetWindowTextA(hwnd, text, sizeof(text));

        SetBkMode(memDC, TRANSPARENT);

        if (hwnd == g_hStaticOffsetModHeader) {
            SetTextColor(memDC, theme.textPrimary);
            SelectObject(memDC, g_hHeaderFont);
            DrawTextA(memDC, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        } else if (hwnd == g_hStaticOffsetInfo) {
            SetTextColor(memDC, theme.statusText);
            SelectObject(memDC, g_hUIFont);
            DrawTextA(memDC, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        } else {
            SetTextColor(memDC, theme.statusText);
            SelectObject(memDC, g_hUIFont);
            int padX = (int)(6.0f * scale + 0.5f);
            int padY = (int)(2.0f * scale + 0.5f);
            RECT textRc = rc;
            textRc.left += padX;
            textRc.top += padY;
            textRc.right -= padX;
            textRc.bottom -= padY;
            DrawTextA(memDC, text, -1, &textRc, DT_LEFT | DT_WORDBREAK);
        }

        BitBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldBM);
        DeleteObject(memBM);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        return 0;
    }
    if (g_oldStatusProc) {
        return CallWindowProc(g_oldStatusProc, hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void RelayoutPanel(HWND hwnd) {
    if (!hwnd) return;
    RECT rc;
    GetClientRect(hwnd, &rc);
    int width = rc.right - rc.left;
    if (width <= 0) return;

    float scale = GetPanelScale(hwnd);
    EnsureFontsForScale(hwnd, scale);

    auto S = [scale](int v) -> int {
        return (int)(v * scale + 0.5f);
    };

    int pad = S(8);
    int availW = (std::max)(S(100), width - (pad * 2));
    int btnH = S(22);

    int y = S(6);
    // Header row: Title (left) & Version (right)
    if (g_hStaticHeaderTitle && g_hStaticHeaderVersion) {
        int verW = S(45);
        int titleW = (std::max)(S(60), availW - verW);
        SetWindowPos(g_hStaticHeaderTitle, NULL, pad, y, titleW, S(18), SWP_NOZORDER);
        SetWindowPos(g_hStaticHeaderVersion, NULL, pad + availW - verW, y + S(1), verW, S(16), SWP_NOZORDER);
        y += S(18) + S(4);
    }

    // Tabs row (always visible)
    if (g_hTabBtnClipboard && g_hTabBtnOffset) {
        int tabW = (availW - S(4)) / 2;
        int tabH = S(20);
        SetWindowPos(g_hTabBtnClipboard, NULL, pad, y, tabW, tabH, SWP_NOZORDER);
        SetWindowPos(g_hTabBtnOffset, NULL, pad + tabW + S(4), y, tabW, tabH, SWP_NOZORDER);
        y += tabH + S(6);
    }

    int activeTab = ClipboardPastePlugin::GetInstance().GetActiveTab();
    if (activeTab == 0) {
        // Tab 0: Clipboard
        if (g_hBtnCopy) {
            SetWindowPos(g_hBtnCopy, NULL, pad, y, availW, btnH, SWP_NOZORDER);
            y += btnH + S(3);
        }
        if (g_hBtnPasteInPlace) {
            SetWindowPos(g_hBtnPasteInPlace, NULL, pad, y, availW, btnH, SWP_NOZORDER);
            y += btnH + S(3);
        }
        if (g_hBtnPasteCenter) {
            SetWindowPos(g_hBtnPasteCenter, NULL, pad, y, availW, btnH, SWP_NOZORDER);
            y += btnH + S(3);
        }
        if (g_hBtnPasteAllArtboards) {
            SetWindowPos(g_hBtnPasteAllArtboards, NULL, pad, y, availW, btnH, SWP_NOZORDER);
            y += btnH + S(3);
        }
        if (g_hBtnReflectVert && g_hBtnReflectHoriz) {
            int halfW = (availW - S(4)) / 2;
            SetWindowPos(g_hBtnReflectVert, NULL, pad, y, halfW, btnH, SWP_NOZORDER);
            SetWindowPos(g_hBtnReflectHoriz, NULL, pad + halfW + S(4), y, halfW, btnH, SWP_NOZORDER);
            y += btnH + S(4);
        }
        if (g_hBtnClear && g_hBtnSettings) {
            int halfW = (availW - S(4)) / 2;
            int smallBtnH = S(20);
            SetWindowPos(g_hBtnClear, NULL, pad, y, halfW, smallBtnH, SWP_NOZORDER);
            SetWindowPos(g_hBtnSettings, NULL, pad + halfW + S(4), y, halfW, smallBtnH, SWP_NOZORDER);
            y += smallBtnH + S(5);
        } else if (g_hBtnClear) {
            int clearW = (std::min)(S(110), availW);
            SetWindowPos(g_hBtnClear, NULL, pad, y, clearW, S(20), SWP_NOZORDER);
            y += S(20) + S(5);
        }
        if (g_hStaticStatus) {
            int statusH = S(32);
            SetWindowPos(g_hStaticStatus, NULL, pad, y, availW, statusH, SWP_NOZORDER);
            y += statusH + S(5);
        }
    } else {
        // Tab 1: Offset Clone
        if (g_hStaticOffsetInfo) {
            int infoH = S(22);
            SetWindowPos(g_hStaticOffsetInfo, NULL, pad, y, availW, infoH, SWP_NOZORDER);
            y += infoH + S(3);
        }

        if (g_hStaticOffsetModHeader) {
            int modH = S(20);
            SetWindowPos(g_hStaticOffsetModHeader, NULL, pad, y, availW, modH, SWP_NOZORDER);
            y += modH + S(4);
        }

        // 3x3 Keypad Grid
        int gap = S(3);
        int colW = (availW - gap * 2) / 3;
        int gridRowH = S(22);

        int grid[3][3] = {
            { 7, 8, 9 },
            { 4, 5, 6 },
            { 1, 2, 3 }
        };

        for (int r = 0; r < 3; ++r) {
            int rowY = y + r * (gridRowH + gap);
            for (int c = 0; c < 3; ++c) {
                int dir = grid[r][c];
                int colX = pad + c * (colW + gap);
                if (g_hBtnDir[dir]) {
                    SetWindowPos(g_hBtnDir[dir], NULL, colX, rowY, colW, gridRowH, SWP_NOZORDER);
                }
            }
        }
        y += 3 * gridRowH + 2 * gap + S(4);

        if (g_hBtnOffsetSettings) {
            int smallBtnH = S(20);
            SetWindowPos(g_hBtnOffsetSettings, NULL, pad, y, availW, smallBtnH, SWP_NOZORDER);
            y += smallBtnH + S(4);
        }

        if (g_hStaticOffsetStatus) {
            int statusH = S(38);
            SetWindowPos(g_hStaticOffsetStatus, NULL, pad, y, availW, statusH, SWP_NOZORDER);
            y += statusH + S(5);
        }
    }

    // Footer links: single line, S(16) high
    if (g_hStaticLinkAuthor) {
        SetWindowPos(g_hStaticLinkAuthor, NULL, pad, y, availW, S(16), SWP_NOZORDER);
        y += S(16) + S(2);
    }
    if (g_hStaticLinkTools) {
        SetWindowPos(g_hStaticLinkTools, NULL, pad, y, availW, S(16), SWP_NOZORDER);
    }
}

static void AIAPI PanelVisibilityChangedNotifyProc(AIPanelRef inPanel, AIBoolean isVisible) {
    PluginLog("[PanelVisibilityChanged] isVisible=%d", isVisible);
    ClipboardPastePlugin::GetInstance().OnVisibilityChanged(isVisible != 0);
}

static void AIAPI PanelSizeChangedNotifyProc(AIPanelRef inPanel) {
    HWND hDlg = ClipboardPastePlugin::GetInstance().GetPanelHWND();
    if (hDlg) {
        RelayoutPanel(hDlg);
    }
}

static void AIAPI PanelClosedNotifyProc(AIPanelRef inPanel) {
    PluginLog("[PanelClosedNotifyProc]");
    ClipboardPastePlugin::GetInstance().OnVisibilityChanged(false);
}

static LRESULT CALLBACK PanelWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            PluginLog("[PanelWndProc] WM_COMMAND id=%d", wmId);
            SPPluginRef pluginRef = ClipboardPastePlugin::GetInstance().GetPluginRef();
            switch (wmId) {
                case IDC_TAB_CLIPBOARD: {
                    ClipboardPastePlugin::GetInstance().SwitchTab(0);
                    return 0;
                }
                case IDC_TAB_OFFSET: {
                    ClipboardPastePlugin::GetInstance().SwitchTab(1);
                    return 0;
                }
                case IDC_BTN_COPY: {
                    AppContext appContext(pluginRef);
                    BufferManager::GetInstance().CopySelection();
                    ClipboardPastePlugin::GetInstance().UpdateUI();
                    return 0;
                }
                case IDC_BTN_PASTE_IN_PLACE: {
                    AppContext appContext(pluginRef);
                    BufferManager::GetInstance().PasteInPlace();
                    ClipboardPastePlugin::GetInstance().UpdateUI();
                    return 0;
                }
                case IDC_BTN_PASTE_CENTER: {
                    AppContext appContext(pluginRef);
                    BufferManager::GetInstance().PasteCenter();
                    ClipboardPastePlugin::GetInstance().UpdateUI();
                    return 0;
                }
                case IDC_BTN_PASTE_ALL_ARTBOARDS: {
                    AppContext appContext(pluginRef);
                    BufferManager::GetInstance().PasteAllArtboards();
                    ClipboardPastePlugin::GetInstance().UpdateUI();
                    return 0;
                }
                case IDC_BTN_REFLECT_VERT: {
                    AppContext appContext(pluginRef);
                    BufferManager::GetInstance().ReflectSelectionVertical();
                    return 0;
                }
                case IDC_BTN_REFLECT_HORIZ: {
                    AppContext appContext(pluginRef);
                    BufferManager::GetInstance().ReflectSelectionHorizontal();
                    return 0;
                }
                case IDC_BTN_CLEAR: {
                    AppContext appContext(pluginRef);
                    BufferManager::GetInstance().Clear();
                    ClipboardPastePlugin::GetInstance().UpdateUI();
                    return 0;
                }
                case IDC_BTN_SETTINGS: {
                    float scale = GetPanelScale(hwnd);
                    if (ConfigManager::GetInstance().ShowSettingsDialog(hwnd, scale)) {
                        ClipboardPastePlugin::GetInstance().UpdateButtonsText();
                        ClipboardPastePlugin::GetInstance().UpdateUI();
                        ClipboardPastePlugin::GetInstance().UpdateOffsetUI();
                    }
                    return 0;
                }
                case IDC_BTN_DIR_1:
                case IDC_BTN_DIR_2:
                case IDC_BTN_DIR_3:
                case IDC_BTN_DIR_4:
                case IDC_BTN_DIR_5:
                case IDC_BTN_DIR_6:
                case IDC_BTN_DIR_7:
                case IDC_BTN_DIR_8:
                case IDC_BTN_DIR_9: {
                    int dir = wmId - IDC_BTN_DIR_1 + 1;
                    AppContext appContext(pluginRef);
                    double gap = ConfigManager::GetInstance().GetOffsetCloneGap();
                    bool isAlt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                    if (isAlt) {
                        if (BufferManager::GetInstance().MoveSelectionWithOffset(dir, gap)) {
                            ClipboardPastePlugin::GetInstance().UpdateOffsetUI(dir, true);
                        }
                    } else {
                        if (BufferManager::GetInstance().CloneSelectionWithOffset(dir, gap)) {
                            ClipboardPastePlugin::GetInstance().UpdateOffsetUI(dir, false);
                        }
                    }
                    return 0;
                }
                case IDC_BTN_OFFSET_SETTINGS: {
                    float scale = GetPanelScale(hwnd);
                    if (ConfigManager::GetInstance().ShowSettingsDialog(hwnd, scale)) {
                        ClipboardPastePlugin::GetInstance().UpdateButtonsText();
                        ClipboardPastePlugin::GetInstance().UpdateUI();
                        ClipboardPastePlugin::GetInstance().UpdateOffsetUI();
                    }
                    return 0;
                }
                case IDC_STATIC_LINK_AUTHOR: {
                    ShellExecuteA(NULL, "open", "https://github.com/SaidAuita/AI-Clipboard-Paste", NULL, NULL, SW_SHOWNORMAL);
                    return 0;
                }
                case IDC_STATIC_LINK_TOOLS: {
                    ShellExecuteA(NULL, "open", "https://ph-cu-s.com/tools", NULL, NULL, SW_SHOWNORMAL);
                    return 0;
                }
            }
            break;
        }
        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT) {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hwnd, &pt);
                HWND hChild = ChildWindowFromPoint(hwnd, pt);
                if (hChild == g_hStaticLinkAuthor || hChild == g_hStaticLinkTools) {
                    SetCursor(LoadCursor(NULL, IDC_HAND));
                    return TRUE;
                }
            }
            break;
        }
        case WM_DPICHANGED:
            RelayoutPanel(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;

        case WM_SIZE:
            RelayoutPanel(hwnd);
            break;

        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            HWND hCtrl = (HWND)lParam;
            UIThemeColors theme = GetCurrentThemeColors();

            SetBkMode(hdc, TRANSPARENT);
            if (hCtrl == g_hStaticHeaderTitle) {
                SetTextColor(hdc, theme.textPrimary);
            } else if (hCtrl == g_hStaticHeaderVersion) {
                SetTextColor(hdc, theme.textSecondary);
            } else if (hCtrl == g_hStaticLinkAuthor || hCtrl == g_hStaticLinkTools) {
                SetTextColor(hdc, theme.linkColor);
            } else {
                SetTextColor(hdc, theme.textPrimary);
            }

            if (g_hPanelBgBrush) {
                DeleteObject(g_hPanelBgBrush);
            }
            g_hPanelBgBrush = CreateSolidBrush(theme.panelBg);
            return (LRESULT)g_hPanelBgBrush;
        }

        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hwnd, &rc);
            UIThemeColors theme = GetCurrentThemeColors();
            if (g_hPanelBgBrush) {
                DeleteObject(g_hPanelBgBrush);
            }
            g_hPanelBgBrush = CreateSolidBrush(theme.panelBg);
            FillRect(hdc, &rc, g_hPanelBgBrush);
            return 1;
        }

        case WM_DESTROY:
            g_oldPanelProc = NULL;
            g_hStaticHeaderTitle = NULL;
            g_hStaticHeaderVersion = NULL;
            g_hTabBtnClipboard = NULL;
            g_hTabBtnOffset = NULL;
            g_hBtnCopy = NULL;
            g_hBtnPasteInPlace = NULL;
            g_hBtnPasteCenter = NULL;
            g_hBtnPasteAllArtboards = NULL;
            g_hBtnReflectVert = NULL;
            g_hBtnReflectHoriz = NULL;
            g_hBtnClear = NULL;
            g_hBtnSettings = NULL;
            g_hStaticStatus = NULL;
            g_hStaticOffsetInfo = NULL;
            g_hStaticOffsetModHeader = NULL;
            for (int i = 1; i <= 9; ++i) g_hBtnDir[i] = NULL;
            g_hStaticOffsetStatus = NULL;
            g_hBtnOffsetSettings = NULL;
            g_hStaticLinkAuthor = NULL;
            g_hStaticLinkTools = NULL;
            break;
    }

    if (g_oldPanelProc) {
        return CallWindowProc(g_oldPanelProc, hwnd, msg, wParam, lParam);
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == PM_REMOVE) {
        if (ConfigManager::GetInstance().IsDialogActive() || ConfigManager::GetInstance().IsPluginDisabled()) {
            return CallNextHookEx(g_hKeyHook, nCode, wParam, lParam);
        }
        MSG* pMsg = reinterpret_cast<MSG*>(lParam);
        if (pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) {
            char cls[32] = {0};
            GetClassNameA(pMsg->hwnd, cls, sizeof(cls));
            bool isEdit = (_stricmp(cls, "Edit") == 0 || _stricmp(cls, "RichEdit") == 0);

            if (!isEdit) {
                UINT vk = (UINT)pMsg->wParam;
                UINT mod = 0;
                if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) mod |= MOD_CONTROL;
                if ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0)    mod |= MOD_ALT;
                if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0)   mod |= MOD_SHIFT;

                auto& cfg = ConfigManager::GetInstance();
                if (cfg.MatchHotkey(cfg.GetCopyHotkey(), vk, mod)) {
                    SPPluginRef pluginRef = ClipboardPastePlugin::GetInstance().GetPluginRef();
                    AppContext appContext(pluginRef);
                    PluginLog("[HotKey Hook] Intercepted Copy Hotkey -> CopySelection()");
                    BufferManager::GetInstance().CopySelection();
                    ClipboardPastePlugin::GetInstance().UpdateUI();
                    pMsg->message = WM_NULL;
                    return 1;
                } else if (cfg.MatchHotkey(cfg.GetPastePlaceHotkey(), vk, mod)) {
                    SPPluginRef pluginRef = ClipboardPastePlugin::GetInstance().GetPluginRef();
                    AppContext appContext(pluginRef);
                    PluginLog("[HotKey Hook] Intercepted PastePlace Hotkey -> PasteInPlace()");
                    BufferManager::GetInstance().PasteInPlace();
                    ClipboardPastePlugin::GetInstance().UpdateUI();
                    pMsg->message = WM_NULL;
                    return 1;
                } else if (cfg.MatchHotkey(cfg.GetPasteCenterHotkey(), vk, mod)) {
                    SPPluginRef pluginRef = ClipboardPastePlugin::GetInstance().GetPluginRef();
                    AppContext appContext(pluginRef);
                    PluginLog("[HotKey Hook] Intercepted PasteCenter Hotkey -> PasteCenter()");
                    BufferManager::GetInstance().PasteCenter();
                    ClipboardPastePlugin::GetInstance().UpdateUI();
                    pMsg->message = WM_NULL;
                    return 1;
                } else if (cfg.MatchHotkey(cfg.GetPasteAllArtboardsHotkey(), vk, mod)) {
                    SPPluginRef pluginRef = ClipboardPastePlugin::GetInstance().GetPluginRef();
                    AppContext appContext(pluginRef);
                    PluginLog("[HotKey Hook] Intercepted PasteAllArtboards Hotkey -> PasteAllArtboards()");
                    BufferManager::GetInstance().PasteAllArtboards();
                    ClipboardPastePlugin::GetInstance().UpdateUI();
                    pMsg->message = WM_NULL;
                    return 1;
                } else if (cfg.MatchHotkey(cfg.GetReflectVertHotkey(), vk, mod)) {
                    SPPluginRef pluginRef = ClipboardPastePlugin::GetInstance().GetPluginRef();
                    AppContext appContext(pluginRef);
                    PluginLog("[HotKey Hook] Intercepted Reflect Vertical Hotkey -> ReflectSelectionVertical()");
                    BufferManager::GetInstance().ReflectSelectionVertical();
                    pMsg->message = WM_NULL;
                    return 1;
                } else if (cfg.MatchHotkey(cfg.GetReflectHorizHotkey(), vk, mod)) {
                    SPPluginRef pluginRef = ClipboardPastePlugin::GetInstance().GetPluginRef();
                    AppContext appContext(pluginRef);
                    PluginLog("[HotKey Hook] Intercepted Reflect Horizontal Hotkey -> ReflectSelectionHorizontal()");
                    BufferManager::GetInstance().ReflectSelectionHorizontal();
                    pMsg->message = WM_NULL;
                    return 1;
                } else if (cfg.IsOffsetCloneHotkeysEnabled() && (mod == cfg.GetOffsetCloneModifier() || mod == cfg.GetOffsetMoveModifier())) {
                    bool isMove = (mod == cfg.GetOffsetMoveModifier() && mod != cfg.GetOffsetCloneModifier());
                    int dir = 0;
                    int preset = cfg.GetOffsetKeyPreset();

                    if (preset == OFFSET_PRESET_NUMPAD) {
                        bool isExtended = (HIWORD(pMsg->lParam) & KF_EXTENDED) != 0;
                        if (vk >= VK_NUMPAD1 && vk <= VK_NUMPAD9) {
                            dir = vk - VK_NUMPAD1 + 1;
                        } else if (!isExtended) {
                            switch (vk) {
                                case VK_END:   dir = 1; break; // Num 1
                                case VK_DOWN:  dir = 2; break; // Num 2
                                case VK_NEXT:  dir = 3; break; // Num 3 (PgDn)
                                case VK_LEFT:  dir = 4; break; // Num 4
                                case VK_CLEAR: dir = 5; break; // Num 5
                                case VK_RIGHT: dir = 6; break; // Num 6
                                case VK_HOME:  dir = 7; break; // Num 7
                                case VK_UP:    dir = 8; break; // Num 8
                                case VK_PRIOR: dir = 9; break; // Num 9 (PgUp)
                            }
                        }
                    } else {
                        // Top Row, QWE/ASD/ZXC, UIO/JKL/M, Custom
                        for (int i = 1; i <= 9; ++i) {
                            if (cfg.GetOffsetKey(i) == vk) {
                                dir = i;
                                break;
                            }
                        }
                    }

                    if (dir >= 1 && dir <= 9) {
                        SPPluginRef pluginRef = ClipboardPastePlugin::GetInstance().GetPluginRef();
                        AppContext appContext(pluginRef);
                        double gap = cfg.GetOffsetCloneGap();
                        if (isMove) {
                            PluginLog("[HotKey Hook] Intercepted Offset Move Hotkey -> Dir=%d", dir);
                            if (BufferManager::GetInstance().MoveSelectionWithOffset(dir, gap)) {
                                ClipboardPastePlugin::GetInstance().UpdateOffsetUI(dir, true);
                            }
                        } else {
                            PluginLog("[HotKey Hook] Intercepted Offset Clone Hotkey -> Dir=%d", dir);
                            if (BufferManager::GetInstance().CloneSelectionWithOffset(dir, gap)) {
                                ClipboardPastePlugin::GetInstance().UpdateOffsetUI(dir, false);
                            }
                        }
                        pMsg->message = WM_NULL;
                        return 1;
                    }
                }
            }
        }
    }
    return CallNextHookEx(g_hKeyHook, nCode, wParam, lParam);
}

ClipboardPastePlugin& ClipboardPastePlugin::GetInstance() {
    static ClipboardPastePlugin instance;
    return instance;
}

ClipboardPastePlugin::ClipboardPastePlugin()
    : m_pluginRef(nullptr),
      m_panelRef(nullptr),
      m_hPanelDlg(NULL),
      m_activeTab(0),
      m_hBrightnessNotifier(nullptr),
      m_hMenuWindow(nullptr),
      m_hMenuCopy(nullptr),
      m_hMenuPasteInPlace(nullptr),
      m_hMenuPasteCenter(nullptr),
      m_hMenuPasteAllArtboards(nullptr),
      m_hMenuReflectVert(nullptr),
      m_hMenuReflectHoriz(nullptr),
      m_hMenuClear(nullptr),
      m_hMenuAbout(nullptr) {}

ClipboardPastePlugin::~ClipboardPastePlugin() {}

SPErr ClipboardPastePlugin::StartupPlugin(void* message) {
    PluginLog("[ClipboardPastePlugin] StartupPlugin starting...");

    // 1. Create official dockable panel
    CreateDockablePanel();

    // 2. Add menu commands
    AddMenuItems();

    // 3. Register theme change notifier
    if (sAINotifier) {
        sAINotifier->AddNotifier(m_pluginRef, "ClipboardPastePlugin",
                                 kAIUIBrightnessChangedNotifier, &m_hBrightnessNotifier);
    }

    // 4. Install thread-level keyboard hook for F5, F6, F7
    if (!g_hKeyHook) {
        g_hKeyHook = SetWindowsHookExW(WH_GETMESSAGE, KeyboardHookProc, NULL, GetCurrentThreadId());
        PluginLog("  [SetWindowsHookExW] Keyboard hook installed: %p (threadId=%u)",
                  g_hKeyHook, GetCurrentThreadId());
    }

    UpdateUI();
    return kSPNoError;
}

SPErr ClipboardPastePlugin::Notify(void* message) {
    if (!message) return 1;
    AINotifierMessage* notifMsg = static_cast<AINotifierMessage*>(message);
    if (notifMsg->notifier == m_hBrightnessNotifier) {
        PluginLog("[ClipboardPastePlugin::Notify] UI Brightness changed!");
        if (m_hPanelDlg) {
            InvalidateRect(m_hPanelDlg, NULL, TRUE);
            EnumChildWindows(m_hPanelDlg, [](HWND hChild, LPARAM) -> BOOL {
                InvalidateRect(hChild, NULL, TRUE);
                return TRUE;
            }, 0);
        }
    }
    return kNoErr;
}

SPErr ClipboardPastePlugin::ShutdownPlugin(void* message) {
    PluginLog("[ClipboardPastePlugin] ShutdownPlugin starting...");

    if (g_hKeyHook) {
        UnhookWindowsHookEx(g_hKeyHook);
        g_hKeyHook = NULL;
    }

    if (m_panelRef && sAIPanel) {
        sAIPanel->Destroy(m_panelRef);
        m_panelRef = nullptr;
    }

    if (g_hUIFont) {
        DeleteObject(g_hUIFont);
        g_hUIFont = NULL;
    }
    if (g_hHeaderFont) {
        DeleteObject(g_hHeaderFont);
        g_hHeaderFont = NULL;
    }
    if (g_hSmallFont) {
        DeleteObject(g_hSmallFont);
        g_hSmallFont = NULL;
    }
    if (g_hPanelBgBrush) {
        DeleteObject(g_hPanelBgBrush);
        g_hPanelBgBrush = NULL;
    }

    BufferManager::GetInstance().Clear();
    return kSPNoError;
}

SPErr ClipboardPastePlugin::CreateDockablePanel() {
    if (!sAIPanel) {
        PluginLog("[CreateDockablePanel FAIL] sAIPanel is NULL!");
        return 1;
    }

    AISize minSize = { 100, 100 };
    AIErr err = sAIPanel->Create(
        m_pluginRef,
        ai::UnicodeString("ClipboardPastePanel"),
        ai::UnicodeString("Clipboard Paste"),
        1,
        minSize,
        true,
        nullptr,
        this,
        m_panelRef
    );

    PluginLog("[CreateDockablePanel] sAIPanel->Create result: err=%d, panelRef=%p", err, m_panelRef);
    if (err != kNoErr || !m_panelRef) {
        return err;
    }

    AISize minS = { 100, 100 };
    AISize maxS = { 1200, 1200 };
    AISize prefConstS = { 190, 320 };
    AISize prefUnconstS = { 210, 330 };
    sAIPanel->SetSizes(m_panelRef, minS, prefUnconstS, prefConstS, maxS);

    // Set panel dock SVG icon (light & dark theme)
    ai::uint32 iconResID = 16300;
    ai::uint32 darkIconResID = 16301;
    sAIPanel->SetSVGIconResourceID(m_panelRef, iconResID, darkIconResID);

    sAIPanel->SetVisibilityChangedNotifyProc(m_panelRef, PanelVisibilityChangedNotifyProc);
    sAIPanel->SetSizeChangedNotifyProc(m_panelRef, PanelSizeChangedNotifyProc);
    sAIPanel->SetClosedNotifyProc(m_panelRef, PanelClosedNotifyProc);
    sAIPanel->Show(m_panelRef, true);

    AIPanelPlatformWindow platWin = nullptr;
    err = sAIPanel->GetPlatformWindow(m_panelRef, platWin);
    m_hPanelDlg = (HWND)platWin;
    PluginLog("[CreateDockablePanel] GetPlatformWindow: err=%d, HWND=%p", err, m_hPanelDlg);

    if (m_hPanelDlg) {
        SetWindowLongPtr(m_hPanelDlg, GWL_STYLE, GetWindowLongPtr(m_hPanelDlg, GWL_STYLE) | WS_CLIPCHILDREN);
        HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(m_hPanelDlg, GWLP_HINSTANCE);
        if (!hInst) hInst = GetModuleHandle(NULL);

        // Header controls: Title and Version
        g_hStaticHeaderTitle = CreateWindowExA(
            0, "STATIC", "Clipboard Paste",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            0, 0, 100, 20, m_hPanelDlg, (HMENU)IDC_STATIC_TITLE, hInst, NULL);

        g_hStaticHeaderVersion = CreateWindowExA(
            0, "STATIC", PLUGIN_VERSION_STR,
            WS_VISIBLE | WS_CHILD | SS_RIGHT,
            0, 0, 50, 20, m_hPanelDlg, (HMENU)IDC_STATIC_VERSION, hInst, NULL);

        // Tabs
        g_hTabBtnClipboard = CreateWindowExW(
            0, L"BUTTON", L"Clipboard",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            0, 0, 50, 20, m_hPanelDlg, (HMENU)IDC_TAB_CLIPBOARD, hInst, NULL);

        g_hTabBtnOffset = CreateWindowExW(
            0, L"BUTTON", L"Offset Clone",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            0, 0, 50, 20, m_hPanelDlg, (HMENU)IDC_TAB_OFFSET, hInst, NULL);

        // Tab 0 controls
        g_hBtnCopy = CreateWindowExA(
            0, "BUTTON", "Copy to Buffer (F5)",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            0, 0, 100, 24, m_hPanelDlg, (HMENU)IDC_BTN_COPY, hInst, NULL);

        g_hBtnPasteInPlace = CreateWindowExA(
            0, "BUTTON", "Paste In Place (F6)",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            0, 0, 100, 24, m_hPanelDlg, (HMENU)IDC_BTN_PASTE_IN_PLACE, hInst, NULL);

        g_hBtnPasteCenter = CreateWindowExA(
            0, "BUTTON", "Paste to Center (F7)",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            0, 0, 100, 24, m_hPanelDlg, (HMENU)IDC_BTN_PASTE_CENTER, hInst, NULL);

        g_hBtnPasteAllArtboards = CreateWindowExA(
            0, "BUTTON", "Paste to All Artboards (F8)",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            0, 0, 100, 24, m_hPanelDlg, (HMENU)IDC_BTN_PASTE_ALL_ARTBOARDS, hInst, NULL);

        g_hBtnReflectVert = CreateWindowExA(
            0, "BUTTON", "Reflect V (F9)",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            0, 0, 50, 24, m_hPanelDlg, (HMENU)IDC_BTN_REFLECT_VERT, hInst, NULL);

        g_hBtnReflectHoriz = CreateWindowExA(
            0, "BUTTON", "Reflect H (F10)",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            0, 0, 50, 24, m_hPanelDlg, (HMENU)IDC_BTN_REFLECT_HORIZ, hInst, NULL);

        g_hBtnClear = CreateWindowExA(
            0, "BUTTON", "Clear Buffer",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            0, 0, 50, 20, m_hPanelDlg, (HMENU)IDC_BTN_CLEAR, hInst, NULL);

        g_hBtnSettings = CreateWindowExA(
            0, "BUTTON", "Settings...",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            0, 0, 50, 20, m_hPanelDlg, (HMENU)IDC_BTN_SETTINGS, hInst, NULL);

        g_hStaticStatus = CreateWindowExA(
            0, "STATIC", "Buffer Status: Empty\n(No selection copied)",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            0, 0, 100, 32, m_hPanelDlg, (HMENU)IDC_STATIC_STATUS, hInst, NULL);

        // Tab 1 controls (Offset Clone)
        g_hStaticOffsetInfo = CreateWindowExA(
            0, "STATIC", "No object selected",
            WS_CHILD | SS_CENTER,
            0, 0, 100, 22, m_hPanelDlg, (HMENU)IDC_STATIC_OFFSET_INFO, hInst, NULL);

        g_hStaticOffsetModHeader = CreateWindowExA(
            0, "STATIC", "[CTRL] Clone  |  [ALT] Move",
            WS_CHILD | SS_CENTER,
            0, 0, 100, 20, m_hPanelDlg, (HMENU)IDC_STATIC_OFFSET_MOD_HEADER, hInst, NULL);

        for (int i = 1; i <= 9; ++i) {
            g_hBtnDir[i] = CreateWindowExW(
                0, L"BUTTON", L"",
                WS_CHILD | BS_PUSHBUTTON,
                0, 0, 30, 22, m_hPanelDlg, (HMENU)(INT_PTR)(IDC_BTN_DIR_1 + i - 1), hInst, NULL);
        }

        g_hBtnOffsetSettings = CreateWindowExA(
            0, "BUTTON", "Settings...",
            WS_CHILD | BS_PUSHBUTTON,
            0, 0, 50, 20, m_hPanelDlg, (HMENU)IDC_BTN_OFFSET_SETTINGS, hInst, NULL);

        g_hStaticOffsetStatus = CreateWindowExA(
            0, "STATIC", "Shortcut: [Ctrl + Key]",
            WS_CHILD | SS_LEFT,
            0, 0, 100, 30, m_hPanelDlg, (HMENU)IDC_STATIC_OFFSET_STATUS, hInst, NULL);

        // Footer links: single line, no word wrap, end ellipsis, clickable
        g_hStaticLinkAuthor = CreateWindowExA(
            0, "STATIC", "github.com/SaidAuita/AI-Clipboard-Paste",
            WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP | SS_ENDELLIPSIS | SS_NOTIFY,
            0, 0, 100, 16, m_hPanelDlg, (HMENU)IDC_STATIC_LINK_AUTHOR, hInst, NULL);

        g_hStaticLinkTools = CreateWindowExA(
            0, "STATIC", "More scripts: ph-cu-s.com/tools",
            WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP | SS_ENDELLIPSIS | SS_NOTIFY,
            0, 0, 100, 16, m_hPanelDlg, (HMENU)IDC_STATIC_LINK_TOOLS, hInst, NULL);

        // Subclass buttons and status for native Illustrator styling and theme support
        g_oldButtonProc = (WNDPROC)GetWindowLongPtr(g_hBtnCopy, GWLP_WNDPROC);
        SetWindowLongPtr(g_hBtnCopy, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
        SetWindowLongPtr(g_hBtnPasteInPlace, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
        SetWindowLongPtr(g_hBtnPasteCenter, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
        SetWindowLongPtr(g_hBtnPasteAllArtboards, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
        SetWindowLongPtr(g_hBtnReflectVert, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
        SetWindowLongPtr(g_hBtnReflectHoriz, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
        SetWindowLongPtr(g_hBtnClear, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
        SetWindowLongPtr(g_hBtnSettings, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);

        SetWindowLongPtr(g_hTabBtnClipboard, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
        SetWindowLongPtr(g_hTabBtnOffset, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);

        for (int i = 1; i <= 9; ++i) {
            SetWindowLongPtr(g_hBtnDir[i], GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
        }
        SetWindowLongPtr(g_hBtnOffsetSettings, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);

        g_oldStatusProc = (WNDPROC)GetWindowLongPtr(g_hStaticStatus, GWLP_WNDPROC);
        SetWindowLongPtr(g_hStaticStatus, GWLP_WNDPROC, (LONG_PTR)CustomStatusProc);
        SetWindowLongPtr(g_hStaticOffsetStatus, GWLP_WNDPROC, (LONG_PTR)CustomStatusProc);
        SetWindowLongPtr(g_hStaticOffsetInfo, GWLP_WNDPROC, (LONG_PTR)CustomStatusProc);
        SetWindowLongPtr(g_hStaticOffsetModHeader, GWLP_WNDPROC, (LONG_PTR)CustomStatusProc);

        UpdateButtonsText();

        g_oldPanelProc = (WNDPROC)SetWindowLongPtr(m_hPanelDlg, GWLP_WNDPROC, (LONG_PTR)PanelWndProc);
        SwitchTab(0);
    }

    return kNoErr;
}

void ClipboardPastePlugin::OnVisibilityChanged(bool isVisible) {
    if (sAIMenu && m_hMenuWindow) {
        sAIMenu->CheckItem(m_hMenuWindow, isVisible);
    }
}

SPErr ClipboardPastePlugin::AddMenuItems() {
    if (!sAIMenu) {
        PluginLog("[AddMenuItems FAIL] sAIMenu is NULL!");
        return 1;
    }

    PluginLog("[AddMenuItems] Adding menu items (pluginRef=%p)...", m_pluginRef);

    // 1. Register in Window -> Tool Palettes
    AIPlatformAddMenuItemDataUS winItemData;
    winItemData.groupName = kToolPalettesMenuGroup;
    winItemData.itemText = ai::UnicodeString("Clipboard Paste");
    SPErr errWin = sAIMenu->AddMenuItem(m_pluginRef, "ClipboardPasteWindowPanelKey",
                                        &winItemData, kMenuItemWantsUpdateOption, &m_hMenuWindow);
    PluginLog("  -> AddMenuItem(group='%s', text='Clipboard Paste'): err=%d, handle=%p",
              kToolPalettesMenuGroup, errWin, m_hMenuWindow);

    // 2. Register quick actions under Edit -> Paste Utilities
    AIPlatformAddMenuItemDataUS copyData;
    copyData.groupName = kPasteUtilsMenuGroup;
    copyData.itemText = ai::UnicodeString("In-Place Copy to Buffer\tF5");
    sAIMenu->AddMenuItem(m_pluginRef, "ClipboardPasteCopyKey", &copyData, kMenuItemNoOptions, &m_hMenuCopy);

    AIPlatformAddMenuItemDataUS pastePlaceData;
    pastePlaceData.groupName = kPasteUtilsMenuGroup;
    pastePlaceData.itemText = ai::UnicodeString("In-Place Paste\tF6");
    sAIMenu->AddMenuItem(m_pluginRef, "ClipboardPastePlaceKey", &pastePlaceData, kMenuItemNoOptions, &m_hMenuPasteInPlace);

    AIPlatformAddMenuItemDataUS pasteCenterData;
    pasteCenterData.groupName = kPasteUtilsMenuGroup;
    pasteCenterData.itemText = ai::UnicodeString("Paste to Center of View\tF7");
    sAIMenu->AddMenuItem(m_pluginRef, "ClipboardPasteCenterKey", &pasteCenterData, kMenuItemNoOptions, &m_hMenuPasteCenter);

    AIPlatformAddMenuItemDataUS pasteAllData;
    pasteAllData.groupName = kPasteUtilsMenuGroup;
    pasteAllData.itemText = ai::UnicodeString("Paste on All Artboards\tF8");
    sAIMenu->AddMenuItem(m_pluginRef, "ClipboardPasteAllArtboardsKey", &pasteAllData, kMenuItemNoOptions, &m_hMenuPasteAllArtboards);

    AIPlatformAddMenuItemDataUS refVertData;
    refVertData.groupName = kPasteUtilsMenuGroup;
    refVertData.itemText = ai::UnicodeString("Reflect Vertical\tF9");
    sAIMenu->AddMenuItem(m_pluginRef, "ClipboardPasteReflectVertKey", &refVertData, kMenuItemNoOptions, &m_hMenuReflectVert);

    AIPlatformAddMenuItemDataUS refHorizData;
    refHorizData.groupName = kPasteUtilsMenuGroup;
    refHorizData.itemText = ai::UnicodeString("Reflect Horizontal\tF10");
    sAIMenu->AddMenuItem(m_pluginRef, "ClipboardPasteReflectHorizKey", &refHorizData, kMenuItemNoOptions, &m_hMenuReflectHoriz);

    AIPlatformAddMenuItemDataUS clearData;
    clearData.groupName = kPasteUtilsMenuGroup;
    clearData.itemText = ai::UnicodeString("Clear In-Memory Buffer");
    sAIMenu->AddMenuItem(m_pluginRef, "ClipboardPasteClearKey", &clearData, kMenuItemNoOptions, &m_hMenuClear);

    // 3. Help -> About
    AIPlatformAddMenuItemDataUS aboutData;
    aboutData.groupName = kAboutMenuGroup;
    aboutData.itemText = ai::UnicodeString("About Clipboard Paste Native...");
    sAIMenu->AddMenuItem(m_pluginRef, "ClipboardPasteAboutKey", &aboutData, kMenuItemNoOptions, &m_hMenuAbout);

    return kNoErr;
}

void ClipboardPastePlugin::SwitchTab(int tabIndex) {
    m_activeTab = tabIndex;
    int showTab0 = (tabIndex == 0) ? SW_SHOW : SW_HIDE;
    int showTab1 = (tabIndex == 1) ? SW_SHOW : SW_HIDE;

    // Tab 0 controls
    if (g_hBtnCopy) ShowWindow(g_hBtnCopy, showTab0);
    if (g_hBtnPasteInPlace) ShowWindow(g_hBtnPasteInPlace, showTab0);
    if (g_hBtnPasteCenter) ShowWindow(g_hBtnPasteCenter, showTab0);
    if (g_hBtnPasteAllArtboards) ShowWindow(g_hBtnPasteAllArtboards, showTab0);
    if (g_hBtnReflectVert) ShowWindow(g_hBtnReflectVert, showTab0);
    if (g_hBtnReflectHoriz) ShowWindow(g_hBtnReflectHoriz, showTab0);
    if (g_hBtnClear) ShowWindow(g_hBtnClear, showTab0);
    if (g_hBtnSettings) ShowWindow(g_hBtnSettings, showTab0);
    if (g_hStaticStatus) ShowWindow(g_hStaticStatus, showTab0);

    // Tab 1 controls
    if (g_hStaticOffsetInfo) ShowWindow(g_hStaticOffsetInfo, showTab1);
    if (g_hStaticOffsetModHeader) ShowWindow(g_hStaticOffsetModHeader, showTab1);
    for (int i = 1; i <= 9; ++i) {
        if (g_hBtnDir[i]) ShowWindow(g_hBtnDir[i], showTab1);
    }
    if (g_hBtnOffsetSettings) ShowWindow(g_hBtnOffsetSettings, showTab1);
    if (g_hStaticOffsetStatus) ShowWindow(g_hStaticOffsetStatus, showTab1);

    if (g_hTabBtnClipboard) InvalidateRect(g_hTabBtnClipboard, NULL, FALSE);
    if (g_hTabBtnOffset) InvalidateRect(g_hTabBtnOffset, NULL, FALSE);

    if (m_hPanelDlg) {
        RelayoutPanel(m_hPanelDlg);
        InvalidateRect(m_hPanelDlg, NULL, TRUE);
    }

    if (tabIndex == 1) {
        UpdateOffsetUI();
    } else {
        UpdateUI();
    }
}

static std::string GetModifierDisplayName(UINT mod, bool uppercase = false) {
    if (mod == (MOD_CONTROL | MOD_SHIFT)) return uppercase ? "CTRL + SHIFT" : "Ctrl+Shift";
    if (mod == (MOD_CONTROL | MOD_ALT))   return uppercase ? "CTRL + ALT" : "Ctrl+Alt";
    if (mod == (MOD_ALT | MOD_SHIFT))     return uppercase ? "ALT + SHIFT" : "Alt+Shift";
    if (mod == (MOD_CONTROL | MOD_ALT | MOD_SHIFT)) return uppercase ? "CTRL + ALT + SHIFT" : "Ctrl+Alt+Shift";
    if (mod == MOD_CONTROL) return uppercase ? "CTRL" : "Ctrl";
    if (mod == MOD_ALT)     return uppercase ? "ALT" : "Alt";
    if (mod == MOD_SHIFT)   return uppercase ? "SHIFT" : "Shift";
    return uppercase ? "KEY" : "Key";
}

void ClipboardPastePlugin::UpdateOffsetUI(int lastDir, bool isMove) {
    auto& cfg = ConfigManager::GetInstance();

    // 1. Update modifier header bar ([CTRL] Clone  |  [ALT] Move)
    if (g_hStaticOffsetModHeader) {
        char modBuf[128];
        if (!cfg.IsOffsetCloneHotkeysEnabled()) {
            std::snprintf(modBuf, sizeof(modBuf), "Offset Hotkeys: Disabled");
        } else {
            std::string cMod = GetModifierDisplayName(cfg.GetOffsetCloneModifier(), true);
            std::string mMod = GetModifierDisplayName(cfg.GetOffsetMoveModifier(), true);
            std::snprintf(modBuf, sizeof(modBuf), "[%s] Clone  |  [%s] Move", cMod.c_str(), mMod.c_str());
        }
        SetWindowTextA(g_hStaticOffsetModHeader, modBuf);
        InvalidateRect(g_hStaticOffsetModHeader, NULL, FALSE);
    }

    // 2. Update texts of 9 direction buttons (Prefix + Key name)
    for (int i = 1; i <= 9; ++i) {
        if (g_hBtnDir[i]) {
            std::string keyName = cfg.GetOffsetKeyName(i);
            std::wstring wLabel = std::wstring(g_dirPrefixes[i]) + std::wstring(keyName.begin(), keyName.end());
            SetWindowTextW(g_hBtnDir[i], wLabel.c_str());
            InvalidateRect(g_hBtnDir[i], NULL, FALSE);
        }
    }

    // 3. Selection dimensions
    if (g_hStaticOffsetInfo) {
        double w = 0.0, h = 0.0;
        bool hasSel = BufferManager::GetInstance().GetSelectionDimensions(w, h);
        char infoBuf[256];
        if (hasSel) {
            std::snprintf(infoBuf, sizeof(infoBuf), "Selection: %.1f x %.1f pt", w, h);
        } else {
            std::snprintf(infoBuf, sizeof(infoBuf), "No object selected");
        }
        SetWindowTextA(g_hStaticOffsetInfo, infoBuf);
        InvalidateRect(g_hStaticOffsetInfo, NULL, FALSE);
    }

    // 4. Status and hints
    if (g_hStaticOffsetStatus) {
        char statusBuf[256];
        if (!cfg.IsOffsetCloneHotkeysEnabled()) {
            std::snprintf(statusBuf, sizeof(statusBuf), "Offset Hotkeys: Disabled\n(Enable in Settings)");
        } else {
            std::string cMod = GetModifierDisplayName(cfg.GetOffsetCloneModifier(), false);
            std::string mMod = GetModifierDisplayName(cfg.GetOffsetMoveModifier(), false);
            double gap = cfg.GetOffsetCloneGap();
            if (lastDir >= 1 && lastDir <= 9) {
                std::string kName = cfg.GetOffsetKeyName(lastDir);
                if (isMove) {
                    std::snprintf(statusBuf, sizeof(statusBuf), "Moved Dir %d | Gap: %.1f pt\nKey: [%s + %s]",
                                  lastDir, gap, mMod.c_str(), kName.c_str());
                } else {
                    std::snprintf(statusBuf, sizeof(statusBuf), "Cloned Dir %d | Gap: %.1f pt\nKey: [%s + %s]",
                                  lastDir, gap, cMod.c_str(), kName.c_str());
                }
            } else {
                int preset = cfg.GetOffsetKeyPreset();
                std::string pName = "Numpad";
                if (preset == OFFSET_PRESET_TOPROW) pName = "Digits 1-9";
                else if (preset == OFFSET_PRESET_QWE_ASD_ZXC) pName = "QWE/ASD/ZXC";
                else if (preset == OFFSET_PRESET_UIO_JKL_M) pName = "UIO/JKL/M";
                else if (preset == OFFSET_PRESET_CUSTOM) pName = "Custom Keys";

                std::snprintf(statusBuf, sizeof(statusBuf), "Clone: [%s+Key] | Move: [%s+Key]\nLayout: %s",
                              cMod.c_str(), mMod.c_str(), pName.c_str());
            }
        }
        SetWindowTextA(g_hStaticOffsetStatus, statusBuf);
        InvalidateRect(g_hStaticOffsetStatus, NULL, FALSE);
    }
}

void ClipboardPastePlugin::UpdateUI() {
    if (!g_hStaticStatus) return;

    char text[256];
    if (ConfigManager::GetInstance().IsPluginDisabled()) {
        if (BufferManager::GetInstance().HasData()) {
            std::snprintf(text, sizeof(text), "Status: Disabled (Hotkeys off)\n(%zu item(s) in buffer)",
                          BufferManager::GetInstance().GetItemCount());
        } else {
            std::snprintf(text, sizeof(text), "Status: Disabled (Hotkeys off)\n(Buffer is empty)");
        }
    } else {
        if (BufferManager::GetInstance().HasData()) {
            std::snprintf(text, sizeof(text), "Buffer Status: Ready\n(%zu item(s) stored)",
                          BufferManager::GetInstance().GetItemCount());
        } else {
            std::snprintf(text, sizeof(text), "Buffer Status: Empty\n(No selection copied)");
        }
    }
    SetWindowTextA(g_hStaticStatus, text);
    if (g_hStaticStatus) {
        InvalidateRect(g_hStaticStatus, NULL, FALSE);
    }
}

void ClipboardPastePlugin::UpdateButtonsText() {
    auto& cfg = ConfigManager::GetInstance();
    bool disabled = cfg.IsPluginDisabled();

    std::string copyHk = cfg.GetHotkeyDisplayString(cfg.GetCopyHotkey());
    std::string placeHk = cfg.GetHotkeyDisplayString(cfg.GetPastePlaceHotkey());
    std::string centerHk = cfg.GetHotkeyDisplayString(cfg.GetPasteCenterHotkey());
    std::string allHk = cfg.GetHotkeyDisplayString(cfg.GetPasteAllArtboardsHotkey());
    std::string refVertHk = cfg.GetHotkeyDisplayString(cfg.GetReflectVertHotkey());
    std::string refHorizHk = cfg.GetHotkeyDisplayString(cfg.GetReflectHorizHotkey());

    if (g_hBtnCopy) {
        std::string text = disabled ? "Copy to Buffer (Off)" : ("Copy to Buffer (" + copyHk + ")");
        SetWindowTextA(g_hBtnCopy, text.c_str());
        InvalidateRect(g_hBtnCopy, NULL, FALSE);
    }
    if (g_hBtnPasteInPlace) {
        std::string text = disabled ? "Paste In Place (Off)" : ("Paste In Place (" + placeHk + ")");
        SetWindowTextA(g_hBtnPasteInPlace, text.c_str());
        InvalidateRect(g_hBtnPasteInPlace, NULL, FALSE);
    }
    if (g_hBtnPasteCenter) {
        std::string text = disabled ? "Paste to Center (Off)" : ("Paste to Center (" + centerHk + ")");
        SetWindowTextA(g_hBtnPasteCenter, text.c_str());
        InvalidateRect(g_hBtnPasteCenter, NULL, FALSE);
    }
    if (g_hBtnPasteAllArtboards) {
        std::string text = disabled ? "Paste to All Artboards (Off)" : ("Paste to All Artboards (" + allHk + ")");
        SetWindowTextA(g_hBtnPasteAllArtboards, text.c_str());
        InvalidateRect(g_hBtnPasteAllArtboards, NULL, FALSE);
    }
    if (g_hBtnReflectVert) {
        std::string text = disabled ? "Reflect V (Off)" : ("Reflect V (" + refVertHk + ")");
        SetWindowTextA(g_hBtnReflectVert, text.c_str());
        InvalidateRect(g_hBtnReflectVert, NULL, FALSE);
    }
    if (g_hBtnReflectHoriz) {
        std::string text = disabled ? "Reflect H (Off)" : ("Reflect H (" + refHorizHk + ")");
        SetWindowTextA(g_hBtnReflectHoriz, text.c_str());
        InvalidateRect(g_hBtnReflectHoriz, NULL, FALSE);
    }
}

SPErr ClipboardPastePlugin::GoMenuItem(void* message) {
    if (!message) return 1;

    AIMenuMessage* menuMsg = static_cast<AIMenuMessage*>(message);
    AIMenuItemHandle item = menuMsg->menuItem;

    if (item == m_hMenuWindow) {
        if (m_panelRef && sAIPanel) {
            AIBoolean isShown = false;
            sAIPanel->IsShown(m_panelRef, isShown);
            sAIPanel->Show(m_panelRef, !isShown);
        }
    } else if (item == m_hMenuCopy) {
        BufferManager::GetInstance().CopySelection();
        UpdateUI();
    } else if (item == m_hMenuPasteInPlace) {
        BufferManager::GetInstance().PasteInPlace();
        UpdateUI();
    } else if (item == m_hMenuPasteCenter) {
        BufferManager::GetInstance().PasteCenter();
        UpdateUI();
    } else if (item == m_hMenuPasteAllArtboards) {
        BufferManager::GetInstance().PasteAllArtboards();
        UpdateUI();
    } else if (item == m_hMenuReflectVert) {
        BufferManager::GetInstance().ReflectSelectionVertical();
    } else if (item == m_hMenuReflectHoriz) {
        BufferManager::GetInstance().ReflectSelectionHorizontal();
    } else if (item == m_hMenuClear) {
        BufferManager::GetInstance().Clear();
        UpdateUI();
    } else if (item == m_hMenuAbout) {
        MessageBoxA(NULL,
            "Clipboard Paste Native Plugin v1.25 (x64)\n\n"
            "High-performance native C++ dockable palette for Adobe Illustrator.\n"
            "Completely bypasses the broken Windows OLE clipboard.\n\n"
            "Tabs:\n"
            " 1. Clipboard: Instant in-memory copy & paste (In-Place, Center, All Artboards, Reflect V/H)\n"
            " 2. Offset Clone & Move: Clone or move selection by exact dimensions via Keypad or Hotkeys\n\n"
            "Shortcuts:\n"
            " - F5: In-Place Copy to Buffer\n"
            " - F6: In-Place Paste\n"
            " - F7: Paste to Center of View\n"
            " - F8: Paste on All Artboards\n"
            " - F9: Reflect Vertical (axis x=cx)\n"
            " - F10: Reflect Horizontal (axis y=cy)\n"
            " - Ctrl + [Key 1..9]: Pattern Offset Clone\n"
            " - Alt + [Key 1..9]: Pattern Offset Move (shift selection without duplicating)\n\n"
            "Author: SaidAuita (ph-cu-s.com/tools)",
            "About Clipboard Paste", MB_OK | MB_ICONINFORMATION);
    }

    return kNoErr;
}

SPErr ClipboardPastePlugin::UpdateMenuItem(void* message) {
    if (!message || !sAIMenu) return kNoErr;

    AIMenuMessage* menuMsg = static_cast<AIMenuMessage*>(message);
    if (menuMsg->menuItem == m_hMenuWindow && m_panelRef && sAIPanel) {
        AIBoolean isShown = false;
        sAIPanel->IsShown(m_panelRef, isShown);
        sAIMenu->CheckItem(m_hMenuWindow, isShown);
    }
    return kNoErr;
}
