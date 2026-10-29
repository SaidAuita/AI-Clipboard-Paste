# AI-Clipboard-Paste 🚀

<p align="center">
  <img src="icons/icon_128.png" width="96" height="96" alt="AI-Clipboard-Paste Logo">
</p>

<p align="center">
  <b>Instant, rock-solid vector duplication in Adobe Illustrator — completely bypassing the broken Windows OLE clipboard.</b><br>
  <i>100% Vector Fidelity • In-Place Alignment • Global Hotkeys (F5 / F6 / F7) • 8 UI Languages</i>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/version-v1.0.12-blue.svg?style=flat-square" alt="Version">
  <img src="https://img.shields.io/badge/Adobe%20Illustrator-2020--2026%2B-orange.svg?style=flat-square" alt="Illustrator Compatibility">
  <img src="https://img.shields.io/badge/Platform-Windows%20%7C%20macOS-lightgrey.svg?style=flat-square" alt="OS Support">
  <img src="https://img.shields.io/badge/Architecture-In--Memory%20DOM%20(No%20OLE)-success.svg?style=flat-square" alt="Architecture">
  <img src="https://img.shields.io/badge/License-MIT-green.svg?style=flat-square" alt="License">
</p>

<p align="center">
  <a href="README_RU.md"><b>🇷🇺 Читать документацию на русском языке</b></a> | 
  <a href="https://github.com/SaidAuita/AI-Clipboard-Paste/releases/latest"><b>📦 Download Latest Release (v1.0.12)</b></a>
</p>

---

## ⚡ The Core Problem: Illustrator Clipboard Failure

During long, intensive design and prepress sessions in Adobe Illustrator (especially on Windows 10/11), users frequently experience a catastrophic vector clipboard breakdown:

1. **Silent Failure:** You select vector artwork and press `Ctrl+C` (or *Edit ➔ Copy*). No error message appears.
2. **Nothing Pastes:** When you press `Ctrl+V` (or *Edit ➔ Paste in Place*), either nothing happens at all, or Illustrator pastes stale data that was copied hours ago.
3. **Persists in New Documents:** Pressing `Ctrl+C` even on a simple rectangle in a fresh document (`Ctrl+N`) fails.
4. **Productivity Loss:** The only workaround was forcefully restarting `Illustrator.exe`, causing loss of undo history and disrupting work.

---

## 🔬 Diagnostic Audit & Proof: How the Cause Was Discovered

To diagnose this issue systematically, a low-level diagnostic monitor — **[AI-ClipboardDoctor](tools/AI-ClipboardDoctor/)** — was developed using the Windows API to audit clipboard events in real time.

```text
================================================================================
  EVENT #5 — [CTRL+C] — INFO
  Timestamp: 23:44:53.731 | Process: Illustrator (PID: 57224)
  GDI Handles: 2,292 | User Handles: 4,545
  State: Awaiting Windows Clipboard Data Publication (Sequence: #5545).
================================================================================

================================================================================
  EVENT #6 — [WARN: NO VECTORS] — WARNING
  Timestamp: 23:44:53.802 | Latency: 71 ms
  Summary: WARNING: Vector Formats Missing in Clipboard!
  Owner:        Illustrator (PID: 57224)
  Locked:       NO (Clipboard is free)
  Revision:     Sequence #5546
  Total Formats: 0
  Total Size:   0.0 KB
  Published Formats: [EMPTY — 0 formats]
================================================================================
```

### Key Technical Findings:
* **GDI/User Handles are Healthy:** GDI count was 2,292 and User count was 4,545 (far below the 10,000 Windows limit). The failure is not caused by OS handle exhaustion.
* **No Third-Party Locking:** The clipboard was not locked or held by external apps (browsers, messengers, or clipboard managers). The clipboard owner was strictly `Illustrator.exe`.
* **Root Cause Located:** Upon pressing `Ctrl+C`, Illustrator calls Windows `OpenClipboard()` and `EmptyClipboard()`, advancing the OS sequence counter (`#5545 ➔ #5546`). However, during internal serialization of vector geometry into standard clipboard streams (`AICB`, `Adobe Illustrator Private Format`, `PDF`), an unhandled exception occurs inside Illustrator's internal serialization engine. Illustrator prematurely invokes `CloseClipboard()`, leaving the system clipboard **completely empty (0 formats, 0.0 KB)**.
* **Paste Engine is 100% Functional:** When synthetic `AICB` vector data was injected directly into the clipboard via `AI-ClipboardDoctor`, pressing `Ctrl+V` in Illustrator instantly and flawlessly pasted the test vector. 
* **Conclusion:** The issue is isolated strictly to **Illustrator's internal vector copy/serialization singleton**, which becomes corrupted and cannot recover without restarting the process.

