#include "ConfigManager.h"
#include <shlobj.h>
#include <vector>
#include <sstream>

#pragma comment(lib, "shell32.lib")

struct KeyEntry {
    UINT vk;
    const char* name;
};

static const KeyEntry g_supportedKeys[] = {
    { VK_F1,  "F1" },  { VK_F2,  "F2" },  { VK_F3,  "F3" },  { VK_F4,  "F4" },
    { VK_F5,  "F5" },  { VK_F6,  "F6" },  { VK_F7,  "F7" },  { VK_F8,  "F8" },
    { VK_F9,  "F9" },  { VK_F10, "F10" }, { VK_F11, "F11" }, { VK_F12, "F12" },
    { 'A', "A" }, { 'B', "B" }, { 'C', "C" }, { 'D', "D" }, { 'E', "E" },
    { 'F', "F" }, { 'G', "G" }, { 'H', "H" }, { 'I', "I" }, { 'J', "J" },
    { 'K', "K" }, { 'L', "L" }, { 'M', "M" }, { 'N', "N" }, { 'O', "O" },
    { 'P', "P" }, { 'Q', "Q" }, { 'R', "R" }, { 'S', "S" }, { 'T', "T" },
    { 'U', "U" }, { 'V', "V" }, { 'W', "W" }, { 'X', "X" }, { 'Y', "Y" }, { 'Z', "Z" },
    { '0', "0" }, { '1', "1" }, { '2', "2" }, { '3', "3" }, { '4', "4" },
    { '5', "5" }, { '6', "6" }, { '7', "7" }, { '8', "8" }, { '9', "9" },
    { VK_INSERT, "Insert" }, { VK_DELETE, "Delete" },
    { VK_HOME, "Home" },     { VK_END, "End" },
    { VK_PRIOR, "Page Up" }, { VK_NEXT, "Page Down" }
};

struct ModEntry {
    UINT mod;
    const char* name;
};

static const ModEntry g_supportedMods[] = {
    { 0, "None" },
    { MOD_CONTROL, "Ctrl" },
    { MOD_ALT, "Alt" },
    { MOD_SHIFT, "Shift" },
    { MOD_CONTROL | MOD_SHIFT, "Ctrl + Shift" },
    { MOD_CONTROL | MOD_ALT, "Ctrl + Alt" },
    { MOD_ALT | MOD_SHIFT, "Alt + Shift" },
    { MOD_CONTROL | MOD_ALT | MOD_SHIFT, "Ctrl + Alt + Shift" }
};

ConfigManager& ConfigManager::GetInstance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() {
    // Defaults: F5 = Copy, F6 = Paste in Place, F7 = Paste Center of View
    m_copyHotkey = { VK_F5, 0 };
    m_pastePlaceHotkey = { VK_F6, 0 };
    m_pasteCenterHotkey = { VK_F7, 0 };

    Load();
}

std::string ConfigManager::GetIniPath() const {
    char appData[MAX_PATH] = { 0 };
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
        std::string folder = std::string(appData) + "\\ClipboardPastePlugin";
        CreateDirectoryA(folder.c_str(), NULL);
        return folder + "\\settings.ini";
    }
    return "settings.ini";
}

void ConfigManager::Load() {
    std::string ini = GetIniPath();

    m_copyHotkey.vkCode = (UINT)GetPrivateProfileIntA("Hotkeys", "Copy_VK", VK_F5, ini.c_str());
    m_copyHotkey.modifiers = (UINT)GetPrivateProfileIntA("Hotkeys", "Copy_Mod", 0, ini.c_str());

    m_pastePlaceHotkey.vkCode = (UINT)GetPrivateProfileIntA("Hotkeys", "PastePlace_VK", VK_F6, ini.c_str());
    m_pastePlaceHotkey.modifiers = (UINT)GetPrivateProfileIntA("Hotkeys", "PastePlace_Mod", 0, ini.c_str());

    m_pasteCenterHotkey.vkCode = (UINT)GetPrivateProfileIntA("Hotkeys", "PasteCenter_VK", VK_F7, ini.c_str());
    m_pasteCenterHotkey.modifiers = (UINT)GetPrivateProfileIntA("Hotkeys", "PasteCenter_Mod", 0, ini.c_str());

    m_selectAfterPaste = (GetPrivateProfileIntA("General", "SelectAfterPaste", 1, ini.c_str()) != 0);
}

