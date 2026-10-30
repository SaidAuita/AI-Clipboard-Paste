# AI-Clipboard-Paste 🚀

<p align="center">
  <b>Instant, rock-solid vector duplication in Adobe Illustrator — completely bypassing the broken Windows OLE clipboard.</b><br>
  <i>100% Vector Fidelity • In-Place Alignment • Global Hotkeys (F5 / F6 / F7) • 8 UI Languages</i>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Native%20Plugin-v1.14%20(Recommended)-brightgreen.svg?style=flat-square" alt="Native Plugin v1.14">
  <img src="https://img.shields.io/badge/CEP%20Extension-v1.0.12-blue.svg?style=flat-square" alt="CEP Version v1.0.12">
  <img src="https://img.shields.io/badge/Adobe%20Illustrator-2020--2026%2B-orange.svg?style=flat-square" alt="Illustrator Compatibility">
  <img src="https://img.shields.io/badge/Platform-Windows%20x64%20%7C%20macOS-lightgrey.svg?style=flat-square" alt="OS Support">
  <img src="https://img.shields.io/badge/License-MIT-green.svg?style=flat-square" alt="License">
</p>

<p align="center">
  *( 🇬🇧 <a href="#english"><b>English</b></a> | 🇷🇺 <a href="#русский"><b>Русский</b></a> )* &nbsp;•&nbsp; 
  <a href="https://github.com/SaidAuita/AI-Clipboard-Paste/releases/latest"><b>📦 Download Latest Release</b></a>
</p>

---

<a id="english"></a>
## 🇬🇧 English

### ⚡ The Core Problem: Illustrator Clipboard Failure

During long, intensive design and prepress sessions in Adobe Illustrator (especially on Windows 10/11), users frequently experience a catastrophic vector clipboard breakdown:

1. **Silent Failure:** You select vector artwork and press `Ctrl+C` (or *Edit ➔ Copy*). No error message appears.
2. **Nothing Pastes:** When you press `Ctrl+V` (or *Edit ➔ Paste in Place*), either nothing happens at all, or Illustrator pastes stale data that was copied hours ago.
3. **Persists in New Documents:** Pressing `Ctrl+C` even on a simple rectangle in a fresh document (`Ctrl+N`) fails.
4. **Productivity Loss:** The only workaround was forcefully restarting `Illustrator.exe`, causing loss of undo history and disrupting work.

---

### 🔬 Diagnostic Audit & Proof: How the Cause Was Discovered

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

#### Key Technical Findings:
* **GDI/User Handles are Healthy:** GDI count was 2,292 and User count was 4,545 (far below the 10,000 Windows limit). The failure is not caused by OS handle exhaustion.
* **No Third-Party Locking:** The clipboard was not locked or held by external apps (browsers, messengers, or clipboard managers). The clipboard owner was strictly `Illustrator.exe`.
* **Root Cause Located:** Upon pressing `Ctrl+C`, Illustrator calls Windows `OpenClipboard()` and `EmptyClipboard()`, advancing the OS sequence counter (`#5545 ➔ #5546`). However, during internal serialization of vector geometry into standard clipboard streams (`AICB`, `Adobe Illustrator Private Format`, `PDF`), an unhandled exception occurs inside Illustrator's internal serialization engine. Illustrator prematurely invokes `CloseClipboard()`, leaving the system clipboard **completely empty (0 formats, 0.0 KB)**.
* **Paste Engine is 100% Functional:** When synthetic `AICB` vector data was injected directly into the clipboard via `AI-ClipboardDoctor`, pressing `Ctrl+V` in Illustrator instantly and flawlessly pasted the test vector. 
* **Conclusion:** The issue is isolated strictly to **Illustrator's internal vector copy/serialization singleton**, which becomes corrupted and cannot recover without restarting the process.

*(The full audit report is available in [`tools/AI-ClipboardDoctor/illustrator_clipboard_failure_analysis.md`](tools/AI-ClipboardDoctor/illustrator_clipboard_failure_analysis.md)).*

---

### 💡 The Solution: AI-Clipboard-Paste