*(The full audit report is available in [`tools/AI-ClipboardDoctor/illustrator_clipboard_failure_analysis.md`](tools/AI-ClipboardDoctor/illustrator_clipboard_failure_analysis.md)).*

---

## 💡 The Solution: AI-Clipboard-Paste

**AI-Clipboard-Paste** provides a permanent fix by **completely bypassing the Windows OLE clipboard and Illustrator's fragile internal serializer**:

* ⚡ **Direct In-Memory Duplication:** All copy and paste operations occur entirely within Illustrator's memory space via the high-speed ExtendScript DOM engine (`CP_SHARED`).
* 🎯 **100% Vector Fidelity:** Flawless preservation of CMYK/Spot swatches, overprints, transparency blending, complex compound paths, and clipping masks.
* 📍 **In-Place Precision:** Objects align exactly to their original `[x, y]` artboard coordinates or can be centered on the target view.
* 🚀 **5–10× Faster:** Eliminates the latency of serializing and deserializing heavy vector payloads through the operating system.
* 🛡️ **Zero Interference:** Never affected by external applications, clipboard managers, or browser activity.

---

## 📸 Interface & Workflows

### 1. Two-Step Independent Buffer (Tab: «2-Step Buffer»)
Classic clipboard workflow: copy artwork in Document A ➔ switch to any other document or layer ➔ paste in place or centered.

<p align="center">
  <img src="images/Clipboard Paste_01.png" alt="2-Step Buffer Tab" width="380">
</p>

### 2. One-Click Direct Transfer (Tab: «1-Click Direct»)
Instant transfer without intermediate steps: directly move or copy selection to a chosen layer, create a new layer above, or send to another open document.

<p align="center">
  <img src="images/Clipboard Paste_00.png" alt="1-Click Direct Tab" width="380">
</p>

### 3. Standalone Micro-Panels for Global Hotkeys (F5, F6, F7)
Three lightweight companion micro-panels featuring high-visibility action buttons and live status indicators. They can be docked in a narrow toolbar or triggered globally:

| `CP_copy` (F5) | `CP_place` (F6) | `CP_paste` (F7) |
|:---:|:---:|:---:|
| <img src="images/Clipboard Paste_02.png" width="280" alt="CP_copy F5"> | <img src="images/Clipboard Paste_03.png" width="280" alt="CP_place F6"> | <img src="images/Clipboard Paste_04.png" width="280" alt="CP_paste F7"> |
| **Copy to Buffer** | **Paste In Place** | **Paste In Center** |

---

## ⌨️ Configuring Global Hotkeys (F5 / F6 / F7)

Adobe CEP panels cannot capture global keystrokes when the panel is out of focus. This limitation is solved by registering three native Illustrator menu commands:
* `CP_copy`
* `CP_place`
* `CP_paste`

By pairing these commands with Illustrator **Actions**, you get true single-key global shortcuts that work from anywhere in your document!

### Method 1: Load Pre-Built Actions (Fast & Recommended)
1. In Adobe Illustrator, open the Actions panel: `Window` ➔ `Actions`.
2. In the panel flyout menu, choose **«Load Actions...»**.
3. Select the included action set file:  
   [`actions/ClipboardPaste_Shortcuts.aia`](actions/ClipboardPaste_Shortcuts.aia)
4. A new **Clipboard Paste** folder will appear with pre-assigned function keys:
   * **F5** ➔ `Buffer_Copy`
   * **F6** ➔ `Buffer_Paste_InPlace`
   * **F7** ➔ `Buffer_Paste_Center`

<p align="center">
  <img src="images/Clipboard Paste_05.png" alt="Illustrator Actions Palette with assigned F5, F6, F7 keys" width="520">
</p>

---

### Method 2: Manual Hotkey Setup (Any Custom Key)

If `F5`–`F7` are already used by other workflows, custom keys (e.g. `F2`, `F3`, `Shift+F5`) can be assigned manually:

1. In the **Actions** panel menu, click **«New Action»**.
2. Name the action and select your desired function key.
3. In the Actions panel flyout menu, select **«Insert Menu Item...»**:

<p align="center">
  <img src="images/Clipboard Paste_06.png" alt="Insert Menu Item in Actions panel menu" width="480">
</p>

4. In the dialog, type the exact command name (e.g., `CP_copy`, `CP_place`, or `CP_paste`) and click **Find** ➔ **OK**:

<p align="center">
  <img src="images/Clipboard Paste_07.png" alt="Insert Menu Item dialog with CP_copy command" width="440">
</p>

5. Stop recording. The shortcut is now active and immediately usable.

---

## 🩺 Diagnostic Sentinel Tool Included

