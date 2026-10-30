# Clipboard Paste — Native C++ Plugin (.aip)

High-speed native C++ vector duplicator plugin for **Adobe Illustrator 2020 – 2026+**.

## 🚀 Key Advantages Over CEP Extension
- **Global Hotkeys Out of the Box:** Configurable hotkeys (`F5`, `F6`, `F7` or custom combinations) work instantly via thread-level Windows hooks. No panel clicking or focus switches required.
- **No Flashing Helper Panels:** Unlike CEP extensions that rely on auxiliary micro-panels to trigger hotkeys, the native plugin operates purely inside the Illustrator process with zero visual disruption.
- **Pixel-Perfect Native UI:** Fully custom-rendered dockable palette matching Illustrator's Spectrum UI (both dark and light themes, including automatic adaptation when brightness changes).
- **Direct Memory DOM Execution:** Deep duplication (`sAIArt->DuplicateArt`) takes <1 ms without Windows OLE serialization.

---

## 📁 Directory Structure
```
native-plugin/
├── src/                      # C++17 source code
│   ├── PluginMain.cpp        # SweetPea entry point
│   ├── ClipboardPastePlugin  # Panel lifecycle, UI rendering, event handling
│   ├── BufferManager         # Vector duplication engine (sAIArt)
│   ├── ConfigManager         # Hotkey settings persistence (Win32 INI)
│   └── Suites                # Illustrator SDK Suite pointers
├── resources/                # Windows resources (.rc, .pipl, SVG icons)
├── build.bat                 # MSVC x64 build script
├── build_dev.js              # Dev packaging script (generates versioned zip)
├── Install_Plugin.bat        # Automated installer (detects AI 2020..2026+)
├── Uninstall_Plugin.bat      # Safe uninstaller
└── ClipboardPaste.aip        # Prebuilt 64-bit binary
```

---

## 🛠️ Building from Source
### Prerequisites
- **Windows 10 / 11 x64**
- **Microsoft Visual Studio 2022** (with C++ desktop development workload)
- **Adobe Illustrator SDK** (CC 2020 or higher)

### Build Steps
1. Set the `ILLUSTRATOR_SDK` environment variable to your SDK path (optional if using default structure):
   ```cmd
   set ILLUSTRATOR_SDK=C:\path\to\Adobe Illustrator 2026 SDK
   ```
2. Run `build.bat` in the command prompt:
   ```cmd
   build.bat
   ```
3. The compiled plugin will be created at `build\x64\ClipboardPaste.aip`.

---

## 📦 Installation
Simply run `Install_Plugin.bat` with administrator privileges. It will automatically detect all installed versions of Adobe Illustrator (2020–2026+) and copy `ClipboardPaste.aip` into the `Plug-ins` folder.
