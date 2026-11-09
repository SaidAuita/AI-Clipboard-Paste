#include "ConfigManager.h"
#include "Suites.h"
#include <shlobj.h>
#include <vector>
#include <sstream>
#include <algorithm>

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
    // Defaults: F5 = Copy, F6 = Paste in Place, F7 = Paste Center, F8 = Paste All Artboards, F9 = Reflect Vert, F10 = Reflect Horiz
    m_copyHotkey = { VK_F5, 0 };
    m_pastePlaceHotkey = { VK_F6, 0 };
    m_pasteCenterHotkey = { VK_F7, 0 };
    m_pasteAllArtboardsHotkey = { VK_F8, 0 };
    m_reflectVertHotkey = { VK_F9, 0 };
    m_reflectHorizHotkey = { VK_F10, 0 };
    SetOffsetKeyPreset(OFFSET_PRESET_NUMPAD);

    Load();
}

void ConfigManager::SetOffsetKeyPreset(int preset) {
    m_offsetKeyPreset = preset;
    switch (preset) {
        case OFFSET_PRESET_TOPROW:
            m_offsetKeys[1] = '1';
            m_offsetKeys[2] = '2';
            m_offsetKeys[3] = '3';
            m_offsetKeys[4] = '4';
            m_offsetKeys[5] = '5';
            m_offsetKeys[6] = '6';
            m_offsetKeys[7] = '7';
            m_offsetKeys[8] = '8';
            m_offsetKeys[9] = '9';
            break;
        case OFFSET_PRESET_QWE_ASD_ZXC:
            m_offsetKeys[7] = 'Q';
            m_offsetKeys[8] = 'W';
            m_offsetKeys[9] = 'E';
            m_offsetKeys[4] = 'A';
            m_offsetKeys[5] = 'S';
            m_offsetKeys[6] = 'D';
            m_offsetKeys[1] = 'Z';
            m_offsetKeys[2] = 'X';
            m_offsetKeys[3] = 'C';
            break;
        case OFFSET_PRESET_UIO_JKL_M:
            m_offsetKeys[7] = 'U';
            m_offsetKeys[8] = 'I';
            m_offsetKeys[9] = 'O';
            m_offsetKeys[4] = 'J';
            m_offsetKeys[5] = 'K';
            m_offsetKeys[6] = 'L';
            m_offsetKeys[1] = 'M';
            m_offsetKeys[2] = VK_OEM_COMMA;  // ','
            m_offsetKeys[3] = VK_OEM_PERIOD; // '.'
            break;
        case OFFSET_PRESET_NUMPAD:
        default:
            m_offsetKeys[1] = VK_NUMPAD1;
            m_offsetKeys[2] = VK_NUMPAD2;
            m_offsetKeys[3] = VK_NUMPAD3;
            m_offsetKeys[4] = VK_NUMPAD4;
            m_offsetKeys[5] = VK_NUMPAD5;
            m_offsetKeys[6] = VK_NUMPAD6;
            m_offsetKeys[7] = VK_NUMPAD7;
            m_offsetKeys[8] = VK_NUMPAD8;
            m_offsetKeys[9] = VK_NUMPAD9;
            break;
    }
}

UINT ConfigManager::GetOffsetKey(int dir) const {
    if (dir >= 1 && dir <= 9) return m_offsetKeys[dir];
    return 0;
}

void ConfigManager::SetOffsetKey(int dir, UINT vk) {
    if (dir >= 1 && dir <= 9) {
        m_offsetKeys[dir] = vk;
        m_offsetKeyPreset = OFFSET_PRESET_CUSTOM;
    }
}