void ConfigManager::Save() {
    std::string ini = GetIniPath();

    WritePrivateProfileStringA("Hotkeys", "Copy_VK", std::to_string(m_copyHotkey.vkCode).c_str(), ini.c_str());
    WritePrivateProfileStringA("Hotkeys", "Copy_Mod", std::to_string(m_copyHotkey.modifiers).c_str(), ini.c_str());

    WritePrivateProfileStringA("Hotkeys", "PastePlace_VK", std::to_string(m_pastePlaceHotkey.vkCode).c_str(), ini.c_str());
    WritePrivateProfileStringA("Hotkeys", "PastePlace_Mod", std::to_string(m_pastePlaceHotkey.modifiers).c_str(), ini.c_str());

    WritePrivateProfileStringA("Hotkeys", "PasteCenter_VK", std::to_string(m_pasteCenterHotkey.vkCode).c_str(), ini.c_str());
    WritePrivateProfileStringA("Hotkeys", "PasteCenter_Mod", std::to_string(m_pasteCenterHotkey.modifiers).c_str(), ini.c_str());

    WritePrivateProfileStringA("General", "SelectAfterPaste", m_selectAfterPaste ? "1" : "0", ini.c_str());
}

std::string ConfigManager::GetHotkeyDisplayString(const HotkeyConfig& hk) {
    std::string s;
    if (hk.modifiers & MOD_CONTROL) s += "Ctrl+";
    if (hk.modifiers & MOD_ALT)     s += "Alt+";
    if (hk.modifiers & MOD_SHIFT)   s += "Shift+";

    for (const auto& k : g_supportedKeys) {
        if (k.vk == hk.vkCode) {
            s += k.name;
            return s;
        }
    }
    if (hk.vkCode >= 'A' && hk.vkCode <= 'Z') {
        s += (char)hk.vkCode;
    } else {
        s += "Key_" + std::to_string(hk.vkCode);
    }
    return s;
}

bool ConfigManager::MatchHotkey(const HotkeyConfig& hk, UINT vkCode, UINT currentModifiers) {
    return (hk.vkCode == vkCode && hk.modifiers == currentModifiers);
}

// Dialog controls IDs
#define IDC_MOD_COPY        201
#define IDC_KEY_COPY        202
#define IDC_MOD_PLACE       203
#define IDC_KEY_PLACE       204
#define IDC_MOD_CENTER      205
#define IDC_KEY_CENTER      206
#define IDC_BTN_OK          207
#define IDC_BTN_CANCEL      208
#define IDC_BTN_DEFAULTS    209
#define IDC_CHK_SELECT      210

struct DialogData {
    HotkeyConfig copyHk;
    HotkeyConfig placeHk;
    HotkeyConfig centerHk;
    bool selectAfterPaste;
    bool saved;
};

static DialogData g_dlgData;
static HFONT g_hDlgFont = NULL;
static bool g_dialogRunning = false;

