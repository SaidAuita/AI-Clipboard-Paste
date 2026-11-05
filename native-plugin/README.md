# Clipboard Paste — Native C++ Plugin (.aip) v1.25

<p align="center">
  <a href="README_RU.md"><b>🇷🇺 Читать документацию на русском</b></a>
</p>

High-speed native C++ vector duplication and transformation plugin for **Adobe Illustrator 2020 – 2026+**.

<p align="center">
  <img src="../images/Clipboard Paste Plugin_01.png" alt="Clipboard Tab" width="260">
  &nbsp;&nbsp;
  <img src="../images/Clipboard Paste Plugin_02.png" alt="Offset Clone Tab" width="260">
  &nbsp;&nbsp;
  <img src="../images/Clipboard Paste Plugin_03.png" alt="Preferences Dialog" width="340">
</p>

## 🚀 Key Features and Advantages
- **🎯 Rapid Pattern, Pixel Art & Grid Creation (Tab «Offset Clone»):**
  - Instantly clone selected artwork edge-to-edge by its exact dimensions via `Ctrl + [1..9]` (or shift without duplicating via `Alt + [1..9]`).
  - No need to use the slow `Move...` dialog (`Shift+Ctrl+M`): dimensions are calculated on the fly. Build isometric art, tessellations, and tile patterns in seconds!
  - Built-in presets for laptop keyboards without Numpad: top number row `1..9`, letter grids `QWE/ASD/ZXC`, `UIO/JKL/M`, or custom keys.
- **🪞 Hotkey Reflection (`F9` / `F10`):**
  - `F9`: Reflect Vertical (flip left/right centered on selection).
  - `F10`: Reflect Horizontal (flip top/bottom centered on selection).
- **Global Hotkeys Out of the Box:** Configurable hotkeys (`F5` Copy, `F6` Paste In-Place, `F7` Paste Center, `F8` Paste All Artboards) work instantly via thread-level Windows hooks. No panel clicking or focus switches required.
- **🛡️ 100% Stability on Complex Artwork:** Resolved crash when pasting Compound Paths, Clipping Masks, and grouped artwork with appearance styles (selection is safely applied to root container level, preventing conflict with `StrokeOffset.aip`).
- **Hotkey Disable Toggle:** Dedicated option in **Settings...** (`Disable plugin (ignore hotkeys)`) to temporarily disable hotkey interception.
- **No Flashing Helper Panels:** Operates purely inside the Illustrator process with zero visual disruption or popup windows.
- **Pixel-Perfect Native UI:** Fully custom-rendered dockable palette matching Illustrator's Spectrum UI (both dark and light themes).
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