std::string ConfigManager::GetOffsetKeyName(int dir) const {
    if (dir < 1 || dir > 9) return "";
    UINT vk = m_offsetKeys[dir];

    if (vk >= VK_NUMPAD1 && vk <= VK_NUMPAD9) {
        return std::to_string(vk - VK_NUMPAD1 + 1);
    }
    if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) {
        return std::string(1, (char)vk);
    }
    if (vk == VK_OEM_COMMA) return ",";
    if (vk == VK_OEM_PERIOD) return ".";
    for (const auto& k : g_supportedKeys) {
        if (k.vk == vk) return k.name;
    }
    return std::to_string(vk);
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

    m_pasteAllArtboardsHotkey.vkCode = (UINT)GetPrivateProfileIntA("Hotkeys", "PasteAllArtboards_VK", VK_F8, ini.c_str());
    m_pasteAllArtboardsHotkey.modifiers = (UINT)GetPrivateProfileIntA("Hotkeys", "PasteAllArtboards_Mod", 0, ini.c_str());

    m_reflectVertHotkey.vkCode = (UINT)GetPrivateProfileIntA("Hotkeys", "ReflectVert_VK", VK_F9, ini.c_str());
    m_reflectVertHotkey.modifiers = (UINT)GetPrivateProfileIntA("Hotkeys", "ReflectVert_Mod", 0, ini.c_str());

    m_reflectHorizHotkey.vkCode = (UINT)GetPrivateProfileIntA("Hotkeys", "ReflectHoriz_VK", VK_F10, ini.c_str());
    m_reflectHorizHotkey.modifiers = (UINT)GetPrivateProfileIntA("Hotkeys", "ReflectHoriz_Mod", 0, ini.c_str());

    m_selectAfterPaste = (GetPrivateProfileIntA("General", "SelectAfterPaste", 1, ini.c_str()) != 0);
    m_disabled = (GetPrivateProfileIntA("General", "Disabled", 0, ini.c_str()) != 0) ||
                 (GetPrivateProfileIntA("General", "DisablePlugin", 0, ini.c_str()) != 0);

    m_offsetCloneEnabled = (GetPrivateProfileIntA("PatternOffset", "Enabled", 1, ini.c_str()) != 0);
    m_offsetCloneModifier = (UINT)GetPrivateProfileIntA("PatternOffset", "CloneModifier",
                                 GetPrivateProfileIntA("PatternOffset", "Modifier", MOD_CONTROL, ini.c_str()), ini.c_str());
    m_offsetMoveModifier = (UINT)GetPrivateProfileIntA("PatternOffset", "MoveModifier", MOD_ALT, ini.c_str());
    m_offsetStepDivisorModifier = (UINT)GetPrivateProfileIntA("PatternOffset", "StepDivisorModifier", (MOD_CONTROL | MOD_ALT), ini.c_str());
    if (m_offsetStepDivisorModifier == (MOD_CONTROL | MOD_SHIFT)) {
        m_offsetStepDivisorModifier = (MOD_CONTROL | MOD_ALT);
    }
    m_offsetStepDivisor = (int)GetPrivateProfileIntA("PatternOffset", "StepDivisor", 1, ini.c_str());
    if (m_offsetStepDivisor < 1 || m_offsetStepDivisor > 9) m_offsetStepDivisor = 1;
    char gapBuf[32] = {0};
    GetPrivateProfileStringA("PatternOffset", "Gap", "0.0", gapBuf, sizeof(gapBuf), ini.c_str());
    m_offsetCloneGap = std::atof(gapBuf);

    m_offsetKeyPreset = GetPrivateProfileIntA("PatternOffset", "KeyPreset", OFFSET_PRESET_NUMPAD, ini.c_str());
    SetOffsetKeyPreset(m_offsetKeyPreset);
    for (int i = 1; i <= 9; ++i) {
        std::string kName = "Key_" + std::to_string(i);
        UINT savedVk = (UINT)GetPrivateProfileIntA("PatternOffset", kName.c_str(), m_offsetKeys[i], ini.c_str());
        if (savedVk != 0) {
            m_offsetKeys[i] = savedVk;
        }
    }
}