**AI-Clipboard-Paste** provides a permanent fix by **completely bypassing the Windows OLE clipboard and Illustrator's fragile internal serializer**:

* ⚡ **Direct In-Memory Duplication:** All copy and paste operations occur entirely within Illustrator's memory space via the native C++ SweetPea DOM engine or high-speed ExtendScript (`CP_SHARED`).
* 🎯 **100% Vector Fidelity:** Flawless preservation of CMYK/Spot swatches, overprints, transparency blending, complex compound paths, and clipping masks.
* 📍 **In-Place Precision:** Objects align exactly to their original `[x, y]` artboard coordinates or can be centered on the target view.
* 🚀 **5–10× Faster:** Eliminates the latency of serializing and deserializing heavy vector payloads through the operating system.
* 🛡️ **Zero Interference:** Never affected by external applications, clipboard managers, or browser activity.

---

### ⚡ Two Editions Available: Choose What Fits You

| Feature | ⚡ **Native C++ Plugin (`.aip`)** <br> **(RECOMMENDED)** | 🌐 **CEP Extension (HTML5/JS)** |
|:---|:---:|:---:|
| **Global Hotkeys (F5, F6, F7)** | ✅ **Works out of the box** (low-level Win32 thread hook) | ⚠️ Requires importing Illustrator `.aia` Actions |
| **No Extra Popups or Flashes** | ✅ **100% silent & seamless background execution** | ❌ Flashes/opens companion HTML micro-panels |
| **User Interface** | ✅ **Pixel-perfect Adobe Spectrum UI** (Dark & Light themes) | 🌐 Modern HTML5/CSS panel |
| **Shortcut Configuration** | ✅ Graphical **Settings...** dialog with key remapping | ⚙️ Configured via Illustrator Actions |
| **Direct Transfer by Layer** | ➖ Core fast duplication focus | ✅ Includes «1-Click Direct» layer transfer tab |
| **Target Platform** | Windows x64 (AI 2020–2026+) *(C++ source in repo)* | Windows & macOS |
| **Location in Repo** | [`native-plugin/`](native-plugin/) | Root / [`Install.bat`](Install.bat) |

---

### 🌟 1. Native C++ Plugin (.aip) — The Recommended Experience

The **Native C++ Plugin** is the primary, most streamlined way to use Clipboard Paste in Adobe Illustrator CC 2020 – 2026+:

<p align="center">
  <img src="images/Clipboard Paste Plugin_01.png" alt="Native C++ Dockable Panel in Illustrator Dark Theme" width="290">
  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
  <img src="images/Clipboard Paste Plugin_02.png" alt="Native C++ Hotkey & Preferences Dialog" width="350">
</p>

* **🔥 Hotkeys Work Instantly Out of the Box:** Press `F5` (Copy to Buffer), `F6` (Paste In Place), or `F7` (Paste to Center) at any time. Keystrokes are captured globally without needing to click the panel or switch focus!
* **🚫 Zero Extra Panels or Visual Flashes:** Unlike the CEP extension (which had to briefly spawn micro-panels to execute commands), the native plugin runs directly inside `Illustrator.exe` thread memory. No popups, no focus-stealing, no screen blinking.
* **🎨 100% Native Illustrator Theme Adaptation:** Fully custom-rendered dockable palette with 3px rounded buttons matching Illustrator's native Properties panel (`Edit Artboards`). Dynamically matches all 4 UI brightness themes (Dark, Medium Dark, Medium Light, Light).
* **⚙️ In-Panel Preferences Dialog:** Click **Settings...** to remap any function key (`F1`–`F12`, `Alt+...`, `Ctrl+...`) and toggle the *"Select object(s) after paste"* option.

#### Quick Native Plugin Installation:
1. Open the [`native-plugin/`](native-plugin/) folder (or download the precompiled zip from Releases).
2. Run **`Install_Plugin.bat`** (it automatically detects all installed Illustrator versions 2020..2026+ and copies `ClipboardPaste.aip`).
3. Launch Adobe Illustrator and open **Window ➔ Clipboard Paste**.

---

### 🌐 2. CEP Extension Edition (Alternative)

