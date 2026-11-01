#pragma once

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <windows.h>
#include <string>

struct HotkeyConfig {
    UINT vkCode;     // Virtual Key Code (e.g. VK_F5, 'C', VK_INSERT)
    UINT modifiers;  // Bitmask: 0 = None, 1 = Ctrl, 2 = Alt, 4 = Shift, or combinations
};

class ConfigManager {
public:
    static ConfigManager& GetInstance();

    void Load();
    void Save();

    HotkeyConfig GetCopyHotkey() const { return m_copyHotkey; }
    HotkeyConfig GetPastePlaceHotkey() const { return m_pastePlaceHotkey; }
    HotkeyConfig GetPasteCenterHotkey() const { return m_pasteCenterHotkey; }
    HotkeyConfig GetPasteAllArtboardsHotkey() const { return m_pasteAllArtboardsHotkey; }
    bool GetSelectAfterPaste() const { return m_selectAfterPaste; }
    bool IsPluginDisabled() const { return m_disabled; }
    bool GetDisabled() const { return m_disabled; }

    void SetCopyHotkey(const HotkeyConfig& hk) { m_copyHotkey = hk; }
    void SetPastePlaceHotkey(const HotkeyConfig& hk) { m_pastePlaceHotkey = hk; }
    void SetPasteCenterHotkey(const HotkeyConfig& hk) { m_pasteCenterHotkey = hk; }
    void SetPasteAllArtboardsHotkey(const HotkeyConfig& hk) { m_pasteAllArtboardsHotkey = hk; }
    void SetSelectAfterPaste(bool b) { m_selectAfterPaste = b; }
    void SetPluginDisabled(bool disabled) { m_disabled = disabled; }

    std::string GetHotkeyDisplayString(const HotkeyConfig& hk);
    bool MatchHotkey(const HotkeyConfig& hk, UINT vkCode, UINT currentModifiers);

    // Displays the modal Preferences / Hotkeys dialog
    bool ShowSettingsDialog(HWND parentHwnd);
    bool IsDialogActive() const { return m_dialogActive; }

    std::string GetIniPath() const;

private:
    ConfigManager();
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    HotkeyConfig m_copyHotkey;
    HotkeyConfig m_pastePlaceHotkey;
    HotkeyConfig m_pasteCenterHotkey;
    HotkeyConfig m_pasteAllArtboardsHotkey;
    bool m_selectAfterPaste = true;
    bool m_disabled = false;
    bool m_dialogActive = false;
};

#endif // CONFIG_MANAGER_H