The repository includes the diagnostic monitor used to analyze the clipboard failure:
* **Directory:** [`tools/AI-ClipboardDoctor/`](tools/AI-ClipboardDoctor/)
* **Executable:** Pre-compiled Windows utility [`tools/AI-ClipboardDoctor/build/AI-ClipboardDoctor-Lite.exe`](tools/AI-ClipboardDoctor/build/AI-ClipboardDoctor-Lite.exe)
* **Capabilities:** Real-time clipboard auditing, vector format inspection, stuck lock reset (`Unlock Clipboard`), and test vector injection.

---

## 🌍 Multilingual Support (8 Languages)

The extension includes complete, built-in translations for:
* 🇬🇧 English • 🇷🇺 Русский • 🇩🇪 Deutsch • 🇪🇸 Español • 🇫🇷 Français • 🇵🇹 Português • 🇨🇳 中文 • 🇯🇵 日本語

Switching the language in the main panel instantly and synchronously updates all open micro-panels (`CP_copy`, `CP_place`, `CP_paste`) in real time.

---

## 📦 Installation

### Automatic Installation (Windows — Recommended)
1. Download the latest release: [`Clipboard_Paste_v1.0.12.zip`](https://github.com/SaidAuita/AI-Clipboard-Paste/releases/latest).
2. Close Adobe Illustrator.
3. Run `Install.bat` with a double-click.
   * Enables Adobe CEP `PlayerDebugMode` in the Windows Registry;
   * Clears stale CEP caches;
   * Safely installs files to `%APPDATA%\Adobe\CEP\extensions\Clipboard Paste\`.
4. Launch Illustrator and navigate to:  
   `Window` ➔ `Extensions` ➔ **`Clipboard Paste`** (or micro-panels `CP_copy`, `CP_place`, `CP_paste`).

### Manual Installation (Windows & macOS)
1. Copy the extension folder into the Adobe CEP extensions directory:
   * **Windows:** `%APPDATA%\Adobe\CEP\extensions\Clipboard Paste\`
   * **macOS:** `~/Library/Application Support/Adobe/CEP/extensions/Clipboard Paste/`
2. Enable `PlayerDebugMode`:
   * **Windows:** Run `enable_player_debug_mode.reg`.
   * **macOS:** Run in Terminal:
     ```bash
     defaults write com.adobe.CSXS.11 PlayerDebugMode 1
     defaults write com.adobe.CSXS.12 PlayerDebugMode 1
     defaults write com.adobe.CSXS.13 PlayerDebugMode 1
     defaults write com.adobe.CSXS.14 PlayerDebugMode 1
     ```
3. Restart Illustrator.

### Uninstallation
Run `Uninstall.bat` to cleanly remove the extension from Adobe CEP without affecting user actions or Illustrator preferences.

---

## 🛠️ Architecture & Developer Notes

* **Shared ExtendScript Engine:** All panels connect through `#targetengine "CP_SHARED"`. Data copied in `CP_copy` is instantly visible to `CP_place`, `CP_paste`, and the main panel without inter-process lag.
* **Triple-Redundant Session Buffer:**  
  1. `$.global.ClipboardPasteHost`
  2. Backup in `app.__CP_BUFFER__`
  3. Structured geometry descriptors (coordinates, artboard bounds, layer hierarchy).
* **Hot Reload:** Click the version badge (`v1.0.12`) in the main panel header or right-click any micro-panel to reload UI and script assets on the fly.

---

## 🛠️ Other Projects

* **[AI Code Pro](https://github.com/SaidAuita/AI-Code-Pro)** — Professional Barcode & QR Code generator for Adobe Illustrator with live verification and batch layout.
* **[ID Code Pro](https://github.com/SaidAuita/ID-Code-Pro)** — Barcode & QR Code generator extension and automation script for Adobe InDesign.
* **[AI Dimension](https://github.com/SaidAuita/AI-Dimension)** — Automatic dimensioning, gap measuring, and technical callout generator for Adobe Illustrator.
* **[ID Dimension](https://github.com/SaidAuita/ID-Dimension)** — Technical dimensioning and drafting annotation tool for Adobe InDesign.
* **[PH-CU-S](https://github.com/SaidAuita/ComfyUI_PH-CU-S)** — Local generative AI Photoshop plugin powered by ComfyUI (no subscriptions, 100% offline).
* **[ph-cu-s.com/tools](https://ph-cu-s.com/tools)** — Complete collection of prepress automation scripts and plugins.

---

## 📄 License & Author

* **Author:** Said Auita ([GitHub: @SaidAuita](https://github.com/SaidAuita))
* **Project Repository:** [github.com/SaidAuita/AI-Clipboard-Paste](https://github.com/SaidAuita/AI-Clipboard-Paste)
* **License:** Distributed under the [MIT License](LICENSE).
