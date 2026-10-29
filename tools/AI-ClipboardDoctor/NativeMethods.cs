using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace AI_ClipboardDoctor
{
    internal static class NativeMethods
    {
        // --- Windows Messages ---
        public const int WM_CLIPBOARDUPDATE = 0x031D;
        public const int WM_KEYDOWN = 0x0100;
        public const int WM_KEYUP = 0x0101;
        public const int WM_SYSKEYDOWN = 0x0104;
        public const int WM_SYSKEYUP = 0x0105;

        // --- Hook Types ---
        public const int WH_KEYBOARD_LL = 13;

        // --- Virtual Keys ---
        public const int VK_C = 0x43;
        public const int VK_X = 0x58;
        public const int VK_V = 0x56;
        public const int VK_LCONTROL = 0xA2;
        public const int VK_RCONTROL = 0xA3;
        public const int VK_CONTROL = 0x11;

        // --- GUI Resource Flags ---
        public const uint GR_GDIOBJECTS = 0;
        public const uint GR_USEROBJECTS = 1;

        // --- GlobalAlloc Flags ---
        public const uint GMEM_MOVEABLE = 0x0002;
        public const uint GMEM_ZEROINIT = 0x0040;
        public const uint GHND = GMEM_MOVEABLE | GMEM_ZEROINIT;

        // --- Standard Clipboard Formats ---
        public const uint CF_TEXT = 1;
        public const uint CF_BITMAP = 2;
        public const uint CF_METAFILEPICT = 3;
        public const uint CF_SYLK = 4;
        public const uint CF_DIF = 5;
        public const uint CF_TIFF = 6;
        public const uint CF_OEMTEXT = 7;
        public const uint CF_DIB = 8;
        public const uint CF_PALETTE = 9;
        public const uint CF_PENDATA = 10;
        public const uint CF_RIFF = 11;
        public const uint CF_WAVE = 12;
        public const uint CF_UNICODETEXT = 13;
        public const uint CF_ENHMETAFILE = 14;
        public const uint CF_HDROP = 15;
        public const uint CF_LOCALE = 16;
        public const uint CF_DIBV5 = 17;

        // --- Known Adobe & Vector Format Names ---
        public const string FORMAT_AI_PRIVATE = "Adobe Illustrator Private Format";
        public const string FORMAT_AICB = "AICB";
        public const string FORMAT_PDF = "Portable Document Format";
        public const string FORMAT_PDF_SHORT = "PDF";
        public const string FORMAT_SVG = "image/svg+xml";
        public const string FORMAT_SVG_XML = "SVG / XML";
        public const string FORMAT_SVG_TEXT = "image/x-inkscape-svg";

        // --- Delegates ---
        public delegate IntPtr LowLevelKeyboardProc(int nCode, IntPtr wParam, IntPtr lParam);

        [StructLayout(LayoutKind.Sequential)]
        public struct KBDLLHOOKSTRUCT
        {
            public uint vkCode;
            public uint scanCode;
            public uint flags;
            public uint time;
            public UIntPtr dwExtraInfo;
        }

        // --- Clipboard API ---
        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool AddClipboardFormatListener(IntPtr hwnd);

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool RemoveClipboardFormatListener(IntPtr hwnd);

        [DllImport("user32.dll")]
        public static extern uint GetClipboardSequenceNumber();

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool OpenClipboard(IntPtr hWndNewOwner);

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool CloseClipboard();

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool EmptyClipboard();

        [DllImport("user32.dll")]
        public static extern uint EnumClipboardFormats(uint format);

        [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        public static extern int GetClipboardFormatName(uint format, [Out] StringBuilder lpszFormatName, int cchMaxCount);

        [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        public static extern uint RegisterClipboardFormat(string lpszFormat);

        [DllImport("user32.dll")]
        public static extern IntPtr GetClipboardData(uint uFormat);

        [DllImport("user32.dll")]
        public static extern IntPtr SetClipboardData(uint uFormat, IntPtr hMem);

        [DllImport("user32.dll")]
        public static extern IntPtr GetClipboardOwner();

        [DllImport("user32.dll")]
        public static extern IntPtr GetOpenClipboardWindow();

        // --- OLE Clipboard API ---
        [DllImport("ole32.dll")]
        public static extern int OleSetClipboard(IntPtr pDataObj);

        [DllImport("ole32.dll")]
        public static extern int OleFlushClipboard();

        // --- Global Memory API ---
        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern IntPtr GlobalAlloc(uint uFlags, UIntPtr dwBytes);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern IntPtr GlobalLock(IntPtr hMem);

        [DllImport("kernel32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool GlobalUnlock(IntPtr hMem);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern UIntPtr GlobalSize(IntPtr hMem);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern IntPtr GlobalFree(IntPtr hMem);

        // --- Window & Process API ---
        [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        public static extern IntPtr FindWindow(string? lpClassName, string? lpWindowName);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool SetForegroundWindow(IntPtr hWnd);

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

        public const int SW_RESTORE = 9;

        [DllImport("user32.dll")]
        public static extern IntPtr GetForegroundWindow();

        [DllImport("user32.dll", SetLastError = true)]
        public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

        [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

        [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

        [DllImport("user32.dll")]
        public static extern short GetAsyncKeyState(int vKey);

        [DllImport("user32.dll", SetLastError = true)]
        public static extern IntPtr SetWindowsHookEx(int idHook, LowLevelKeyboardProc lpfn, IntPtr hMod, uint dwThreadId);

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool UnhookWindowsHookEx(IntPtr hhk);

        [DllImport("user32.dll")]
        public static extern IntPtr CallNextHookEx(IntPtr hhk, int nCode, IntPtr wParam, IntPtr lParam);

        [DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        public static extern IntPtr GetModuleHandle(string? lpModuleName);

        // --- GUI Resources API ---
        [DllImport("user32.dll")]
        public static extern uint GetGuiResources(IntPtr hProcess, uint uiFlags);

        // --- Helper Methods ---
        public static string GetStandardFormatName(uint format)
        {
            return format switch
            {
                CF_TEXT => "CF_TEXT",
                CF_BITMAP => "CF_BITMAP",
                CF_METAFILEPICT => "CF_METAFILEPICT",
                CF_SYLK => "CF_SYLK",
                CF_DIF => "CF_DIF",
                CF_TIFF => "CF_TIFF",
                CF_OEMTEXT => "CF_OEMTEXT",
                CF_DIB => "CF_DIB",
                CF_PALETTE => "CF_PALETTE",
                CF_PENDATA => "CF_PENDATA",
                CF_RIFF => "CF_RIFF",
                CF_WAVE => "CF_WAVE",
                CF_UNICODETEXT => "CF_UNICODETEXT",
                CF_ENHMETAFILE => "CF_ENHMETAFILE",
                CF_HDROP => "CF_HDROP",
                CF_LOCALE => "CF_LOCALE",
                CF_DIBV5 => "CF_DIBV5",
                _ => $"Format #{format}"
            };
        }

        public static bool IsVectorFormatName(string name)
        {
            if (string.IsNullOrEmpty(name)) return false;
            return name.Contains("Illustrator", StringComparison.OrdinalIgnoreCase) ||
                   name.Equals(FORMAT_AICB, StringComparison.OrdinalIgnoreCase) ||
                   name.Contains("PDF", StringComparison.OrdinalIgnoreCase) ||
                   name.Contains("SVG", StringComparison.OrdinalIgnoreCase) ||
                   name.Equals("CF_ENHMETAFILE", StringComparison.OrdinalIgnoreCase) ||
                   name.Contains("PostScript", StringComparison.OrdinalIgnoreCase);
        }
    }
}