static void PopulateCombos(HWND hDlg) {
    HWND hModCopy   = GetDlgItem(hDlg, IDC_MOD_COPY);
    HWND hKeyCopy   = GetDlgItem(hDlg, IDC_KEY_COPY);
    HWND hModPlace  = GetDlgItem(hDlg, IDC_MOD_PLACE);
    HWND hKeyPlace  = GetDlgItem(hDlg, IDC_KEY_PLACE);
    HWND hModCenter = GetDlgItem(hDlg, IDC_MOD_CENTER);
    HWND hKeyCenter = GetDlgItem(hDlg, IDC_KEY_CENTER);

    HWND modCombos[] = { hModCopy, hModPlace, hModCenter };
    HWND keyCombos[] = { hKeyCopy, hKeyPlace, hKeyCenter };

    for (HWND mc : modCombos) {
        SendMessage(mc, CB_RESETCONTENT, 0, 0);
        for (size_t i = 0; i < sizeof(g_supportedMods) / sizeof(g_supportedMods[0]); ++i) {
            SendMessageA(mc, CB_ADDSTRING, 0, (LPARAM)g_supportedMods[i].name);
        }
    }

    for (HWND kc : keyCombos) {
        SendMessage(kc, CB_RESETCONTENT, 0, 0);
        for (size_t i = 0; i < sizeof(g_supportedKeys) / sizeof(g_supportedKeys[0]); ++i) {
            SendMessageA(kc, CB_ADDSTRING, 0, (LPARAM)g_supportedKeys[i].name);
        }
    }

    auto selectMod = [](HWND h, UINT mod) {
        for (size_t i = 0; i < sizeof(g_supportedMods) / sizeof(g_supportedMods[0]); ++i) {
            if (g_supportedMods[i].mod == mod) {
                SendMessage(h, CB_SETCURSEL, (WPARAM)i, 0);
                return;
            }
        }
        SendMessage(h, CB_SETCURSEL, 0, 0);
    };

    auto selectKey = [](HWND h, UINT vk) {
        for (size_t i = 0; i < sizeof(g_supportedKeys) / sizeof(g_supportedKeys[0]); ++i) {
            if (g_supportedKeys[i].vk == vk) {
                SendMessage(h, CB_SETCURSEL, (WPARAM)i, 0);
                return;
            }
        }
        SendMessage(h, CB_SETCURSEL, 0, 0);
    };

    selectMod(hModCopy, g_dlgData.copyHk.modifiers);
    selectKey(hKeyCopy, g_dlgData.copyHk.vkCode);

    selectMod(hModPlace, g_dlgData.placeHk.modifiers);
    selectKey(hKeyPlace, g_dlgData.placeHk.vkCode);

    selectMod(hModCenter, g_dlgData.centerHk.modifiers);
    selectKey(hKeyCenter, g_dlgData.centerHk.vkCode);

    CheckDlgButton(hDlg, IDC_CHK_SELECT, g_dlgData.selectAfterPaste ? BST_CHECKED : BST_UNCHECKED);
}

static LRESULT CALLBACK SettingsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            if (!g_hDlgFont) {
                g_hDlgFont = CreateFontA(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
            }
            break;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == IDC_BTN_OK) {
                HWND hModCopy   = GetDlgItem(hwnd, IDC_MOD_COPY);
                HWND hKeyCopy   = GetDlgItem(hwnd, IDC_KEY_COPY);
                HWND hModPlace  = GetDlgItem(hwnd, IDC_MOD_PLACE);
                HWND hKeyPlace  = GetDlgItem(hwnd, IDC_KEY_PLACE);
                HWND hModCenter = GetDlgItem(hwnd, IDC_MOD_CENTER);
                HWND hKeyCenter = GetDlgItem(hwnd, IDC_KEY_CENTER);

                int idxModCopy = (int)SendMessage(hModCopy, CB_GETCURSEL, 0, 0);
                int idxKeyCopy = (int)SendMessage(hKeyCopy, CB_GETCURSEL, 0, 0);
                if (idxModCopy >= 0 && idxModCopy < (int)(sizeof(g_supportedMods) / sizeof(g_supportedMods[0]))) {
                    g_dlgData.copyHk.modifiers = g_supportedMods[idxModCopy].mod;
                }
                if (idxKeyCopy >= 0 && idxKeyCopy < (int)(sizeof(g_supportedKeys) / sizeof(g_supportedKeys[0]))) {
                    g_dlgData.copyHk.vkCode = g_supportedKeys[idxKeyCopy].vk;
                }

                int idxModPlace = (int)SendMessage(hModPlace, CB_GETCURSEL, 0, 0);
                int idxKeyPlace = (int)SendMessage(hKeyPlace, CB_GETCURSEL, 0, 0);
                if (idxModPlace >= 0 && idxModPlace < (int)(sizeof(g_supportedMods) / sizeof(g_supportedMods[0]))) {
                    g_dlgData.placeHk.modifiers = g_supportedMods[idxModPlace].mod;
                }
                if (idxKeyPlace >= 0 && idxKeyPlace < (int)(sizeof(g_supportedKeys) / sizeof(g_supportedKeys[0]))) {
                    g_dlgData.placeHk.vkCode = g_supportedKeys[idxKeyPlace].vk;
                }

                int idxModCenter = (int)SendMessage(hModCenter, CB_GETCURSEL, 0, 0);
                int idxKeyCenter = (int)SendMessage(hKeyCenter, CB_GETCURSEL, 0, 0);
                if (idxModCenter >= 0 && idxModCenter < (int)(sizeof(g_supportedMods) / sizeof(g_supportedMods[0]))) {
                    g_dlgData.centerHk.modifiers = g_supportedMods[idxModCenter].mod;
                }
                if (idxKeyCenter >= 0 && idxKeyCenter < (int)(sizeof(g_supportedKeys) / sizeof(g_supportedKeys[0]))) {
                    g_dlgData.centerHk.vkCode = g_supportedKeys[idxKeyCenter].vk;
                }

                g_dlgData.selectAfterPaste = (IsDlgButtonChecked(hwnd, IDC_CHK_SELECT) == BST_CHECKED);

                g_dlgData.saved = true;
                g_dialogRunning = false;
                PostMessageA(hwnd, WM_NULL, 0, 0);
                return 0;
            } else if (id == IDC_BTN_CANCEL) {
                g_dlgData.saved = false;
                g_dialogRunning = false;
                PostMessageA(hwnd, WM_NULL, 0, 0);
                return 0;
            } else if (id == IDC_BTN_DEFAULTS) {
                g_dlgData.copyHk = { VK_F5, 0 };
                g_dlgData.placeHk = { VK_F6, 0 };
                g_dlgData.centerHk = { VK_F7, 0 };
                g_dlgData.selectAfterPaste = true;
                PopulateCombos(hwnd);
                return 0;
            }
            break;
        }

        case WM_CLOSE:
            g_dlgData.saved = false;
            g_dialogRunning = false;
            PostMessageA(hwnd, WM_NULL, 0, 0);
            return 0;

        case WM_DESTROY:
            g_dialogRunning = false;
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

