using System;
using System.Diagnostics;
using System.Text;

namespace AI_ClipboardDoctor.Core
{
    public class IllustratorTracker : IDisposable
    {
        private readonly System.Windows.Forms.Timer _pollTimer;
        private Process? _cachedProcess;

        public bool IsRunning => _cachedProcess != null && !_cachedProcess.HasExited;
        public int Pid => IsRunning ? _cachedProcess!.Id : 0;
        public string Version { get; private set; } = "Not Running";
        public int GdiHandles { get; private set; }
        public int UserHandles { get; private set; }
        public long WorkingSetMb { get; private set; }
        public bool IsForeground { get; private set; }
        public string MainWindowTitle { get; private set; } = string.Empty;

        public event Action? StateChanged;

        public IllustratorTracker()
        {
            _pollTimer = new System.Windows.Forms.Timer { Interval = 1000 };
            _pollTimer.Tick += (s, e) => Refresh();
            _pollTimer.Start();
            Refresh();
        }

        public void Refresh()
        {
            try
            {
                if (_cachedProcess == null || _cachedProcess.HasExited)
                {
                    _cachedProcess?.Dispose();
                    _cachedProcess = null;

                    var processes = Process.GetProcessesByName("Illustrator");
                    if (processes.Length > 0)
                    {
                        _cachedProcess = processes[0];
                        try
                        {
                            Version = _cachedProcess.MainModule?.FileVersionInfo.FileVersion ?? "Unknown";
                        }
                        catch
                        {
                            Version = "Adobe Illustrator";
                        }
                    }
                    else
                    {
                        Version = "Not Running";
                        GdiHandles = 0;
                        UserHandles = 0;
                        WorkingSetMb = 0;
                        IsForeground = false;
                        MainWindowTitle = string.Empty;
                        StateChanged?.Invoke();
                        return;
                    }
                }

                if (_cachedProcess != null && !_cachedProcess.HasExited)
                {
                    _cachedProcess.Refresh();
                    WorkingSetMb = _cachedProcess.WorkingSet64 / (1024 * 1024);

                    // GUI handles
                    IntPtr hProcess = _cachedProcess.Handle;
                    GdiHandles = (int)NativeMethods.GetGuiResources(hProcess, NativeMethods.GR_GDIOBJECTS);
                    UserHandles = (int)NativeMethods.GetGuiResources(hProcess, NativeMethods.GR_USEROBJECTS);

                    // Check foreground window
                    IntPtr fgHwnd = NativeMethods.GetForegroundWindow();
                    NativeMethods.GetWindowThreadProcessId(fgHwnd, out uint fgPid);
                    IsForeground = (fgPid == _cachedProcess.Id);

                    var sb = new StringBuilder(256);
                    NativeMethods.GetWindowText(fgHwnd, sb, 256);
                    if (IsForeground)
                    {
                        MainWindowTitle = sb.ToString();
                    }
                }
            }
            catch
            {
                // Process might have terminated or access denied
                _cachedProcess = null;
            }

            StateChanged?.Invoke();
        }

        public void Dispose()
        {
            _pollTimer.Stop();
            _pollTimer.Dispose();
            _cachedProcess?.Dispose();
        }
    }
}
