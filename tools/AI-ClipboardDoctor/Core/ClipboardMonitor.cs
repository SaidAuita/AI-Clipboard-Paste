using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using AI_ClipboardDoctor.Models;

namespace AI_ClipboardDoctor.Core
{
    public class ClipboardMonitor : NativeWindow, IDisposable
    {
        private bool _isListening;
        private bool _isDisposed;

        public event Action<ClipboardSnapshot>? ClipboardChanged;

        public ClipboardMonitor()
        {
            CreateHandle(new CreateParams());
            StartListening();
        }

        public void StartListening()
        {
            if (!_isListening && Handle != IntPtr.Zero)
            {
                _isListening = NativeMethods.AddClipboardFormatListener(Handle);
            }
        }

        public void StopListening()
        {
            if (_isListening && Handle != IntPtr.Zero)
            {
                NativeMethods.RemoveClipboardFormatListener(Handle);
                _isListening = false;
            }
        }

        protected override void WndProc(ref Message m)
        {
            if (m.Msg == NativeMethods.WM_CLIPBOARDUPDATE)
            {
                try
                {
                    var snapshot = CaptureSnapshot();
                    ClipboardChanged?.Invoke(snapshot);
                }
                catch (Exception ex)
                {
                    Debug.WriteLine("ClipboardChanged error: " + ex.Message);
                }
            }

            base.WndProc(ref m);
        }

        public ClipboardSnapshot CaptureSnapshot()
        {
            var snapshot = new ClipboardSnapshot
            {
                Timestamp = DateTime.Now,
                SequenceNumber = NativeMethods.GetClipboardSequenceNumber(),
                OwnerHwnd = NativeMethods.GetClipboardOwner(),
                OpenHwnd = NativeMethods.GetOpenClipboardWindow()
            };

            // Owner process
            if (snapshot.OwnerHwnd != IntPtr.Zero)
            {
                NativeMethods.GetWindowThreadProcessId(snapshot.OwnerHwnd, out uint ownerPid);
                snapshot.OwnerPid = (int)ownerPid;
                try
                {
                    using var p = Process.GetProcessById((int)ownerPid);
                    snapshot.OwnerProcessName = p.ProcessName;
                }
                catch
                {
                    snapshot.OwnerProcessName = "Process #" + ownerPid;
                }
            }

            // Open/Locking process
            if (snapshot.OpenHwnd != IntPtr.Zero)
            {
                NativeMethods.GetWindowThreadProcessId(snapshot.OpenHwnd, out uint openPid);
                snapshot.OpenPid = (int)openPid;
                try
                {
                    using var p = Process.GetProcessById((int)openPid);
                    snapshot.OpenProcessName = p.ProcessName;
                }
                catch
                {
                    snapshot.OpenProcessName = "Process #" + openPid;
                }
            }

            // Attempt to open clipboard with retries
            bool opened = false;
            for (int retry = 0; retry < 5; retry++)
            {
                if (NativeMethods.OpenClipboard(Handle))
                {
                    opened = true;
                    break;
                }
                Thread.Sleep(10);
            }

            if (opened)
            {
                try
                {
                    uint format = 0;
                    while ((format = NativeMethods.EnumClipboardFormats(format)) != 0)
                    {
                        var item = new ClipboardFormatItem { FormatId = format };

                        if (format < 0xC000)
                        {
                            item.Name = NativeMethods.GetStandardFormatName(format);
                        }
                        else
                        {
                            var sb = new StringBuilder(256);
                            if (NativeMethods.GetClipboardFormatName(format, sb, 256) > 0)
                            {
                                item.Name = sb.ToString();
                            }
                            else
                            {
                                item.Name = $"Custom #{format}";
                            }
                        }

                        item.IsVector = NativeMethods.IsVectorFormatName(item.Name);
                        item.IsAdobe = item.Name.Contains("Adobe", StringComparison.OrdinalIgnoreCase) ||
                                       item.Name.Contains("Illustrator", StringComparison.OrdinalIgnoreCase) ||
                                       item.Name.Equals(NativeMethods.FORMAT_AICB, StringComparison.OrdinalIgnoreCase);

                        // Non-invasive monitoring: do not invoke delayed rendering or call GlobalSize
                        // on unmanaged GDI/OLE handles which corrupts heap in Windows 11.
                        item.SizeBytes = -1;

                        snapshot.Formats.Add(item);
                    }
                }
                catch (Exception ex)
                {
                    Debug.WriteLine("Error enumerating formats: " + ex.Message);
                }
                finally
                {
                    NativeMethods.CloseClipboard();
                }
            }
            else
            {
                // Clipboard could not be opened - locked by another process!
                if (snapshot.OpenPid == 0)
                {
                    IntPtr openHwnd = NativeMethods.GetOpenClipboardWindow();
                    if (openHwnd != IntPtr.Zero)
                    {
                        NativeMethods.GetWindowThreadProcessId(openHwnd, out uint openPid);
                        snapshot.OpenPid = (int)openPid;
                        try
                        {
                            using var p = Process.GetProcessById((int)openPid);
                            snapshot.OpenProcessName = p.ProcessName;
                        }
                        catch
                        {
                            snapshot.OpenProcessName = "Process #" + openPid;
                        }
                    }
                }
            }

            return snapshot;
        }

        public void Dispose()
        {
            if (!_isDisposed)
            {
                StopListening();
                DestroyHandle();
                _isDisposed = true;
            }
        }
    }
}
