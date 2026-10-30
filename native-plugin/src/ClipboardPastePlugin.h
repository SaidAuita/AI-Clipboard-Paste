#pragma once

#ifndef CLIPBOARD_PASTE_PLUGIN_H
#define CLIPBOARD_PASTE_PLUGIN_H

#include "Suites.h"
#include "BufferManager.h"

class ClipboardPastePlugin {
public:
    static ClipboardPastePlugin& GetInstance();

    SPErr StartupPlugin(void* message);
    SPErr ShutdownPlugin(void* message);
    SPErr GoMenuItem(void* message);
    SPErr UpdateMenuItem(void* message);
    SPErr Notify(void* message);

    void SetPluginRef(SPPluginRef pluginRef) { m_pluginRef = pluginRef; }
    SPPluginRef GetPluginRef() const { return m_pluginRef; }
    HWND GetPanelHWND() const { return m_hPanelDlg; }
    void OnVisibilityChanged(bool isVisible);

    void UpdateUI();
    void UpdateButtonsText();

private:
    ClipboardPastePlugin();
    ~ClipboardPastePlugin();
    ClipboardPastePlugin(const ClipboardPastePlugin&) = delete;
    ClipboardPastePlugin& operator=(const ClipboardPastePlugin&) = delete;

    SPErr AddMenuItems();
    SPErr CreateDockablePanel();

    SPPluginRef       m_pluginRef;
    AIPanelRef        m_panelRef;
    HWND              m_hPanelDlg;
    AINotifierHandle  m_hBrightnessNotifier;

    AIMenuItemHandle  m_hMenuWindow;
    AIMenuItemHandle  m_hMenuCopy;
    AIMenuItemHandle  m_hMenuPasteInPlace;
    AIMenuItemHandle  m_hMenuPasteCenter;
    AIMenuItemHandle  m_hMenuClear;
    AIMenuItemHandle  m_hMenuAbout;
};

#endif // CLIPBOARD_PASTE_PLUGIN_H
