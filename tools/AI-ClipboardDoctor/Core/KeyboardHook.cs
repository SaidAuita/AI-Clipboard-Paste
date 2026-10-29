using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace AI_ClipboardDoctor.Core
{
    public class KeyboardHook : IDisposable
    {
        private IntPtr _hookId = IntPtr.Zero;
        private readonly NativeMethods.LowLevelKeyboardProc _proc;
        private bool _isDisposed;

        public event Action<bool>? CopyTriggeredInIllustrator; // true if Cut (Ctrl+X), false if Copy (Ctrl+C)
        public event Action? PasteTriggeredInIllustrator;

        public KeyboardHook()
        {
            _proc = HookCallback;
            InstallHook();
        }

        private void InstallHook()
        {
            using var curProcess = Process.GetCurrentProcess();
            using var curModule = curProcess.MainModule;
            IntPtr hMod = NativeMethods.GetModuleHandle(curModule?.ModuleName);
            _hookId = NativeMethods.SetWindowsHookEx(NativeMethods.WH_KEYBOARD_LL, _proc, hMod, 0);
        }

        private IntPtr HookCallback(int nCode, IntPtr wParam, IntPtr lParam)
        {
            try
            {
                if (nCode >= 0 && (wParam == (IntPtr)NativeMethods.WM_KEYDOWN || wParam == (IntPtr)NativeMethods.WM_SYSKEYDOWN))
                {
                    var hookStruct = Marshal.PtrToStructure<NativeMethods.KBDLLHOOKSTRUCT>(lParam);
                    uint vkCode = hookStruct.vkCode;

                    bool isCtrlDown = (NativeMethods.GetAsyncKeyState(NativeMethods.VK_CONTROL) & 0x8000) != 0 ||
                                      (NativeMethods.GetAsyncKeyState(NativeMethods.VK_LCONTROL) & 0x8000) != 0 ||
                                      (NativeMethods.GetAsyncKeyState(NativeMethods.VK_RCONTROL) & 0x8000) != 0;

                    if (isCtrlDown)
                    {
                        if (vkCode == NativeMethods.VK_C || vkCode == NativeMethods.VK_X || vkCode == NativeMethods.VK_V)
                        {
                            // Check if active foreground window belongs to Illustrator
                            IntPtr fgHwnd = NativeMethods.GetForegroundWindow();
                            NativeMethods.GetWindowThreadProcessId(fgHwnd, out uint fgPid);

                            if (fgPid > 0)
                            {
                                try
                                {
                                    using var fgProc = Process.GetProcessById((int)fgPid);
                                    if (fgProc.ProcessName.Equals("Illustrator", StringComparison.OrdinalIgnoreCase))
                                    {
                                        if (vkCode == NativeMethods.VK_C)
                                        {
                                            CopyTriggeredInIllustrator?.Invoke(false);
                                        }
                                        else if (vkCode == NativeMethods.VK_X)
                                        {
                                            CopyTriggeredInIllustrator?.Invoke(true);
                                        }
                                        else if (vkCode == NativeMethods.VK_V)
                                        {
                                            PasteTriggeredInIllustrator?.Invoke();
                                        }
                                    }
                                }
                                catch
                                {
                                    // Process might have terminated
                                }
                            }
                        }
                    }
                }
            }
            catch
            {
                // Never allow unmanaged hook to throw
            }

            return NativeMethods.CallNextHookEx(_hookId, nCode, wParam, lParam);
        }

        public void Dispose()
        {
            if (!_isDisposed)
            {
                if (_hookId != IntPtr.Zero)
                {
                    NativeMethods.UnhookWindowsHookEx(_hookId);
                    _hookId = IntPtr.Zero;
                }
                _isDisposed = true;
            }
        }
    }
}