void ConfigManager::Save() {
    std::string ini = GetIniPath();

    WritePrivateProfileStringA("Hotkeys", "Copy_VK", std::to_string(m_copyHotkey.vkCode).c_str(), ini.c_str());
    WritePrivateProfileStringA("Hotkeys", "Copy_Mod", std::to_string(m_copyHotkey.modifiers).c_str(), ini.c_str());

    WritePrivateProfileStringA("Hotkeys", "PastePlace_VK", std::to_string(m_pastePlaceHotkey.vkCode).c_str(), ini.c_str());
    WritePrivateProfileStringA("Hotkeys", "PastePlace_Mod", std::to_string(m_pastePlaceHotkey.modifiers).c_str(), ini.c_str());

    WritePrivateProfileStringA("Hotkeys", "PasteCenter_VK", std::to_string(m_pasteCenterHotkey.vkCode).c_str(), ini.c_str());
    WritePrivateProfileStringA("Hotkeys", "PasteCenter_Mod", std::to_string(m_pasteCenterHotkey.modifiers).c_str(), ini.c_str());

    WritePrivateProfileStringA("Hotkeys", "PasteAllArtboards_VK", std::to_string(m_pasteAllArtboardsHotkey.vkCode).c_str(), ini.c_str());
    WritePrivateProfileStringA("Hotkeys", "PasteAllArtboards_Mod", std::to_string(m_pasteAllArtboardsHotkey.modifiers).c_str(), ini.c_str());

    WritePrivateProfileStringA("Hotkeys", "ReflectVert_VK", std::to_string(m_reflectVertHotkey.vkCode).c_str(), ini.c_str());
    WritePrivateProfileStringA("Hotkeys", "ReflectVert_Mod", std::to_string(m_reflectVertHotkey.modifiers).c_str(), ini.c_str());

    WritePrivateProfileStringA("Hotkeys", "ReflectHoriz_VK", std::to_string(m_reflectHorizHotkey.vkCode).c_str(), ini.c_str());
    WritePrivateProfileStringA("Hotkeys", "ReflectHoriz_Mod", std::to_string(m_reflectHorizHotkey.modifiers).c_str(), ini.c_str());

    WritePrivateProfileStringA("General", "SelectAfterPaste", m_selectAfterPaste ? "1" : "0", ini.c_str());
    WritePrivateProfileStringA("General", "Disabled", m_disabled ? "1" : "0", ini.c_str());
    WritePrivateProfileStringA("General", "DisablePlugin", m_disabled ? "1" : "0", ini.c_str());

    WritePrivateProfileStringA("PatternOffset", "Enabled", m_offsetCloneEnabled ? "1" : "0", ini.c_str());
    WritePrivateProfileStringA("PatternOffset", "CloneModifier", std::to_string(m_offsetCloneModifier).c_str(), ini.c_str());
    WritePrivateProfileStringA("PatternOffset", "MoveModifier", std::to_string(m_offsetMoveModifier).c_str(), ini.c_str());
    WritePrivateProfileStringA("PatternOffset", "StepDivisorModifier", std::to_string(m_offsetStepDivisorModifier).c_str(), ini.c_str());
    WritePrivateProfileStringA("PatternOffset", "StepDivisor", std::to_string(m_offsetStepDivisor).c_str(), ini.c_str());
    WritePrivateProfileStringA("PatternOffset", "Modifier", std::to_string(m_offsetCloneModifier).c_str(), ini.c_str());
    WritePrivateProfileStringA("PatternOffset", "Gap", std::to_string(m_offsetCloneGap).c_str(), ini.c_str());
    WritePrivateProfileStringA("PatternOffset", "KeyPreset", std::to_string(m_offsetKeyPreset).c_str(), ini.c_str());
    for (int i = 1; i <= 9; ++i) {
        std::string kName = "Key_" + std::to_string(i);
        WritePrivateProfileStringA("PatternOffset", kName.c_str(), std::to_string(m_offsetKeys[i]).c_str(), ini.c_str());
    }
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
#define IDC_MOD_ALL         211
#define IDC_KEY_ALL         212
#define IDC_MOD_REFLECT_VERT 230
#define IDC_KEY_REFLECT_VERT 231
#define IDC_MOD_REFLECT_HORIZ 232
#define IDC_KEY_REFLECT_HORIZ 233
#define IDC_BTN_OK          207
#define IDC_BTN_CANCEL      208
#define IDC_BTN_DEFAULTS    209
#define IDC_CHK_SELECT      210
#define IDC_CHK_DISABLE     213
#define IDC_CHK_OFFSET      214
#define IDC_MOD_OFFSET      215
#define IDC_COMBO_OFFSET_PRESET 216
#define IDC_MOD_OFFSET_MOVE 217
#define IDC_MOD_OFFSET_STEP 218
#define IDC_EDIT_OFFSET_KEY_1 221
#define IDC_EDIT_OFFSET_KEY_2 222
#define IDC_EDIT_OFFSET_KEY_3 223
#define IDC_EDIT_OFFSET_KEY_4 224
#define IDC_EDIT_OFFSET_KEY_5 225
#define IDC_EDIT_OFFSET_KEY_6 226
#define IDC_EDIT_OFFSET_KEY_7 227
#define IDC_EDIT_OFFSET_KEY_8 228
#define IDC_EDIT_OFFSET_KEY_9 229

static const char* g_offsetPresetNames[] = {
    "Numpad (1..9)",
    "Top Row (1..9)",
    "Letters: QWE / ASD / ZXC",
    "Letters: UIO / JKL / M,.",
    "Custom (Edit 9 keys below)"
};

struct DialogData {
    HotkeyConfig copyHk;
    HotkeyConfig placeHk;
    HotkeyConfig centerHk;
    HotkeyConfig allHk;
    HotkeyConfig reflectVertHk;
    HotkeyConfig reflectHorizHk;
    bool selectAfterPaste;
    bool disabled;
    bool offsetEnabled;
    UINT offsetMod;
    UINT offsetMoveMod;
    UINT offsetStepMod;
    int offsetPreset;
    UINT offsetKeys[10];
    bool saved;
};

static DialogData g_dlgData;
static HFONT g_hDlgFont = NULL;
static bool g_dialogRunning = false;
static bool g_internalUpdatingEdits = false;

static std::string VkToDisplayChar(UINT vk) {
    if (vk >= VK_NUMPAD1 && vk <= VK_NUMPAD9) {
        return std::to_string(vk - VK_NUMPAD1 + 1);
    }
    if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) {
        return std::string(1, (char)vk);
    }
    if (vk == VK_OEM_COMMA) return ",";
    if (vk == VK_OEM_PERIOD) return ".";
    return std::string(1, (char)vk);
}

static UINT CharToVkCode(char ch, int preset) {
    char c = (char)toupper((unsigned char)ch);
    if (preset == OFFSET_PRESET_NUMPAD && c >= '1' && c <= '9') {
        return VK_NUMPAD1 + (c - '1');
    }
    if (c >= 'A' && c <= 'Z') return (UINT)c;
    if (c >= '0' && c <= '9') return (UINT)c;
    if (c == ',') return VK_OEM_COMMA;
    if (c == '.') return VK_OEM_PERIOD;
    return (UINT)c;
}

