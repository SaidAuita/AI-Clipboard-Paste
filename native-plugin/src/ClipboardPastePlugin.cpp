#include "ClipboardPastePlugin.h"
#include "ConfigManager.h"
#include <cstdio>
#include <windows.h>
#include <shellapi.h>

#define PLUGIN_VERSION_STR     "v1.14"

#define IDC_STATIC_TITLE       100
#define IDC_STATIC_VERSION     101
#define IDC_BTN_COPY           102
#define IDC_BTN_PASTE_IN_PLACE 103
#define IDC_BTN_PASTE_CENTER   104
#define IDC_BTN_CLEAR          105
#define IDC_STATIC_STATUS      106
#define IDC_BTN_SETTINGS       107
#define IDC_STATIC_LINK_AUTHOR 108
#define IDC_STATIC_LINK_TOOLS  109

static HWND g_hStaticHeaderTitle = NULL;
static HWND g_hStaticHeaderVersion = NULL;
static HWND g_hBtnCopy = NULL;
static HWND g_hBtnPasteInPlace = NULL;
static HWND g_hBtnPasteCenter = NULL;
static HWND g_hBtnClear = NULL;
static HWND g_hBtnSettings = NULL;
static HWND g_hStaticStatus = NULL;
static HWND g_hStaticLinkAuthor = NULL;
static HWND g_hStaticLinkTools = NULL;

