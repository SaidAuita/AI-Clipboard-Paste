# Reddit Post: AI-Clipboard-Paste (Updated for v1.25)

> **Post URL:**  
> https://www.reddit.com/r/AdobeIllustrator/comments/1xjnpkj/fixed_illustrators_broken_clipboard_issue_without/
>
> **Subreddits:**
> - r/AdobeIllustrator
> - r/graphic_design
> - r/freesoftware / r/opensource

---

## 📝 Updated Post Content (Markdown for Reddit Editor)

Hey everyone,

A quick update on the free native C++ clipboard plugin (**Clipboard Paste Plugin**) I posted about earlier!

Thanks to community feedback and stress testing on heavy production files, I just pushed a major release — **v1.25** — which not only fixes a nasty edge-case crash on complex vector artwork, but also introduces a dedicated **Pattern Offset & Pixel Art engine** and single-key reflections that will save you an absurd amount of time.

---

### Quick Recap: What This Plugin Does
If you've ever had Illustrator on Windows randomly "forget" how to copy vectors after 2–3 hours of work (you hit `Ctrl+C`, but `Ctrl+V` pastes empty air or something from an hour ago until you restart Illustrator), this plugin fixes that at the root.

It duplicates artwork directly inside Illustrator's session RAM (`sAIArt->DuplicateArt`), completely bypassing the buggy Windows OLE clipboard. No screen flashing, no helper popups, and instant global hotkeys (`F5` Copy, `F6` Paste in Place, `F7` Paste Center of View, `F8` Paste on All Artboards).

---

### 🚀 What's New in v1.25:

#### 1. 🎯 Pattern Offset Clone & Move (Stop opening the Move dialog!)
If you build seamless patterns, geometric grids, isometric artwork, tessellations, or pixel-art vectors, you know the pain:  
To duplicate a tile or modular block edge-to-edge by its exact width or height, you traditionally had to open `Object ➔ Transform ➔ Move...` (`Shift+Ctrl+M`), check or calculate the dimensions in points/pixels, type values into Horizontal/Vertical inputs, click "Copy", and then hit `Ctrl+D`. Repeating this dozens of times ruins your flow.

**Now you can do it in real time right from your keyboard:**
* **`Ctrl + Numpad [1..9]`** — Instantly clones your selection edge-to-edge in any of the 8 directions (including the 4 diagonal corners: `7` NW, `9` NE, `1` SW, `3` SE).
* **`Alt + Numpad [1..9]`** — Shifts your selection by its exact bounding box dimensions without creating duplicates.
* **Draw vectors like a pixel brush:** Rapidly pressing or holding keys (e.g., `Ctrl+Num6`, `Ctrl+Num6`, `Ctrl+Num2`) lets you build seamless tile paths, brickwork, and isometric scenes in seconds — with raster-brush speed, but retaining 100% clean vector geometry!
* **Laptop Friendly:** On a laptop with no physical Numpad? Open **Settings...** to switch with 1-click to the top number row `1..9`, letter grids like `QWE / ASD / ZXC` or `UIO / JKL / M`, or customize each key individually.

#### 2. 🪞 Single-Key Symmetry (Reflect Vertical & Horizontal)
* **`F9`** — Reflect Vertical (flip left ⟷ right across the selection's geometric center).
* **`F10`** — Reflect Horizontal (flip top ⟷ bottom across the selection's geometric center).
* No need to select the Reflect tool `(O)`, set an anchor, or open the transform submenu. Combined with Offset Clone, you can generate symmetrical mandalas, borders, and ornaments in seconds.

#### 3. 🛡️ Critical Crash Fix for Complex Vector Artwork
* If you previously experienced a crash when pasting complex artwork (Compound Paths, Clipping Masks, or groups with appearance effects), this is now completely resolved.
* *Root cause:* Auto-selection flags were previously propagated recursively down to individual sub-paths within compound shapes, confusing Illustrator’s internal DOM and causing a crash in `StrokeOffset.aip` upon redraw.
* *Fix:* Selection is now applied strictly to the top-level container according to Adobe SDK rules, and the `CopyScope` lifecycle is safely isolated.

#### 4. 🎨 HiDPI / 4K UI Scaling & Dark/Light Themes
* The dockable palette features 3px rounded buttons matching Illustrator’s native Properties panel, fully scaling crisp typography on 4K/HiDPI monitors and adapting dynamically between Dark, Medium Dark, Medium Light, and Light themes.

---

### ⌨️ Global Hotkeys Summary (Fully Customizable in Settings):
| Hotkey | Action |
| :--- | :--- |
| **`F5`** | **Copy to RAM Buffer** (independent snapshot) |
| **`F6`** | **Paste in Place** (exact coordinates in active document) |
| **`F7`** | **Paste to Center of View** (centered on active viewport) |
| **`F8`** | **Paste on All Artboards** (duplicates to all artboards) |
| **`F9`** | **Reflect Vertical** (flip left/right) |
| **`F10`** | **Reflect Horizontal** (flip top/bottom) |
| **`Ctrl + [1..9]`** | **Pattern Offset Clone** (edge-to-edge duplicate) |
| **`Alt + [1..9]`** | **Pattern Offset Move** (edge-to-edge step shift) |

*(All hotkeys work immediately on process level without needing to click the palette first. If you ever need to disable them temporarily, there is an instant toggle checkbox in Settings).*

---

### 📦 Download & Installation:
* **Compatibility:** Adobe Illustrator CC 2020 through 2026+ (Windows x64).
* **Cost:** 100% Free & Open Source (MIT License).
* **Direct Zip Download:** [**Clipboard_Paste_Plugin_v1.25.zip**](https://ph-cu-s.com/static/tools/Clipboard_Paste_Plugin_v1.25.zip) (or via [Tools Page](https://ph-cu-s.com/tools/ai-clipboard-paste))
* **GitHub Repo & Source Code:** [https://github.com/SaidAuita/AI-Clipboard-Paste](https://github.com/SaidAuita/AI-Clipboard-Paste)

**How to Install:**
1. Close Illustrator.
2. Download and unzip `Clipboard_Paste_Plugin_v1.25.zip`.
3. Run `Install_Plugin.bat` (auto-detects Illustrator 2020..2026+ and copies `ClipboardPaste.aip`).
4. Launch Illustrator — palette will be waiting in **Window ➔ Clipboard Paste**, and all hotkeys will work right away.

Hope this helps you work faster and saves you from losing your undo history next time Illustrator's clipboard misbehaves! Let me know if you run into any issues or have ideas for future iterations.