static void FillPresetKeys(int preset, UINT keys[10]) {
    switch (preset) {
        case OFFSET_PRESET_TOPROW:
            keys[1] = '1'; keys[2] = '2'; keys[3] = '3';
            keys[4] = '4'; keys[5] = '5'; keys[6] = '6';
            keys[7] = '7'; keys[8] = '8'; keys[9] = '9';
            break;
        case OFFSET_PRESET_QWE_ASD_ZXC:
            keys[7] = 'Q'; keys[8] = 'W'; keys[9] = 'E';
            keys[4] = 'A'; keys[5] = 'S'; keys[6] = 'D';
            keys[1] = 'Z'; keys[2] = 'X'; keys[3] = 'C';
            break;
        case OFFSET_PRESET_UIO_JKL_M:
            keys[7] = 'U'; keys[8] = 'I'; keys[9] = 'O';
            keys[4] = 'J'; keys[5] = 'K'; keys[6] = 'L';
            keys[1] = 'M'; keys[2] = VK_OEM_COMMA; keys[3] = VK_OEM_PERIOD;
            break;
        case OFFSET_PRESET_NUMPAD:
        default:
            keys[1] = VK_NUMPAD1; keys[2] = VK_NUMPAD2; keys[3] = VK_NUMPAD3;
            keys[4] = VK_NUMPAD4; keys[5] = VK_NUMPAD5; keys[6] = VK_NUMPAD6;
            keys[7] = VK_NUMPAD7; keys[8] = VK_NUMPAD8; keys[9] = VK_NUMPAD9;
            break;
    }
}

static void UpdateKeyEdits(HWND hDlg) {
    g_internalUpdatingEdits = true;
    for (int i = 1; i <= 9; ++i) {
        HWND hEdit = GetDlgItem(hDlg, IDC_EDIT_OFFSET_KEY_1 + i - 1);
        if (hEdit) {
            std::string s = VkToDisplayChar(g_dlgData.offsetKeys[i]);
            SetWindowTextA(hEdit, s.c_str());
        }
    }
    g_internalUpdatingEdits = false;
}

