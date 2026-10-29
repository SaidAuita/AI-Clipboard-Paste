using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace AI_ClipboardDoctor.Core
{
    public static class ClipboardInjector
    {
        private const string TEST_SVG = 
@"<svg xmlns=""http://www.w3.org/2000/svg"" width=""150"" height=""150"" viewBox=""0 0 100 100"">
  <rect x=""5"" y=""5"" width=""90"" height=""90"" rx=""15"" fill=""#10141D"" stroke=""#00D26A"" stroke-width=""3""/>
  <polygon points=""50,15 61,38 86,38 66,54 74,78 50,63 26,78 34,54 14,38 39,38"" fill=""#00D26A""/>
  <text x=""50"" y=""92"" font-family=""Arial"" font-size=""8"" fill=""#FFFFFF"" text-anchor=""middle"">DOCTOR TEST</text>
</svg>";

        private const string TEST_AICB = 
@"%!PS-Adobe-3.0
%%Creator: AI-ClipboardDoctor
%%Title: Test Vector Object
%%BoundingBox: 0 0 150 150
%%EndComments
%%BeginProlog
%%EndProlog
gsave
0 0 150 150 rectclip
newpath
50 15 moveto
61 38 lineto
86 38 lineto
66 54 lineto
74 78 lineto
50 63 lineto
26 78 lineto
34 54 lineto
14 38 lineto
39 38 lineto
closepath
0.8 0 0.8 0 setcmykcolor
fill
grestore
%%EOF
";

        public static bool InjectTestVector(IntPtr ownerHwnd, out string error)
        {
            error = string.Empty;

            bool opened = false;
            for (int i = 0; i < 10; i++)
            {
                if (NativeMethods.OpenClipboard(ownerHwnd))
                {
                    opened = true;
                    break;
                }
                Thread.Sleep(20);
            }

            if (!opened)
            {
                error = "Could not open clipboard (locked by another process).";
                return false;
            }

            try
            {
                if (!NativeMethods.EmptyClipboard())
                {
                    error = "Failed to empty clipboard.";
                    return false;
                }

                // 1. Text format
                string text = "[AI-ClipboardDoctor: Verified Vector Test Object. Press Ctrl+V in Illustrator to test paste]";
                SetClipboardText(NativeMethods.CF_UNICODETEXT, text);

                // 2. SVG format
                uint svgFormat = NativeMethods.RegisterClipboardFormat(NativeMethods.FORMAT_SVG);
                if (svgFormat != 0)
                {
                    SetClipboardAnsiText(svgFormat, TEST_SVG);
                }

                uint svgXmlFormat = NativeMethods.RegisterClipboardFormat(NativeMethods.FORMAT_SVG_XML);
                if (svgXmlFormat != 0)
                {
                    SetClipboardAnsiText(svgXmlFormat, TEST_SVG);
                }

                // 3. AICB format
                uint aicbFormat = NativeMethods.RegisterClipboardFormat(NativeMethods.FORMAT_AICB);
                if (aicbFormat != 0)
                {
                    SetClipboardAnsiText(aicbFormat, TEST_AICB);
                }

                return true;
            }
            catch (Exception ex)
            {
                error = ex.Message;
                return false;
            }
            finally
            {
                NativeMethods.CloseClipboard();
            }
        }

        public static bool EmergencyReset(IntPtr ownerHwnd, out string error)
        {
            error = string.Empty;

            try
            {
                // 1. Flush and release high-level Windows OLE clipboard object
                NativeMethods.OleSetClipboard(IntPtr.Zero);
                NativeMethods.OleFlushClipboard();
            }
            catch { }

            // 2. Clear low-level Win32 clipboard
            bool opened = false;
            for (int i = 0; i < 10; i++)
            {
                if (NativeMethods.OpenClipboard(ownerHwnd))
                {
                    opened = true;
                    break;
                }
                Thread.Sleep(20);
            }

            if (!opened)
            {
                error = "Could not open clipboard for reset.";
                return false;
            }

            try
            {
                NativeMethods.EmptyClipboard();
                return true;
            }
            catch (Exception ex)
            {
                error = ex.Message;
                return false;
            }
            finally
            {
                NativeMethods.CloseClipboard();
                try
                {
                    NativeMethods.OleFlushClipboard();
                }
                catch { }
            }
        }

        private static void SetClipboardText(uint format, string text)
        {
            byte[] bytes = Encoding.Unicode.GetBytes(text + "\0");
            IntPtr hMem = NativeMethods.GlobalAlloc(NativeMethods.GHND, (UIntPtr)bytes.Length);
            if (hMem != IntPtr.Zero)
            {
                IntPtr ptr = NativeMethods.GlobalLock(hMem);
                if (ptr != IntPtr.Zero)
                {
                    Marshal.Copy(bytes, 0, ptr, bytes.Length);
                    NativeMethods.GlobalUnlock(hMem);
                    NativeMethods.SetClipboardData(format, hMem);
                }
            }
        }

        private static void SetClipboardAnsiText(uint format, string text)
        {
            byte[] bytes = Encoding.UTF8.GetBytes(text + "\0");
            IntPtr hMem = NativeMethods.GlobalAlloc(NativeMethods.GHND, (UIntPtr)bytes.Length);
            if (hMem != IntPtr.Zero)
            {
                IntPtr ptr = NativeMethods.GlobalLock(hMem);
                if (ptr != IntPtr.Zero)
                {
                    Marshal.Copy(bytes, 0, ptr, bytes.Length);
                    NativeMethods.GlobalUnlock(hMem);
                    NativeMethods.SetClipboardData(format, hMem);
                }
            }
        }
    }
}