#### 1. Two-Step Independent Buffer (Tab: «2-Step Buffer»)
Classic clipboard workflow: copy artwork in Document A ➔ switch to any other document or layer ➔ paste in place or centered.

<p align="center">
  <img src="images/Clipboard Paste_01.png" alt="2-Step Buffer Tab" width="380">
</p>

#### 2. One-Click Direct Transfer (Tab: «1-Click Direct»)
Instant transfer without intermediate steps: directly move or copy selection to a chosen layer, create a new layer above, or send to another open document.

<p align="center">
  <img src="images/Clipboard Paste_00.png" alt="1-Click Direct Tab" width="380">
</p>

#### 3. Standalone Micro-Panels for Global Hotkeys (F5, F6, F7)
Three lightweight companion micro-panels featuring high-visibility action buttons and live status indicators. They can be docked in a narrow toolbar or triggered globally:

| `CP_copy` (F5) | `CP_place` (F6) | `CP_paste` (F7) |
|:---:|:---:|:---:|
| <img src="images/Clipboard Paste_02.png" width="280" alt="CP_copy F5"> | <img src="images/Clipboard Paste_03.png" width="280" alt="CP_place F6"> | <img src="images/Clipboard Paste_04.png" width="280" alt="CP_paste F7"> |
| **Copy to Buffer** | **Paste In Place** | **Paste In Center** |

---

### ⌨️ Configuring Global Hotkeys (F5 / F6 / F7)

Adobe CEP panels cannot capture global keystrokes when the panel is out of focus. This limitation is solved by registering three native Illustrator menu commands:
* `CP_copy`
* `CP_place`
* `CP_paste`

By pairing these commands with Illustrator **Actions**, you get true single-key global shortcuts that work from anywhere in your document!

#### Method 1: Load Pre-Built Actions (Fast & Recommended)
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

#### Method 2: Manual Hotkey Setup (Any Custom Key)

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

### 🩺 Diagnostic Sentinel Tool Included

The repository includes the diagnostic monitor used to analyze the clipboard failure:
* **Directory:** [`tools/AI-ClipboardDoctor/`](tools/AI-ClipboardDoctor/)
* **Executable:** Pre-compiled Windows utility [`tools/AI-ClipboardDoctor/build/AI-ClipboardDoctor-Lite.exe`](tools/AI-ClipboardDoctor/build/AI-ClipboardDoctor-Lite.exe)
* **Capabilities:** Real-time clipboard auditing, vector format inspection, stuck lock reset (`Unlock Clipboard`), and test vector injection.

---

### 🌍 Multilingual Support (8 Languages)

The extension includes complete, built-in translations for:
* 🇬🇧 English • 🇷🇺 Русский • 🇩🇪 Deutsch • 🇪🇸 Español • 🇫🇷 Français • 🇵🇹 Português • 🇨🇳 中文 • 🇯🇵 日本語

Switching the language in the main panel instantly and synchronously updates all open micro-panels (`CP_copy`, `CP_place`, `CP_paste`) in real time.

---

### 📦 Installation

