#pragma once

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <windows.h>
#include <string>

struct HotkeyConfig {
    UINT vkCode;     // Virtual Key Code (e.g. VK_F5, 'C', VK_INSERT)
    UINT modifiers;  // Bitmask: 0 = None, 1 = Ctrl, 2 = Alt, 4 = Shift, or combinations
};

enum OffsetKeyPreset {
    OFFSET_PRESET_NUMPAD = 0,
    OFFSET_PRESET_TOPROW = 1,
    OFFSET_PRESET_QWE_ASD_ZXC = 2,
    OFFSET_PRESET_UIO_JKL_M = 3,
    OFFSET_PRESET_CUSTOM = 4
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
    HotkeyConfig GetReflectVertHotkey() const { return m_reflectVertHotkey; }
    HotkeyConfig GetReflectHorizHotkey() const { return m_reflectHorizHotkey; }
    bool GetSelectAfterPaste() const { return m_selectAfterPaste; }
    bool IsPluginDisabled() const { return m_disabled; }
    bool GetDisabled() const { return m_disabled; }

    // Pattern Offset settings
    bool IsOffsetCloneHotkeysEnabled() const { return m_offsetCloneEnabled; }
    UINT GetOffsetCloneModifier() const { return m_offsetCloneModifier; }
    UINT GetOffsetMoveModifier() const { return m_offsetMoveModifier; }
    double GetOffsetCloneGap() const { return m_offsetCloneGap; }
    int GetOffsetKeyPreset() const { return m_offsetKeyPreset; }
    UINT GetOffsetKey(int dir) const;
    std::string GetOffsetKeyName(int dir) const;

    void SetCopyHotkey(const HotkeyConfig& hk) { m_copyHotkey = hk; }
    void SetPastePlaceHotkey(const HotkeyConfig& hk) { m_pastePlaceHotkey = hk; }
    void SetPasteCenterHotkey(const HotkeyConfig& hk) { m_pasteCenterHotkey = hk; }
    void SetPasteAllArtboardsHotkey(const HotkeyConfig& hk) { m_pasteAllArtboardsHotkey = hk; }
    void SetReflectVertHotkey(const HotkeyConfig& hk) { m_reflectVertHotkey = hk; }
    void SetReflectHorizHotkey(const HotkeyConfig& hk) { m_reflectHorizHotkey = hk; }
    void SetSelectAfterPaste(bool b) { m_selectAfterPaste = b; }
    void SetPluginDisabled(bool disabled) { m_disabled = disabled; }
    void SetOffsetCloneHotkeysEnabled(bool b) { m_offsetCloneEnabled = b; }
    void SetOffsetCloneModifier(UINT m) { m_offsetCloneModifier = m; }
    void SetOffsetMoveModifier(UINT m) { m_offsetMoveModifier = m; }
    void SetOffsetCloneGap(double g) { m_offsetCloneGap = g; }
    void SetOffsetKeyPreset(int preset);
    void SetOffsetKey(int dir, UINT vk);

    std::string GetHotkeyDisplayString(const HotkeyConfig& hk);
    bool MatchHotkey(const HotkeyConfig& hk, UINT vkCode, UINT currentModifiers);

    // Displays the modal Preferences / Hotkeys dialog
    bool ShowSettingsDialog(HWND parentHwnd, float currentScale = 0.0f);
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
    HotkeyConfig m_reflectVertHotkey;
    HotkeyConfig m_reflectHorizHotkey;
    bool m_selectAfterPaste = true;
    bool m_disabled = false;
    bool m_offsetCloneEnabled = true;
    UINT m_offsetCloneModifier = MOD_CONTROL;
    UINT m_offsetMoveModifier = MOD_ALT;
    double m_offsetCloneGap = 0.0;
    int m_offsetKeyPreset = OFFSET_PRESET_NUMPAD;
    UINT m_offsetKeys[10]; // index 1..9
    bool m_dialogActive = false;
};

#endif // CONFIG_MANAGER_H