bool ConfigManager::ShowSettingsDialog(HWND parentHwnd) {
    HINSTANCE hInst = GetModuleHandleA(NULL);

    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(wc);
    if (!GetClassInfoExA(hInst, "ClipboardPasteSettingsDlg", &wc)) {
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = SettingsDlgProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = "ClipboardPasteSettingsDlg";
        RegisterClassExA(&wc);
    }

    g_dlgData.copyHk = m_copyHotkey;
    g_dlgData.placeHk = m_pastePlaceHotkey;
    g_dlgData.centerHk = m_pasteCenterHotkey;
    g_dlgData.selectAfterPaste = m_selectAfterPaste;
    g_dlgData.saved = false;

    // Resolve top-level parent window for true modality
    HWND hOwner = parentHwnd;
    if (hOwner && IsWindow(hOwner)) {
        HWND hRoot = GetAncestor(hOwner, GA_ROOT);
        if (hRoot && IsWindow(hRoot)) {
            hOwner = hRoot;
        }
    }

    // Center dialog relative to owner or desktop
    RECT rcParent;
    if (hOwner && IsWindow(hOwner)) {
        GetWindowRect(hOwner, &rcParent);
    } else {
        GetWindowRect(GetDesktopWindow(), &rcParent);
    }
    int dlgW = 440;
    int dlgH = 265;
    int x = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int y = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;

    HWND hDlg = CreateWindowExA(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        "ClipboardPasteSettingsDlg",
        "Clipboard Paste - Preferences",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, dlgW, dlgH,
        hOwner, NULL, hInst, NULL
    );

    if (!hDlg) return false;

    HFONT hFont = g_hDlgFont ? g_hDlgFont : (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    auto addLabel = [&](const char* text, int lx, int ly, int lw, int lh) {
        HWND lbl = CreateWindowExA(0, "STATIC", text, WS_CHILD | WS_VISIBLE | SS_LEFT,
                                   lx, ly, lw, lh, hDlg, NULL, hInst, NULL);
        SendMessage(lbl, WM_SETFONT, (WPARAM)hFont, TRUE);
        return lbl;
    };

    addLabel("Configure Hotkeys:", 18, 14, 380, 20);

    // Row 1: Copy
    addLabel("1. Copy to Buffer:", 18, 44, 160, 22);
    HWND hModCopy = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                                    184, 42, 116, 180, hDlg, (HMENU)IDC_MOD_COPY, hInst, NULL);
    HWND hKeyCopy = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                                    308, 42, 96, 180, hDlg, (HMENU)IDC_KEY_COPY, hInst, NULL);

    // Row 2: Paste Place
    addLabel("2. Paste in Place:", 18, 78, 160, 22);
    HWND hModPlace = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                                     184, 76, 116, 180, hDlg, (HMENU)IDC_MOD_PLACE, hInst, NULL);
    HWND hKeyPlace = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                                     308, 76, 96, 180, hDlg, (HMENU)IDC_KEY_PLACE, hInst, NULL);

    // Row 3: Paste Center
    addLabel("3. Paste Center of View:", 18, 112, 160, 22);
    HWND hModCenter = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                                      184, 110, 116, 180, hDlg, (HMENU)IDC_MOD_CENTER, hInst, NULL);
    HWND hKeyCenter = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                                      308, 110, 96, 180, hDlg, (HMENU)IDC_KEY_CENTER, hInst, NULL);

    // Checkbox: Select object(s) after paste
    HWND hChkSelect = CreateWindowExA(0, "BUTTON", "Select object(s) after paste",
                                      WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                                      18, 144, 380, 22, hDlg, (HMENU)IDC_CHK_SELECT, hInst, NULL);

    // Bottom buttons (strictly in English)
    HWND hBtnDefaults = CreateWindowExA(0, "BUTTON", "Defaults", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                                        18, 180, 90, 28, hDlg, (HMENU)IDC_BTN_DEFAULTS, hInst, NULL);
    HWND hBtnOk = CreateWindowExA(0, "BUTTON", "Save", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
                                  228, 180, 88, 28, hDlg, (HMENU)IDC_BTN_OK, hInst, NULL);
    HWND hBtnCancel = CreateWindowExA(0, "BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                                      324, 180, 80, 28, hDlg, (HMENU)IDC_BTN_CANCEL, hInst, NULL);

    HWND allControls[] = { hModCopy, hKeyCopy, hModPlace, hKeyPlace, hModCenter, hKeyCenter,
                           hChkSelect, hBtnDefaults, hBtnOk, hBtnCancel };
    for (HWND c : allControls) {
        SendMessage(c, WM_SETFONT, (WPARAM)hFont, TRUE);
    }

    PopulateCombos(hDlg);

    if (hOwner && IsWindow(hOwner)) {
        EnableWindow(hOwner, FALSE);
    }

    ShowWindow(hDlg, SW_SHOW);
    SetForegroundWindow(hDlg);
    SetActiveWindow(hDlg);
    SetFocus(hBtnOk);

    m_dialogActive = true;
    g_dialogRunning = true;

    // Safe modal message loop using GetMessage
    MSG msg;
    while (g_dialogRunning && IsWindow(hDlg)) {
        BOOL bRet = GetMessageA(&msg, NULL, 0, 0);
        if (bRet <= 0) {
            if (bRet == 0) {
                // Forward real application quit
                PostQuitMessage((int)msg.wParam);
            }
            break;
        }

        if (IsDialogMessageA(hDlg, &msg)) {
            continue;
        }

        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    m_dialogActive = false;

    // First restore owner window state before destroying dialog
    if (hOwner && IsWindow(hOwner)) {
        EnableWindow(hOwner, TRUE);
        SetForegroundWindow(hOwner);
        SetActiveWindow(hOwner);
    }

    if (IsWindow(hDlg)) {
        DestroyWindow(hDlg);
    }

    if (g_dlgData.saved) {
        m_copyHotkey = g_dlgData.copyHk;
        m_pastePlaceHotkey = g_dlgData.placeHk;
        m_pasteCenterHotkey = g_dlgData.centerHk;
        m_selectAfterPaste = g_dlgData.selectAfterPaste;
        Save();
        return true;
    }
    return false;
}