static void PopulateCombos(HWND hDlg) {
    HWND hModCopy   = GetDlgItem(hDlg, IDC_MOD_COPY);
    HWND hKeyCopy   = GetDlgItem(hDlg, IDC_KEY_COPY);
    HWND hModPlace  = GetDlgItem(hDlg, IDC_MOD_PLACE);
    HWND hKeyPlace  = GetDlgItem(hDlg, IDC_KEY_PLACE);
    HWND hModCenter = GetDlgItem(hDlg, IDC_MOD_CENTER);
    HWND hKeyCenter = GetDlgItem(hDlg, IDC_KEY_CENTER);
    HWND hModAll    = GetDlgItem(hDlg, IDC_MOD_ALL);
    HWND hKeyAll    = GetDlgItem(hDlg, IDC_KEY_ALL);
    HWND hModReflectVert = GetDlgItem(hDlg, IDC_MOD_REFLECT_VERT);
    HWND hKeyReflectVert = GetDlgItem(hDlg, IDC_KEY_REFLECT_VERT);
    HWND hModReflectHoriz = GetDlgItem(hDlg, IDC_MOD_REFLECT_HORIZ);
    HWND hKeyReflectHoriz = GetDlgItem(hDlg, IDC_KEY_REFLECT_HORIZ);
    HWND hModOffset = GetDlgItem(hDlg, IDC_MOD_OFFSET);
    HWND hModOffsetMove = GetDlgItem(hDlg, IDC_MOD_OFFSET_MOVE);
    HWND hModOffsetStep = GetDlgItem(hDlg, IDC_MOD_OFFSET_STEP);
    HWND hPresetOffset = GetDlgItem(hDlg, IDC_COMBO_OFFSET_PRESET);

    HWND modCombos[] = { hModCopy, hModPlace, hModCenter, hModAll, hModReflectVert, hModReflectHoriz, hModOffset, hModOffsetMove, hModOffsetStep };
    HWND keyCombos[] = { hKeyCopy, hKeyPlace, hKeyCenter, hKeyAll, hKeyReflectVert, hKeyReflectHoriz };

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

    if (hPresetOffset) {
        SendMessage(hPresetOffset, CB_RESETCONTENT, 0, 0);
        for (size_t i = 0; i < sizeof(g_offsetPresetNames) / sizeof(g_offsetPresetNames[0]); ++i) {
            SendMessageA(hPresetOffset, CB_ADDSTRING, 0, (LPARAM)g_offsetPresetNames[i]);
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

    selectMod(hModAll, g_dlgData.allHk.modifiers);
    selectKey(hKeyAll, g_dlgData.allHk.vkCode);

    selectMod(hModReflectVert, g_dlgData.reflectVertHk.modifiers);
    selectKey(hKeyReflectVert, g_dlgData.reflectVertHk.vkCode);

    selectMod(hModReflectHoriz, g_dlgData.reflectHorizHk.modifiers);
    selectKey(hKeyReflectHoriz, g_dlgData.reflectHorizHk.vkCode);

    selectMod(hModOffset, g_dlgData.offsetMod);
    selectMod(hModOffsetMove, g_dlgData.offsetMoveMod);
    selectMod(hModOffsetStep, g_dlgData.offsetStepMod);

    if (hPresetOffset) {
        int p = g_dlgData.offsetPreset;
        if (p < 0 || p > 4) p = 0;
        SendMessage(hPresetOffset, CB_SETCURSEL, (WPARAM)p, 0);
    }

    UpdateKeyEdits(hDlg);

    CheckDlgButton(hDlg, IDC_CHK_SELECT, g_dlgData.selectAfterPaste ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHK_DISABLE, g_dlgData.disabled ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, IDC_CHK_OFFSET, g_dlgData.offsetEnabled ? BST_CHECKED : BST_UNCHECKED);
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

                HWND hModAll = GetDlgItem(hwnd, IDC_MOD_ALL);
                HWND hKeyAll = GetDlgItem(hwnd, IDC_KEY_ALL);
                int idxModAll = (int)SendMessage(hModAll, CB_GETCURSEL, 0, 0);
                int idxKeyAll = (int)SendMessage(hKeyAll, CB_GETCURSEL, 0, 0);
                if (idxModAll >= 0 && idxModAll < (int)(sizeof(g_supportedMods) / sizeof(g_supportedMods[0]))) {
                    g_dlgData.allHk.modifiers = g_supportedMods[idxModAll].mod;
                }
                if (idxKeyAll >= 0 && idxKeyAll < (int)(sizeof(g_supportedKeys) / sizeof(g_supportedKeys[0]))) {
                    g_dlgData.allHk.vkCode = g_supportedKeys[idxKeyAll].vk;
                }

                HWND hModReflectVert = GetDlgItem(hwnd, IDC_MOD_REFLECT_VERT);
                HWND hKeyReflectVert = GetDlgItem(hwnd, IDC_KEY_REFLECT_VERT);
                int idxModReflectVert = (int)SendMessage(hModReflectVert, CB_GETCURSEL, 0, 0);
                int idxKeyReflectVert = (int)SendMessage(hKeyReflectVert, CB_GETCURSEL, 0, 0);
                if (idxModReflectVert >= 0 && idxModReflectVert < (int)(sizeof(g_supportedMods) / sizeof(g_supportedMods[0]))) {
                    g_dlgData.reflectVertHk.modifiers = g_supportedMods[idxModReflectVert].mod;
                }
                if (idxKeyReflectVert >= 0 && idxKeyReflectVert < (int)(sizeof(g_supportedKeys) / sizeof(g_supportedKeys[0]))) {
                    g_dlgData.reflectVertHk.vkCode = g_supportedKeys[idxKeyReflectVert].vk;
                }

                HWND hModReflectHoriz = GetDlgItem(hwnd, IDC_MOD_REFLECT_HORIZ);
                HWND hKeyReflectHoriz = GetDlgItem(hwnd, IDC_KEY_REFLECT_HORIZ);
                int idxModReflectHoriz = (int)SendMessage(hModReflectHoriz, CB_GETCURSEL, 0, 0);
                int idxKeyReflectHoriz = (int)SendMessage(hKeyReflectHoriz, CB_GETCURSEL, 0, 0);
                if (idxModReflectHoriz >= 0 && idxModReflectHoriz < (int)(sizeof(g_supportedMods) / sizeof(g_supportedMods[0]))) {
                    g_dlgData.reflectHorizHk.modifiers = g_supportedMods[idxModReflectHoriz].mod;
                }
                if (idxKeyReflectHoriz >= 0 && idxKeyReflectHoriz < (int)(sizeof(g_supportedKeys) / sizeof(g_supportedKeys[0]))) {
                    g_dlgData.reflectHorizHk.vkCode = g_supportedKeys[idxKeyReflectHoriz].vk;
                }

                g_dlgData.selectAfterPaste = (IsDlgButtonChecked(hwnd, IDC_CHK_SELECT) == BST_CHECKED);
                g_dlgData.disabled = (IsDlgButtonChecked(hwnd, IDC_CHK_DISABLE) == BST_CHECKED);
                g_dlgData.offsetEnabled = (IsDlgButtonChecked(hwnd, IDC_CHK_OFFSET) == BST_CHECKED);

                HWND hModOffset = GetDlgItem(hwnd, IDC_MOD_OFFSET);
                int idxModOffset = (int)SendMessage(hModOffset, CB_GETCURSEL, 0, 0);
                if (idxModOffset >= 0 && idxModOffset < (int)(sizeof(g_supportedMods) / sizeof(g_supportedMods[0]))) {
                    g_dlgData.offsetMod = g_supportedMods[idxModOffset].mod;
                }

                HWND hModOffsetMove = GetDlgItem(hwnd, IDC_MOD_OFFSET_MOVE);
                int idxModOffsetMove = (int)SendMessage(hModOffsetMove, CB_GETCURSEL, 0, 0);
                if (idxModOffsetMove >= 0 && idxModOffsetMove < (int)(sizeof(g_supportedMods) / sizeof(g_supportedMods[0]))) {
                    g_dlgData.offsetMoveMod = g_supportedMods[idxModOffsetMove].mod;
                }

                HWND hModOffsetStep = GetDlgItem(hwnd, IDC_MOD_OFFSET_STEP);
                int idxModOffsetStep = (int)SendMessage(hModOffsetStep, CB_GETCURSEL, 0, 0);
                if (idxModOffsetStep >= 0 && idxModOffsetStep < (int)(sizeof(g_supportedMods) / sizeof(g_supportedMods[0]))) {
                    g_dlgData.offsetStepMod = g_supportedMods[idxModOffsetStep].mod;
                }

                HWND hPreset = GetDlgItem(hwnd, IDC_COMBO_OFFSET_PRESET);
                int idxPreset = (int)SendMessage(hPreset, CB_GETCURSEL, 0, 0);
                if (idxPreset >= 0 && idxPreset <= 4) {
                    g_dlgData.offsetPreset = idxPreset;
                }

                for (int i = 1; i <= 9; ++i) {
                    HWND hEdit = GetDlgItem(hwnd, IDC_EDIT_OFFSET_KEY_1 + i - 1);
                    if (hEdit) {
                        char txt[16] = {0};
                        GetWindowTextA(hEdit, txt, sizeof(txt));
                        if (txt[0]) {
                            g_dlgData.offsetKeys[i] = CharToVkCode(txt[0], g_dlgData.offsetPreset);
                        }
                    }
                }

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
                g_dlgData.allHk = { VK_F8, 0 };
                g_dlgData.reflectVertHk = { VK_F9, 0 };
                g_dlgData.reflectHorizHk = { VK_F10, 0 };
                g_dlgData.selectAfterPaste = true;
                g_dlgData.disabled = false;
                g_dlgData.offsetEnabled = true;
                g_dlgData.offsetMod = MOD_CONTROL;
                g_dlgData.offsetMoveMod = MOD_ALT;
                g_dlgData.offsetStepMod = MOD_CONTROL | MOD_ALT;
                g_dlgData.offsetPreset = OFFSET_PRESET_NUMPAD;
                FillPresetKeys(OFFSET_PRESET_NUMPAD, g_dlgData.offsetKeys);
                PopulateCombos(hwnd);
                return 0;
            } else if (id == IDC_COMBO_OFFSET_PRESET && HIWORD(wParam) == CBN_SELCHANGE) {
                HWND hPreset = (HWND)lParam;
                int idx = (int)SendMessage(hPreset, CB_GETCURSEL, 0, 0);
                if (idx >= 0 && idx <= 3) {
                    g_dlgData.offsetPreset = idx;
                    FillPresetKeys(idx, g_dlgData.offsetKeys);
                    UpdateKeyEdits(hwnd);
                } else if (idx == 4) {
                    g_dlgData.offsetPreset = OFFSET_PRESET_CUSTOM;
                }
                return 0;
            } else if (id >= IDC_EDIT_OFFSET_KEY_1 && id <= IDC_EDIT_OFFSET_KEY_9 && HIWORD(wParam) == EN_CHANGE) {
                if (!g_internalUpdatingEdits) {
                    int dir = id - IDC_EDIT_OFFSET_KEY_1 + 1;
                    HWND hEdit = (HWND)lParam;
                    char txt[16] = {0};
                    GetWindowTextA(hEdit, txt, sizeof(txt));
                    if (txt[0]) {
                        g_dlgData.offsetKeys[dir] = CharToVkCode(txt[0], OFFSET_PRESET_CUSTOM);
                        g_dlgData.offsetPreset = OFFSET_PRESET_CUSTOM;
                        HWND hPreset = GetDlgItem(hwnd, IDC_COMBO_OFFSET_PRESET);
                        if (hPreset) {
                            SendMessage(hPreset, CB_SETCURSEL, (WPARAM)OFFSET_PRESET_CUSTOM, 0);
                        }
                    }
                }
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

static float GetDialogScale(HWND hwnd) {
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

bool ConfigManager::ShowSettingsDialog(HWND parentHwnd, float currentScale) {
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
    g_dlgData.allHk = m_pasteAllArtboardsHotkey;
    g_dlgData.reflectVertHk = m_reflectVertHotkey;
    g_dlgData.reflectHorizHk = m_reflectHorizHotkey;
    g_dlgData.selectAfterPaste = m_selectAfterPaste;
    g_dlgData.disabled = m_disabled;
    g_dlgData.offsetEnabled = m_offsetCloneEnabled;
    g_dlgData.offsetMod = m_offsetCloneModifier;
    g_dlgData.offsetMoveMod = m_offsetMoveModifier;
    g_dlgData.offsetStepMod = m_offsetStepDivisorModifier;
    g_dlgData.offsetPreset = m_offsetKeyPreset;
    for (int i = 1; i <= 9; ++i) g_dlgData.offsetKeys[i] = m_offsetKeys[i];
    g_dlgData.saved = false;

    // Resolve top-level parent window for true modality
    HWND hOwner = parentHwnd;
    if (hOwner && IsWindow(hOwner)) {
        HWND hRoot = GetAncestor(hOwner, GA_ROOT);
        if (hRoot && IsWindow(hRoot)) {
            hOwner = hRoot;
        }
    }

    float scale = currentScale;
    if (scale < 0.75f || scale > 4.0f) {
        scale = GetDialogScale(parentHwnd ? parentHwnd : hOwner);
    }
    auto S = [scale](int v) -> int { return (int)(v * scale + 0.5f); };

    int clientW = S(346);
    int clientH = S(386);
    RECT rc = { 0, 0, clientW, clientH };
    DWORD dwStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
    DWORD dwExStyle = WS_EX_DLGMODALFRAME | WS_EX_TOPMOST;
    AdjustWindowRectEx(&rc, dwStyle, FALSE, dwExStyle);
    int dlgW = rc.right - rc.left;
    int dlgH = rc.bottom - rc.top;

    // Center dialog relative to owner or desktop
    RECT rcParent;
    if (hOwner && IsWindow(hOwner)) {
        GetWindowRect(hOwner, &rcParent);
    } else {
        GetWindowRect(GetDesktopWindow(), &rcParent);
    }
    int x = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int y = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;
    if (x < 50) x = 50;
    if (y < 50) y = 50;

    HWND hDlg = CreateWindowExA(
        dwExStyle,
        "ClipboardPasteSettingsDlg",
        "Clipboard Paste - Preferences",
        dwStyle,
        x, y, dlgW, dlgH,
        hOwner, NULL, hInst, NULL
    );

    if (!hDlg) return false;

    if (g_hDlgFont) {
        DeleteObject(g_hDlgFont);
        g_hDlgFont = NULL;
    }
    int fontH = -(int)(11.0f * scale + 0.5f);
    g_hDlgFont = CreateFontA(fontH, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");

    auto addLabel = [&](const char* text, int lx, int ly, int lw, int lh) {
        HWND lbl = CreateWindowExA(0, "STATIC", text, WS_CHILD | WS_VISIBLE | SS_LEFT,
                                   lx, ly, lw, lh, hDlg, NULL, hInst, NULL);
        SendMessage(lbl, WM_SETFONT, (WPARAM)g_hDlgFont, TRUE);
        return lbl;
    };

    auto makeCombo = [&](int cid, int cx, int cy, int cw) {
        HWND h = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
                                 cx, cy, cw, S(160), hDlg, (HMENU)(INT_PTR)cid, hInst, NULL);
        SendMessage(h, WM_SETFONT, (WPARAM)g_hDlgFont, TRUE);
        SendMessageA(h, CB_SETITEMHEIGHT, (WPARAM)-1, (LPARAM)S(20));
        SendMessageA(h, CB_SETITEMHEIGHT, (WPARAM)0,  (LPARAM)S(18));
        return h;
    };

    addLabel("Configure Hotkeys:", S(14), S(10), S(312), S(16));

    // Row 1: Copy
    addLabel("1. Copy to Buffer:", S(14), S(32), S(140), S(18));
    HWND hModCopy = makeCombo(IDC_MOD_COPY, S(158), S(30), S(94));
    HWND hKeyCopy = makeCombo(IDC_KEY_COPY, S(256), S(30), S(76));

    // Row 2: Paste Place
    addLabel("2. Paste in Place:", S(14), S(56), S(140), S(18));
    HWND hModPlace = makeCombo(IDC_MOD_PLACE, S(158), S(54), S(94));
    HWND hKeyPlace = makeCombo(IDC_KEY_PLACE, S(256), S(54), S(76));

    // Row 3: Paste Center
    addLabel("3. Paste to Center:", S(14), S(80), S(140), S(18));
    HWND hModCenter = makeCombo(IDC_MOD_CENTER, S(158), S(78), S(94));
    HWND hKeyCenter = makeCombo(IDC_KEY_CENTER, S(256), S(78), S(76));

    // Row 4: Paste on All Artboards
    addLabel("4. Paste on All Artboards:", S(14), S(104), S(140), S(18));
    HWND hModAll = makeCombo(IDC_MOD_ALL, S(158), S(102), S(94));
    HWND hKeyAll = makeCombo(IDC_KEY_ALL, S(256), S(102), S(76));

    // Row 5: Reflect Vertical
    addLabel("5. Reflect Vertical:", S(14), S(128), S(140), S(18));
    HWND hModReflectVert = makeCombo(IDC_MOD_REFLECT_VERT, S(158), S(126), S(94));
    HWND hKeyReflectVert = makeCombo(IDC_KEY_REFLECT_VERT, S(256), S(126), S(76));

    // Row 6: Reflect Horizontal
    addLabel("6. Reflect Horizontal:", S(14), S(152), S(140), S(18));
    HWND hModReflectHoriz = makeCombo(IDC_MOD_REFLECT_HORIZ, S(158), S(150), S(94));
    HWND hKeyReflectHoriz = makeCombo(IDC_KEY_REFLECT_HORIZ, S(256), S(150), S(76));

    // Row 7: Pattern Offset Checkbox (renamed to 7.)
    HWND hChkOffset = CreateWindowExA(0, "BUTTON", "7. Pattern Offset (Hotkeys):",
                                      WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                                      S(14), S(174), S(312), S(18), hDlg, (HMENU)IDC_CHK_OFFSET, hInst, NULL);
    SendMessage(hChkOffset, WM_SETFONT, (WPARAM)g_hDlgFont, TRUE);

    // Row 8: Clone modifier (left) & Preset dropdown (right)
    addLabel("Clone:", S(24), S(196), S(42), S(18));
    HWND hModOffset = makeCombo(IDC_MOD_OFFSET, S(68), S(194), S(88));

    addLabel("Keys:", S(164), S(196), S(34), S(18));
    HWND hPresetOffset = makeCombo(IDC_COMBO_OFFSET_PRESET, S(200), S(194), S(132));

    // Row 9: Move modifier (left)
    addLabel("Move:", S(24), S(222), S(42), S(18));
    HWND hModOffsetMove = makeCombo(IDC_MOD_OFFSET_MOVE, S(68), S(220), S(88));

    // Row 10: Step modifier (left)
    addLabel("Step:", S(24), S(248), S(42), S(18));
    HWND hModOffsetStep = makeCombo(IDC_MOD_OFFSET_STEP, S(68), S(246), S(88));

    // Keypad Layout (3x3 Matrix)
    addLabel("Direction Keys (3x3):", S(24), S(272), S(160), S(18));

    int gridDirs[3][3] = {
        { 7, 8, 9 },
        { 4, 5, 6 },
        { 1, 2, 3 }
    };
    int editW = S(38);
    int editH = S(20);
    int editGap = S(4);
    int gridStartX = S(200);
    int gridStartY = S(220);

    for (int r = 0; r < 3; ++r) {
        int ey = gridStartY + r * (editH + editGap);
        for (int c = 0; c < 3; ++c) {
            int dir = gridDirs[r][c];
            int ex = gridStartX + c * (editW + editGap);
            HWND hEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                                         WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_CENTER | ES_UPPERCASE,
                                         ex, ey, editW, editH, hDlg, (HMENU)(INT_PTR)(IDC_EDIT_OFFSET_KEY_1 + dir - 1), hInst, NULL);
            SendMessage(hEdit, WM_SETFONT, (WPARAM)g_hDlgFont, TRUE);
            SendMessage(hEdit, EM_SETLIMITTEXT, 4, 0);
        }
    }

    // Checkbox 1: Select object(s) after paste
    HWND hChkSelect = CreateWindowExA(0, "BUTTON", "Select object(s) after paste",
                                      WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                                      S(14), S(296), S(312), S(18), hDlg, (HMENU)IDC_CHK_SELECT, hInst, NULL);
    SendMessage(hChkSelect, WM_SETFONT, (WPARAM)g_hDlgFont, TRUE);

    // Checkbox 2: Disable plugin (ignore hotkeys)
    HWND hChkDisable = CreateWindowExA(0, "BUTTON", "Disable plugin (ignore hotkeys)",
                                       WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                                       S(14), S(318), S(312), S(18), hDlg, (HMENU)IDC_CHK_DISABLE, hInst, NULL);
    SendMessage(hChkDisable, WM_SETFONT, (WPARAM)g_hDlgFont, TRUE);

    // Bottom buttons (strictly in English)
    HWND hBtnDefaults = CreateWindowExA(0, "BUTTON", "Defaults", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                                        S(14), S(350), S(70), S(24), hDlg, (HMENU)IDC_BTN_DEFAULTS, hInst, NULL);
    SendMessage(hBtnDefaults, WM_SETFONT, (WPARAM)g_hDlgFont, TRUE);

    HWND hBtnOk = CreateWindowExA(0, "BUTTON", "Save", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
                                  S(194), S(350), S(66), S(24), hDlg, (HMENU)IDC_BTN_OK, hInst, NULL);
    SendMessage(hBtnOk, WM_SETFONT, (WPARAM)g_hDlgFont, TRUE);

    HWND hBtnCancel = CreateWindowExA(0, "BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                                      S(266), S(350), S(66), S(24), hDlg, (HMENU)IDC_BTN_CANCEL, hInst, NULL);
    SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)g_hDlgFont, TRUE);

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
        m_pasteAllArtboardsHotkey = g_dlgData.allHk;
        m_reflectVertHotkey = g_dlgData.reflectVertHk;
        m_reflectHorizHotkey = g_dlgData.reflectHorizHk;
        m_selectAfterPaste = g_dlgData.selectAfterPaste;
        m_disabled = g_dlgData.disabled;
        m_offsetCloneEnabled = g_dlgData.offsetEnabled;
        m_offsetCloneModifier = g_dlgData.offsetMod;
        m_offsetMoveModifier = g_dlgData.offsetMoveMod;
        m_offsetStepDivisorModifier = g_dlgData.offsetStepMod;
        m_offsetKeyPreset = g_dlgData.offsetPreset;
        for (int i = 1; i <= 9; ++i) m_offsetKeys[i] = g_dlgData.offsetKeys[i];
        Save();
        return true;
    }
    return false;
}
