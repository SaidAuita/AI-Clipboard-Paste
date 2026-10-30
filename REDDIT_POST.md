# Reddit Post: AI-Clipboard-Paste

> **Subreddits to post to:**
> - r/AdobeIllustrator (Primary)
> - r/graphic_design
> - r/freesoftware / r/opensource

---

## 📌 Title Options (Choose one)

**Option 1 (Direct & Relatable — Recommended):**
> I made a free native plugin to fix Illustrator’s silent clipboard bug (when Ctrl+C stops copying vectors until you restart)

**Option 2 (Feature-focused):**
> Tired of Illustrator's clipboard randomly dying mid-project? Made a lightweight C++ plugin that bypasses it (F5/F6/F7 out of the box, open source)

**Option 3 (Short & Punchy):**
> Fixed Illustrator's broken clipboard issue without needing a restart — free native C++ plugin (.aip)

---

## 📝 Post Content (Markdown for Reddit)

Hey everyone,

If you work in Illustrator on Windows for long sessions, you’ve probably hit this infuriating bug at least once:

You’re 2–3 hours into a heavy layout. You select an object, hit `Ctrl+C`, switch artboards or open another file, press `Ctrl+V` (or `Ctrl+F`)... and **literally nothing happens**. Or worse, it pastes something you copied an hour ago.

You try creating a clean new file (`Ctrl+N`), draw a simple rectangle, hit `Ctrl+C` — still dead. The clipboard is completely broken for vectors, and the only "solution" recommended on the Adobe forums is to save, close Illustrator, restart it, and lose your entire undo history.

### What actually happens behind the scenes
A while ago I wrote a low-level diagnostic tool (`AI-ClipboardDoctor`) to figure out what Illustrator is doing. Turns out Windows isn't the problem — the clipboard isn't locked by Chrome or any other app. What happens is that Illustrator’s internal vector serializer crashes silently during `Ctrl+C`. It empties the Windows clipboard (`EmptyClipboard`), fails to write the AICB/PDF vector payload, and closes the clipboard with **0 formats and 0 bytes**. Once that internal module crashes, Illustrator won't copy vectors again until the process is restarted.

### The Fix: In-Memory Duplication (Native C++ Plugin)
Instead of dealing with the broken OS clipboard, I wrote a small, native C++ plugin (**`ClipboardPaste.aip`**) that completely bypasses the Windows clipboard:

* **Direct RAM duplication:** When you copy, it creates an in-memory clone of your art directly inside Illustrator (`DuplicateArt`). It never touches the Windows OLE clipboard, so it literally cannot get blocked or fail.
* **Global Hotkeys out of the box:**
  * **`F5`** — Copy selection to buffer
  * **`F6`** — Paste in place (exact artboard coordinates)
  * **`F7`** — Paste into the center of your current screen view
  * *Hotkeys work immediately upon opening Illustrator.* No need to import `.aia` action sets, and no need to click on the panel first to give it focus.
* **No flashing helper windows:** Some of you might remember earlier CEP/HTML panel attempts at this — they were clunky because CEP had to flash auxiliary HTML micro-windows to catch keystrokes. This native plugin runs silently inside Illustrator's thread without any window popping up or screen blinking.
* **100% Vector Fidelity:** Keeps Spot / Pantone swatches, CMYK values, overprints, blend modes, compound paths, and clipping masks intact.
* **Native Spectrum UI:** The panel matches Illustrator’s standard dockable palettes (Properties panel style with 3px rounded buttons) and automatically redraws if you switch between Dark and Light UI brightness. There's also a **Settings...** dialog to remap keys to whatever you prefer.
* *(Note: We also kept the classic CEP panel in the repo for anyone who specifically needs the manual layer routing tab, but for everyday work, the native C++ plugin is 10x smoother).*

### Compatibility & Download
* **Compatibility:** Adobe Illustrator CC 2020 through 2026+ (Windows x64).
* **Cost:** 100% Free & Open Source (MIT License).
* **Source & Releases:** [GitHub Repository](https://github.com/SaidAuita/AI-Clipboard-Paste)
* **Direct zip download & Info:** [PH-CU-S Tools](https://ph-cu-s.com/tools/ai-clipboard-paste)

### Installation
1. Close Illustrator.
2. Download the `Clipboard_Paste_Plugin_v1.14.zip` from GitHub Releases or the website.
3. Run `Install_Plugin.bat` (it auto-detects all installed Illustrator versions and drops the `.aip` into the `Plug-ins` folder).
4. Launch Illustrator — you'll find it under **Window ➔ Clipboard Paste**, and `F5` / `F6` / `F7` will just work.

The full C++17 source code and MSVC build scripts are in the `native-plugin/` folder of the repo if anyone wants to inspect the code or compile it themselves.

Hope this saves a few of you from losing your momentum (and undo history) next time Illustrator decides to stop copying! Feedback and PRs are welcome.