#### Automatic Installation (Windows — Recommended)
1. Download the latest release: [`Clipboard_Paste_v1.0.12.zip`](https://github.com/SaidAuita/AI-Clipboard-Paste/releases/latest).
2. Close Adobe Illustrator.
3. Run `Install.bat` with a double-click.
   * Enables Adobe CEP `PlayerDebugMode` in the Windows Registry;
   * Clears stale CEP caches;
   * Safely installs files to `%APPDATA%\Adobe\CEP\extensions\Clipboard Paste\`.
4. Launch Illustrator and navigate to:  
   `Window` ➔ `Extensions` ➔ **`Clipboard Paste`** (or micro-panels `CP_copy`, `CP_place`, `CP_paste`).

#### Manual Installation (Windows & macOS)
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

#### Uninstallation
Run `Uninstall.bat` to cleanly remove the extension from Adobe CEP without affecting user actions or Illustrator preferences.

---

### 🛠️ Other Projects

**[AI Dimension](https://github.com/SaidAuita/AI-Dimension)**
* Automatic dimensioning, gap measuring, and technical callout generator for Adobe Illustrator.

**[ID Dimension](https://github.com/SaidAuita/ID-Dimension)**
* Technical dimensioning and drafting annotation tool for Adobe InDesign (includes both CEP panel and ScriptUI palette).

**[ComfyUI Photoshop Plugin (PH-CU-S)](https://github.com/SaidAuita/ComfyUI_PH-CU-S)**
* A powerful Photoshop plugin powered by ComfyUI, providing direct integration with local generative models without any clouds, subscriptions, or recurring fees.

### 💎 QR Code with Custom Shape Logo

<table>
  <tr>
    <td align="center" width="50%">
      <a href="https://ph-cu-s.com/ai-code-pro">
        <img src="images/ai_code_pro.png" width="130" alt="AI Code PRO - Custom Shape Logo QR Code" /><br/>
        <b>AI Code PRO</b>
      </a><br/>
      <sub>Vector Barcodes (18 Standards) & Designer QR Codes for <b>Adobe Illustrator</b></sub><br/><br/>
      <a href="https://ph-cu-s.com/ai-code-pro"><img src="https://img.shields.io/badge/PRO-Illustrator%20Extension-orange.svg" alt="AI Code PRO" /></a>
    </td>
    <td align="center" width="50%">
      <a href="https://ph-cu-s.com/id-code-pro">
        <img src="images/id_code_pro.png" width="130" alt="ID Code PRO - Custom Shape Logo QR Code" /><br/>
        <b>ID Code PRO</b>
      </a><br/>
      <sub>Vector Barcodes (18 Standards) & Designer QR Codes for <b>Adobe InDesign</b></sub><br/><br/>
      <a href="https://ph-cu-s.com/id-code-pro"><img src="https://img.shields.io/badge/PRO-InDesign%20Extension-purple.svg" alt="ID Code PRO" /></a>
    </td>
  </tr>
</table>

---

<a id="русский"></a>
## 🇷🇺 Русский

### ⚡ Проблема: отвал буфера обмена в Illustrator при активной работе

При продолжительной и активной работе в Adobe Illustrator (особенно на Windows 10/11) пользователи регулярно сталкиваются с отказом буфера обмена:

1. **Тихий сбой:** Выделяется векторный объект, нажимается `Ctrl+C` (или *Редактирование ➔ Скопировать*). Никаких предупреждений или ошибок не появляется.
2. **Вставка не работает:** При нажатии `Ctrl+V` (или *Вставить на то же место*) ничего не происходит, либо вставляется старое содержимое, скопированное задолго до этого.
3. **Сбой сохраняется в новых макетах:** Нажатие `Ctrl+C` даже на обычном прямоугольнике в чистом новом документе (`Ctrl+N`) не работает.
4. **Потеря рабочего времени:** Единственным решением оставался принудительный перезапуск `Illustrator.exe` со сбросом истории отмен.

---

### 🔬 Диагностический аудит: как была выявлена причина сбоя

Для точной локализации проблемы была разработана низкоуровневая диагностическая утилита — **[AI-ClipboardDoctor](tools/AI-ClipboardDoctor/)**, отслеживающая системные события Windows API в реальном времени.

```text
================================================================================
  СОБЫТИЕ #5 — [CTRL+C] — INFO
  Время: 23:44:53.731 | Процесс: Illustrator (PID: 57224)
  GDI Handles: 2 292 | User Handles: 4 545
  Состояние: Ожидание публикации данных Windows Clipboard (Ревизия: #5545).
================================================================================

================================================================================
  СОБЫТИЕ #6 — [WARN: NO VECTORS] — WARNING
  Время: 23:44:53.802 | Задержка: 71 мс
  Сводка: ВНИМАНИЕ: Векторные форматы отсутствуют в буфере обмена!
  Владелец буфера: Illustrator (PID: 57224)
  Заблокирован:    НЕТ (Буфер свободен)
  Ревизия буфера:  Sequence #5546
  Всего форматов:  0
  Общий объём:     0.0 КБ
  Форматы:         [ПУСТО — 0 форматов]
================================================================================
```

#### Технические выводы аудита:
* **Дескрипторы GDI и User в норме:** 2 292 GDI и 4 545 User дескрипторов (значительно ниже лимита Windows в 10 000). Утечки дескрипторов ОС нет.
* **Блокировок буфера сторонними программами нет:** Буфер обмена не захвачен внешними приложениями (браузерами, мессенджерами или менеджерами буфера). Владельцем является исключительно `Illustrator.exe`.
* **Локализация сбоя:** При нажатии `Ctrl+C` Illustrator вызывает API-функции `OpenClipboard()` и `EmptyClipboard()`, инкрементируя системный счётчик буфера (`#5545 ➔ #5546`). Однако во время внутренней сериализации геометрии в стандартные векторные форматы обмена (`AICB`, `Adobe Illustrator Private Format`, `PDF`) внутри собственного сериализатора Illustrator происходит критический сбой. Illustrator аварийно вызывает `CloseClipboard()`, оставляя системный буфер **абсолютно пустым (0 форматов, 0.0 КБ)**.
* **Подсистема вставки (Paste Engine) исправна:** При инъекции тестового векторного потока `AICB` напрямую в буфер через `AI-ClipboardDoctor`, нажатие `Ctrl+V` в Illustrator мгновенно и безошибочно вставляет тестовый вектор.
* **Вывод:** Проблема изолирована строго во **внутреннем модуле сериализации копирования Illustrator** (процессный синглтон), который после сбоя не может восстановиться без перезапуска процесса.

*(Полный текст технического отчёта доступен в файле [`tools/AI-ClipboardDoctor/illustrator_clipboard_failure_analysis.md`](tools/AI-ClipboardDoctor/illustrator_clipboard_failure_analysis.md)).*

---

### 💡 Решение проблемы: AI-Clipboard-Paste

**AI-Clipboard-Paste** полностью исключает обращение к системному буферу обмена Windows и падающему внутреннему сериализатору Illustrator:

* ⚡ **Прямое дублирование в оперативной памяти:** Все операции выполняются напрямую в адресном пространстве процесса Illustrator через нативное ядро C++ SweetPea DOM или скоростной ExtendScript (`CP_SHARED`).
* 🎯 **100% Векторная точность:** Полное сохранение плашечных и триадных цветов (CMYK/Spot), оверпринтов, режимов наложения, сложных составных контуров и обтравочных масок.
* 📍 **Идеальное позиционирование (In Place):** Объекты вставляются точно в свои исходные координаты `[x, y]` артборда либо центрируются на видимой области.
* 🚀 **В 5–10 раз быстрее:** Отсутствуют задержки на конвертацию тяжёлых векторов через ОС.
* 🛡️ **Полная изоляция:** Буфер сессии защищён от любых внешних приложений и не сбрасывается при работе с текстом в браузере или мессенджере.

---

### ⚡ Два варианта работы: выберите удобный для вас

| Возможность | ⚡ **Нативный C++ плагин (`.aip`)** <br> **(РЕКОМЕНДУЕТСЯ)** | 🌐 **CEP расширение (HTML5/JS)** |
|:---|:---:|:---:|
| **Горячие клавиши (F5, F6, F7)** | ✅ **Работают сразу из коробки** (низкоуровневый Win32 хук) | ⚠️ Требуется загрузка набора экшенов `.aia` |
| **Никаких лишних/всплывающих окон** | ✅ **100% бесшумная работа в фоне**, экран не моргает | ❌ Открывает/моргает вспомогательными микро-панелями |
| **Интерфейс** | ✅ **Пиксель-в-пиксель нативный стиль Illustrator** (тёмная/светлая темы) | 🌐 Веб-панель HTML5/CSS |
| **Настройка горячих клавиш** | ✅ Встроенный диалог **Settings...** с переназначением клавиш | ⚙️ Настройка через стандартные Actions Illustrator |
| **Прямой перенос по слоям** | ➖ Фокус на мгновенном дублировании в памяти | ✅ Вкладка «1-Click Direct» для отправки на слои |
| **Целевая платформа** | Windows x64 (AI 2020–2026+) *(Исходники C++ в репозитории)* | Windows и macOS |
| **Где находится в репозитории** | Папка [`native-plugin/`](native-plugin/) | Корень / [`Install.bat`](Install.bat) |

---

### 🌟 1. Нативный C++ плагин (.aip) — Приоритетная версия

**Нативный C++ плагин** — это самый быстрый, чистый и комфортный способ работы в Adobe Illustrator CC 2020 – 2026+:

<p align="center">
  <img src="images/Clipboard Paste Plugin_01.png" alt="Нативная палитра Clipboard Paste в тёмной теме Illustrator" width="290">
  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
  <img src="images/Clipboard Paste Plugin_02.png" alt="Настройки горячих клавиш нативного плагина" width="350">
</p>

* **🔥 Горячие клавиши работают из коробки:** Нажимайте `F5` (копировать в буфер), `F6` (вставить на место) или `F7` (вставить в центр экрана) в любой момент. Плагин перехватывает нажатия глобально без необходимости кликать мышью по палитре или переключать фокус!
* **🚫 Никаких всплывающих микро-панелей и морганий:** В отличие от CEP-версии, которой для перехвата сочетаний клавиш требовалось на доли секунды открывать вспомогательные HTML-окна (`cp_copy`, `cp_place`), нативный плагин работает напрямую в адресном пространстве процесса `Illustrator.exe`. Никаких мелькающих окон на экране.
* **🎨 Нативное оформление и поддержка всех тем:** Палитра повторяет дизайн стандартных кнопок палитры *Свойства (Properties)* со скруглением 3px и автоматически перерисовывается под текущую тему Illustrator (тёмная, средне-тёмная, средне-светлая, светлая).
* **⚙️ Графический диалог настроек:** Кнопка **Settings...** открывает окно переназначения любых клавиш (`F1`–`F12`, `Alt+...`, `Ctrl+...`) и переключатель опции автоматического выделения объектов после вставки (*Select object(s) after paste*).

#### Быстрая установка нативного плагина:
1. Откройте папку [`native-plugin/`](native-plugin/) (или скачайте готовый архив из секции Releases).
2. Запустите **`Install_Plugin.bat`** (инсталлятор автоматически обнаружит все установленные версии Illustrator 2020..2026+ и скопирует файл `ClipboardPaste.aip`).
3. Запустите Adobe Illustrator — палитра ждёт вас в меню **Окно ➔ Clipboard Paste** (Window ➔ Clipboard Paste).

---

### 🌐 2. CEP расширение (HTML5 / JS) — Альтернативная версия

#### 1. Двухшаговый независимый буфер (Вкладка «2-Step Buffer»)
Классический сценарий: скопировать объект в одном документе ➔ переключиться в любой другой документ или слой ➔ вставить на то же место или по центру.

<p align="center">
  <img src="images/Clipboard Paste_01.png" alt="Вкладка 2-Step Buffer" width="380">
</p>

#### 2. Однокликовый прямой перенос (Вкладка «1-Click Direct»)
Мгновенное перемещение выделенных объектов без промежуточных шагов: напрямую на выбранный слой, на созданный новый слой выше (`+ New Layer`) или в другой открытый документ.

<p align="center">
  <img src="images/Clipboard Paste_00.png" alt="Вкладка 1-Click Direct" width="380">
</p>

#### 3. Автономные микро-панели под горячие клавиши (F5, F6, F7)
Три компактные панели с крупными кнопками и индикатором статуса, которые можно закрепить в узкую док-панель или привязать к глобальным клавишам:

| `CP_copy` (F5) | `CP_place` (F6) | `CP_paste` (F7) |
|:---:|:---:|:---:|
| <img src="images/Clipboard Paste_02.png" width="280" alt="CP_copy F5"> | <img src="images/Clipboard Paste_03.png" width="280" alt="CP_place F6"> | <img src="images/Clipboard Paste_04.png" width="280" alt="CP_paste F7"> |
| **Копировать в буфер** | **Вставить на то же место** | **Вставить по центру** |

---

### ⌨️ Настройка глобальных горячих клавиш (F5 / F6 / F7)

Сторонние расширения Adobe CEP не могут перехватывать системные нажатия клавиш без активного фокуса на панели. Проблема решена регистрацией трёх команд меню Illustrator:
* `CP_copy`
* `CP_place`
* `CP_paste`

Связав их с операциями (Actions), пользователь получает **настоящие глобальные горячие клавиши**, работающие из любого места макета!

#### Способ 1: Загрузка готового набора операций (Рекомендуется)
1. В Illustrator откройте палитру операций: `Окно` ➔ `Операции` (`Window` ➔ `Actions`).
2. В меню палитры выберите пункт **«Загрузить операции...»** (**Load Actions...**).
3. Выберите файл из каталога плагина:  
   [`actions/ClipboardPaste_Shortcuts.aia`](actions/ClipboardPaste_Shortcuts.aia)
4. В палитре появится готовая папка **Clipboard Paste** с назначенными клавишами:
   * **F5** ➔ `Buffer_Copy`
   * **F6** ➔ `Buffer_Paste_InPlace`
   * **F7** ➔ `Buffer_Paste_Center`

<p align="center">
  <img src="images/Clipboard Paste_05.png" alt="Палитра Actions с назначенными хоткеями F5, F6, F7" width="520">
</p>

---

#### Способ 2: Ручная настройка на любые сочетания клавиш

Если клавиши `F5`–`F7` уже заняты другими процессами, можно назначить любые другие клавиши (например, `F2`, `F3`, `Shift+F5`):

1. В меню палитры **Actions** нажмите **«Новая операция»** (**New Action**).
2. Задайте имя операции и выберите желаемую функциональную клавишу.
3. В правом меню палитры Actions выберите пункт **«Вставить пункт меню...»** (**Insert Menu Item...**):

<p align="center">
  <img src="images/Clipboard Paste_06.png" alt="Выбор Insert Menu Item в палитре Actions" width="480">
</p>

4. В открывшемся диалоге введите точное имя команды (например, `CP_copy`, `CP_place` или `CP_paste`) и нажмите **Find** ➔ **OK**:

<p align="center">
  <img src="images/Clipboard Paste_07.png" alt="Диалог Insert Menu Item с командой CP_copy" width="440">
</p>

5. Остановите запись операции. Теперь операция будет мгновенно срабатывать по нажатию назначенной клавиши.

---

### 🩺 Диагностическая утилита в комплекте

В репозиторий включена диагностическая программа, использованная при аудите сбоя:
* **Каталог:** [`tools/AI-ClipboardDoctor/`](tools/AI-ClipboardDoctor/)
* **Исполняемый файл:** Готовая Windows-утилита [`tools/AI-ClipboardDoctor/build/AI-ClipboardDoctor-Lite.exe`](tools/AI-ClipboardDoctor/build/AI-ClipboardDoctor-Lite.exe)
* **Возможности:** Мониторинг форматов буфера в реальном времени, фиксация сбоев Illustrator, сброс зависших блокировок (`Unlock Clipboard`) и тест инъекции векторов.

---

### 🌍 Мультиязычность (8 языков)

Расширение содержит встроенную локализацию на 8 языков:
* 🇷🇺 Русский • 🇬🇧 English • 🇩🇪 Deutsch • 🇪🇸 Español • 🇫🇷 Français • 🇵🇹 Português • 🇨🇳 中文 • 🇯🇵 日本語

При выборе языка в главной панели все открытые микро-панели (`CP_copy`, `CP_place`, `CP_paste`) **мгновенно и синхронно обновляют перевод** в реальном времени.

---

### 📦 Установка

#### Автоматическая установка (Windows — Рекомендуется)
1. Скачайте свежий релиз: [`Clipboard_Paste_v1.0.12.zip`](https://github.com/SaidAuita/AI-Clipboard-Paste/releases/latest).
2. Закройте Adobe Illustrator.
3. Запустите файл `Install.bat` двойным кликом.
   * Скрипт пропишет режим отладки Adobe CEP (`PlayerDebugMode`) в системном реестре Windows;
   * Очистит устаревший кэш CEP;
   * Безопасно скопирует расширение в `%APPDATA%\Adobe\CEP\extensions\Clipboard Paste\`.
4. Запустите Illustrator и откройте панель:  
   `Окно` ➔ `Расширения` ➔ **`Clipboard Paste`** (или микро-панели `CP_copy`, `CP_place`, `CP_paste`).

#### Ручная установка (macOS & Windows)
1. Скопируйте папку расширения в системный каталог CEP:
   * **Windows:** `%APPDATA%\Adobe\CEP\extensions\Clipboard Paste\`
   * **macOS:** `~/Library/Application Support/Adobe/CEP/extensions/Clipboard Paste/`
2. Включите режим отладки `PlayerDebugMode`:
   * **Windows:** запустите `enable_player_debug_mode.reg`.
   * **macOS:** выполните в Терминале:
     ```bash
     defaults write com.adobe.CSXS.11 PlayerDebugMode 1
     defaults write com.adobe.CSXS.12 PlayerDebugMode 1
     defaults write com.adobe.CSXS.13 PlayerDebugMode 1
     defaults write com.adobe.CSXS.14 PlayerDebugMode 1
     ```
3. Перезапустите Illustrator.

#### Удаление
Для полного удаления запустите `Uninstall.bat`. Настройки и экшены Illustrator не будут затронуты.

---

### 🛠️ Другие проекты (My Projects)

**[AI Dimension](https://github.com/SaidAuita/AI-Dimension)**
* Автоматическая простановка технических размеров, зазоров и выносных линий для Adobe Illustrator.

**[ID Dimension](https://github.com/SaidAuita/ID-Dimension)**
* Инструмент простановки чертежных размеров и аннотаций для Adobe InDesign (включает панель CEP и скрипт ScriptUI).

**[ComfyUI Photoshop Plugin (PH-CU-S)](https://github.com/SaidAuita/ComfyUI_PH-CU-S)**
* Мощный плагин для Photoshop на базе ComfyUI, обеспечивающий прямую интеграцию с локальными генеративными моделями без подписок и облаков.

### 💎 QR код с логотипом произвольной формы

<table>
  <tr>
    <td align="center" width="50%">
      <a href="https://ph-cu-s.com/ru/ai-code-pro">
        <img src="images/ai_code_pro_ru.png" width="130" alt="AI Code PRO - QR код с логотипом произвольной формы" /><br/>
        <b>AI Code PRO</b>
      </a><br/>
      <sub>Генератор векторных штрихкодов (18 стандартов) и дизайнерских QR-кодов для <b>Adobe Illustrator</b></sub><br/><br/>
      <a href="https://ph-cu-s.com/ru/ai-code-pro"><img src="https://img.shields.io/badge/PRO-Расширение%20Illustrator-orange.svg" alt="AI Code PRO" /></a>
    </td>
    <td align="center" width="50%">
      <a href="https://ph-cu-s.com/ru/id-code-pro">
        <img src="images/id_code_pro_ru.png" width="130" alt="ID Code PRO - QR код с логотипом произвольной формы" /><br/>
        <b>ID Code PRO</b>
      </a><br/>
      <sub>Генератор векторных штрихкодов (18 стандартов) и дизайнерских QR-кодов для <b>Adobe InDesign</b></sub><br/><br/>
      <a href="https://ph-cu-s.com/ru/id-code-pro"><img src="https://img.shields.io/badge/PRO-Расширение%20InDesign-purple.svg" alt="ID Code PRO" /></a>
    </td>
  </tr>
</table>

---

## 📄 License & Author

* **Author:** Said Auita ([GitHub: @SaidAuita](https://github.com/SaidAuita))
* **Project Repository:** [github.com/SaidAuita/AI-Clipboard-Paste](https://github.com/SaidAuita/AI-Clipboard-Paste)
* **Prepress Automation Hub:** [ph-cu-s.com/tools](https://ph-cu-s.com/tools)
* **License:** Distributed under the [MIT License](LICENSE).
