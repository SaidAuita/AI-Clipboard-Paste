using System;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;
using AI_ClipboardDoctor.Core;
using AI_ClipboardDoctor.UI;

namespace AI_ClipboardDoctor
{
    internal static class Program
    {
        [STAThread]
        static void Main()
        {
            var current = Process.GetCurrentProcess();
            var running = Process.GetProcessesByName(current.ProcessName);

            if (running.Length > 1)
            {
                // Another instance is already running! Restore its window and bring to front.
                foreach (var p in running)
                {
                    if (p.Id != current.Id && p.MainWindowHandle != IntPtr.Zero)
                    {
                        NativeMethods.ShowWindow(p.MainWindowHandle, NativeMethods.SW_RESTORE);
                        NativeMethods.SetForegroundWindow(p.MainWindowHandle);
                        return;
                    }
                }
                return;
            }

            ApplicationConfiguration.Initialize();
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.SetUnhandledExceptionMode(UnhandledExceptionMode.CatchException);

            Application.ThreadException += (s, e) =>
            {
                try
                {
                    string logPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "crash.log");
                    File.AppendAllText(logPath, $"[{DateTime.Now}] UI THREAD EXCEPTION: {e.Exception}\r\n");
                }
                catch { }
            };

            AppDomain.CurrentDomain.UnhandledException += (s, e) =>
            {
                try
                {
                    string logPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "crash.log");
                    File.AppendAllText(logPath, $"[{DateTime.Now}] CRASH: {e.ExceptionObject}\r\n");
                }
                catch { }
            };

            try
            {
                Application.Run(new MainForm());
            }
            catch (Exception ex)
            {
                try
                {
                    string logPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "crash.log");
                    File.AppendAllText(logPath, $"[{DateTime.Now}] CRASH IN MAIN: {ex}\r\n");
                }
                catch { }
            }
        }
    }
}