static HFONT g_hUIFont = NULL;
static HFONT g_hHeaderFont = NULL;
static HFONT g_hFooterFont = NULL;
static HBRUSH g_hPanelBgBrush = NULL;
static WNDPROC g_oldPanelProc = NULL;
static WNDPROC g_oldButtonProc = NULL;
static WNDPROC g_oldStatusProc = NULL;
static HHOOK g_hKeyHook = NULL;

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
        c.textSecondary = RGB(150, 150, 150);   // #969696
        c.btnBg         = RGB(68, 68, 68);      // #444444 (like Edit Artboards)
        c.btnBgHover    = RGB(84, 84, 84);      // #545454
        c.btnBgPressed  = RGB(50, 50, 50);      // #323232
        c.btnBorder     = RGB(92, 92, 92);      // #5c5c5c
        c.btnText       = RGB(235, 235, 235);   // #ebebeb
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
        c.statusBg      = RGB(230, 230, 230);   // #e6e6e6
        c.statusBorder  = RGB(195, 195, 195);   // #c3c3c3
        c.statusText    = RGB(35, 35, 35);      // #232323
        c.linkColor     = RGB(15, 105, 210);    // #0f69d2
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

            COLORREF fillCol = !isEnabled ? theme.panelBg : (isPushed ? theme.btnBgPressed : (isHover ? theme.btnBgHover : theme.btnBg));
            COLORREF borderCol = !isEnabled ? theme.statusBorder : theme.btnBorder;
            COLORREF textCol = !isEnabled ? theme.textSecondary : theme.btnText;

            HBRUSH hPanelBg = CreateSolidBrush(theme.panelBg);
            FillRect(memDC, &rc, hPanelBg);
            DeleteObject(hPanelBg);

            HPEN hPen = CreatePen(PS_SOLID, 1, borderCol);
            HBRUSH hBrush = CreateSolidBrush(fillCol);
            HGDIOBJ oldPen = SelectObject(memDC, hPen);
            HGDIOBJ oldBrush = SelectObject(memDC, hBrush);

            RoundRect(memDC, rc.left, rc.top, rc.right, rc.bottom, 6, 6);

            SelectObject(memDC, oldPen);
            SelectObject(memDC, oldBrush);
            DeleteObject(hPen);
            DeleteObject(hBrush);

            char text[128] = {0};
            GetWindowTextA(hwnd, text, sizeof(text));

            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, textCol);
            SelectObject(memDC, g_hUIFont);

            RECT textRc = rc;
            if (isPushed) {
                OffsetRect(&textRc, 0, 1);
            }
            DrawTextA(memDC, text, -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

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

        RoundRect(memDC, rc.left, rc.top, rc.right, rc.bottom, 6, 6);

        SelectObject(memDC, oldPen);
        SelectObject(memDC, oldBrush);
        DeleteObject(hPen);
        DeleteObject(hBrush);

        char text[256] = {0};
        GetWindowTextA(hwnd, text, sizeof(text));

        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, theme.statusText);
        SelectObject(memDC, g_hUIFont);

        RECT textRc = rc;
        textRc.left += 8;
        textRc.top += 3;
        textRc.right -= 8;
        textRc.bottom -= 3;
        DrawTextA(memDC, text, -1, &textRc, DT_LEFT | DT_WORDBREAK);

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
    int height = rc.bottom - rc.top;

    int pad = 8;
    int btnW = (std::max)(100, width - (pad * 2));
    int btnH = 26;

    int y = 6;
    // Header row: Title (left) & Version (right)
    if (g_hStaticHeaderTitle && g_hStaticHeaderVersion) {
        int verW = 45;
        int titleW = (std::max)(60, btnW - verW);
        SetWindowPos(g_hStaticHeaderTitle, NULL, pad, y, titleW, 18, SWP_NOZORDER);
        SetWindowPos(g_hStaticHeaderVersion, NULL, pad + btnW - verW, y, verW, 18, SWP_NOZORDER);
        y += 18 + 5;
    }

    if (g_hBtnCopy) {
        SetWindowPos(g_hBtnCopy, NULL, pad, y, btnW, btnH, SWP_NOZORDER);
        y += btnH + 4;
    }
    if (g_hBtnPasteInPlace) {
        SetWindowPos(g_hBtnPasteInPlace, NULL, pad, y, btnW, btnH, SWP_NOZORDER);
        y += btnH + 4;
    }
    if (g_hBtnPasteCenter) {
        SetWindowPos(g_hBtnPasteCenter, NULL, pad, y, btnW, btnH, SWP_NOZORDER);
        y += btnH + 5;
    }
    if (g_hBtnClear && g_hBtnSettings) {
        int halfW = (btnW - 6) / 2;
        SetWindowPos(g_hBtnClear, NULL, pad, y, halfW, 24, SWP_NOZORDER);
        SetWindowPos(g_hBtnSettings, NULL, pad + halfW + 6, y, halfW, 24, SWP_NOZORDER);
        y += 24 + 5;
    } else if (g_hBtnClear) {
        int clearW = (std::min)(110, btnW);
        SetWindowPos(g_hBtnClear, NULL, pad, y, clearW, 24, SWP_NOZORDER);
        y += 24 + 5;
    }
    if (g_hStaticStatus) {
        SetWindowPos(g_hStaticStatus, NULL, pad, y, btnW, 34, SWP_NOZORDER);
        y += 34 + 6;
    }
    // Footer links: single line, 18px high
    if (g_hStaticLinkAuthor) {
        SetWindowPos(g_hStaticLinkAuthor, NULL, pad, y, btnW, 18, SWP_NOZORDER);
        y += 18 + 2;
    }
    if (g_hStaticLinkTools) {
        SetWindowPos(g_hStaticLinkTools, NULL, pad, y, btnW, 18, SWP_NOZORDER);
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
                case IDC_BTN_CLEAR: {
                    AppContext appContext(pluginRef);
                    BufferManager::GetInstance().Clear();
                    ClipboardPastePlugin::GetInstance().UpdateUI();
                    return 0;
                }
                case IDC_BTN_SETTINGS: {
                    if (ConfigManager::GetInstance().ShowSettingsDialog(hwnd)) {
                        ClipboardPastePlugin::GetInstance().UpdateButtonsText();
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
            g_hBtnCopy = NULL;
            g_hBtnPasteInPlace = NULL;
            g_hBtnPasteCenter = NULL;
            g_hBtnClear = NULL;
            g_hBtnSettings = NULL;
            g_hStaticStatus = NULL;
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
        if (ConfigManager::GetInstance().IsDialogActive()) {
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
      m_hBrightnessNotifier(nullptr),
      m_hMenuWindow(nullptr),
      m_hMenuCopy(nullptr),
      m_hMenuPasteInPlace(nullptr),
      m_hMenuPasteCenter(nullptr),
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
    if (g_hFooterFont) {
        DeleteObject(g_hFooterFont);
        g_hFooterFont = NULL;
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
    AISize prefConstS = { 190, 235 };
    AISize prefUnconstS = { 210, 240 };
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

        if (!g_hUIFont) {
            g_hUIFont = CreateFontA(-12, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        }
        if (!g_hHeaderFont) {
            g_hHeaderFont = CreateFontA(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        }
        if (!g_hFooterFont) {
            g_hFooterFont = CreateFontA(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        }

        // Header controls: Title and Version
        g_hStaticHeaderTitle = CreateWindowExA(
            0, "STATIC", "Clipboard Paste",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            10, 6, 160, 18, m_hPanelDlg, (HMENU)IDC_STATIC_TITLE, hInst, NULL);

        g_hStaticHeaderVersion = CreateWindowExA(
            0, "STATIC", PLUGIN_VERSION_STR,
            WS_VISIBLE | WS_CHILD | SS_RIGHT,
            170, 6, 60, 18, m_hPanelDlg, (HMENU)IDC_STATIC_VERSION, hInst, NULL);

        g_hBtnCopy = CreateWindowExA(
            0, "BUTTON", "Copy to Buffer (F5)",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            10, 29, 220, 26, m_hPanelDlg, (HMENU)IDC_BTN_COPY, hInst, NULL);

        g_hBtnPasteInPlace = CreateWindowExA(
            0, "BUTTON", "Paste In Place (F6)",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            10, 59, 220, 26, m_hPanelDlg, (HMENU)IDC_BTN_PASTE_IN_PLACE, hInst, NULL);

        g_hBtnPasteCenter = CreateWindowExA(
            0, "BUTTON", "Paste to Center (F7)",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            10, 89, 220, 26, m_hPanelDlg, (HMENU)IDC_BTN_PASTE_CENTER, hInst, NULL);

        g_hBtnClear = CreateWindowExA(
            0, "BUTTON", "Clear Buffer",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            10, 120, 100, 24, m_hPanelDlg, (HMENU)IDC_BTN_CLEAR, hInst, NULL);

        g_hBtnSettings = CreateWindowExA(
            0, "BUTTON", "Settings...",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            120, 120, 100, 24, m_hPanelDlg, (HMENU)IDC_BTN_SETTINGS, hInst, NULL);

        g_hStaticStatus = CreateWindowExA(
            0, "STATIC", "Buffer Status: Empty\n(No selection copied)",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            10, 149, 220, 34, m_hPanelDlg, (HMENU)IDC_STATIC_STATUS, hInst, NULL);

        // Footer links: single line, no word wrap, end ellipsis, clickable
        g_hStaticLinkAuthor = CreateWindowExA(
            0, "STATIC", "github.com/SaidAuita/AI-Clipboard-Paste",
            WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP | SS_ENDELLIPSIS | SS_NOTIFY,
            10, 189, 220, 18, m_hPanelDlg, (HMENU)IDC_STATIC_LINK_AUTHOR, hInst, NULL);

        g_hStaticLinkTools = CreateWindowExA(
            0, "STATIC", "More scripts: ph-cu-s.com/tools",
            WS_VISIBLE | WS_CHILD | SS_LEFTNOWORDWRAP | SS_ENDELLIPSIS | SS_NOTIFY,
            10, 209, 220, 18, m_hPanelDlg, (HMENU)IDC_STATIC_LINK_TOOLS, hInst, NULL);

        SendMessage(g_hStaticHeaderTitle, WM_SETFONT, (WPARAM)g_hHeaderFont, TRUE);
        SendMessage(g_hStaticHeaderVersion, WM_SETFONT, (WPARAM)g_hFooterFont, TRUE);
        SendMessage(g_hBtnCopy, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        SendMessage(g_hBtnPasteInPlace, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        SendMessage(g_hBtnPasteCenter, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        SendMessage(g_hBtnClear, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        SendMessage(g_hBtnSettings, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        SendMessage(g_hStaticStatus, WM_SETFONT, (WPARAM)g_hUIFont, TRUE);
        SendMessage(g_hStaticLinkAuthor, WM_SETFONT, (WPARAM)g_hFooterFont, TRUE);
        SendMessage(g_hStaticLinkTools, WM_SETFONT, (WPARAM)g_hFooterFont, TRUE);

        // Subclass buttons and status for native Illustrator styling and theme support
        g_oldButtonProc = (WNDPROC)GetWindowLongPtr(g_hBtnCopy, GWLP_WNDPROC);
        SetWindowLongPtr(g_hBtnCopy, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
        SetWindowLongPtr(g_hBtnPasteInPlace, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
        SetWindowLongPtr(g_hBtnPasteCenter, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
        SetWindowLongPtr(g_hBtnClear, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
        SetWindowLongPtr(g_hBtnSettings, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);

        g_oldStatusProc = (WNDPROC)GetWindowLongPtr(g_hStaticStatus, GWLP_WNDPROC);
        SetWindowLongPtr(g_hStaticStatus, GWLP_WNDPROC, (LONG_PTR)CustomStatusProc);

        UpdateButtonsText();

        g_oldPanelProc = (WNDPROC)SetWindowLongPtr(m_hPanelDlg, GWLP_WNDPROC, (LONG_PTR)PanelWndProc);
        RelayoutPanel(m_hPanelDlg);
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

void ClipboardPastePlugin::UpdateUI() {
    if (!g_hStaticStatus) return;

    char text[256];
    if (BufferManager::GetInstance().HasData()) {
        std::snprintf(text, sizeof(text), "Buffer Status: Ready\n(%zu item(s) stored)",
                      BufferManager::GetInstance().GetItemCount());
    } else {
        std::snprintf(text, sizeof(text), "Buffer Status: Empty\n(No selection copied)");
    }
    SetWindowTextA(g_hStaticStatus, text);
    if (g_hStaticStatus) {
        InvalidateRect(g_hStaticStatus, NULL, FALSE);
    }
}

void ClipboardPastePlugin::UpdateButtonsText() {
    auto& cfg = ConfigManager::GetInstance();
    std::string copyHk = cfg.GetHotkeyDisplayString(cfg.GetCopyHotkey());
    std::string placeHk = cfg.GetHotkeyDisplayString(cfg.GetPastePlaceHotkey());
    std::string centerHk = cfg.GetHotkeyDisplayString(cfg.GetPasteCenterHotkey());

    if (g_hBtnCopy) {
        std::string text = "Copy to Buffer (" + copyHk + ")";
        SetWindowTextA(g_hBtnCopy, text.c_str());
        InvalidateRect(g_hBtnCopy, NULL, FALSE);
    }
    if (g_hBtnPasteInPlace) {
        std::string text = "Paste In Place (" + placeHk + ")";
        SetWindowTextA(g_hBtnPasteInPlace, text.c_str());
        InvalidateRect(g_hBtnPasteInPlace, NULL, FALSE);
    }
    if (g_hBtnPasteCenter) {
        std::string text = "Paste to Center (" + centerHk + ")";
        SetWindowTextA(g_hBtnPasteCenter, text.c_str());
        InvalidateRect(g_hBtnPasteCenter, NULL, FALSE);
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
    } else if (item == m_hMenuClear) {
        BufferManager::GetInstance().Clear();
        UpdateUI();
    } else if (item == m_hMenuAbout) {
        MessageBoxA(NULL,
            "Clipboard Paste Native Plugin v1.0.0 (x64)\n\n"
            "High-performance native C++ dockable palette for Adobe Illustrator.\n"
            "Completely bypasses the broken Windows OLE clipboard.\n\n"
            "Shortcuts:\n"
            " - F5: In-Place Copy to Buffer\n"
            " - F6: In-Place Paste\n"
            " - F7: Paste to Center of Artboard\n\n"
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
