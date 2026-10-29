using System;
using System.Collections.Generic;
using System.Diagnostics;
using AI_ClipboardDoctor.Models;

namespace AI_ClipboardDoctor.Core
{
    public class CopyEventTracker : IDisposable
    {
        private readonly KeyboardHook _keyboardHook;
        private readonly ClipboardMonitor _clipboardMonitor;
        private readonly IllustratorTracker _illustratorTracker;
        private readonly System.Windows.Forms.Timer _timeoutTimer;

        private long _lastCopyTimestamp;
        private bool _pendingCopy;
        private bool _pendingIsCut;
        private int _nextEventId = 1;

        public List<DiagnosticEvent> Events { get; } = new();

        public int TotalCopiesAttempted { get; private set; }
        public int SuccessfulVectorCopies { get; private set; }
        public int SuccessfulTextCopies { get; private set; }
        public int VectorMissingCopies { get; private set; }
        public int FailedNoUpdateCopies { get; private set; }
        public int ClipboardLockCollisions { get; private set; }

        public event Action<DiagnosticEvent>? NewEventAdded;
        public event Action? StatsChanged;

        public CopyEventTracker(KeyboardHook hook, ClipboardMonitor monitor, IllustratorTracker tracker)
        {
            _keyboardHook = hook;
            _clipboardMonitor = monitor;
            _illustratorTracker = tracker;

            _timeoutTimer = new System.Windows.Forms.Timer { Interval = 1200 };
            _timeoutTimer.Tick += OnCopyTimeout;

            _keyboardHook.CopyTriggeredInIllustrator += OnKeyboardCopy;
            _clipboardMonitor.ClipboardChanged += OnClipboardChanged;
        }

        private void OnKeyboardCopy(bool isCut)
        {
            _lastCopyTimestamp = Stopwatch.GetTimestamp();
            _pendingCopy = true;
            _pendingIsCut = isCut;
            TotalCopiesAttempted++;

            _timeoutTimer.Stop();
            _timeoutTimer.Start();

            string action = isCut ? "Ctrl+X (Cut)" : "Ctrl+C (Copy)";
            var evt = new DiagnosticEvent
            {
                Id = _nextEventId++,
                Timestamp = DateTime.Now,
                Type = CopyEventType.CtrlCPressed,
                Severity = EventSeverity.Info,
                IllustratorPid = _illustratorTracker.Pid,
                IllustratorVersion = _illustratorTracker.Version,
                GdiHandles = _illustratorTracker.GdiHandles,
                UserHandles = _illustratorTracker.UserHandles,
                Summary = $"{action} detected in Illustrator",
                Details = $"Waiting for Windows clipboard update (Seq: {NativeMethods.GetClipboardSequenceNumber()}). GDI: {_illustratorTracker.GdiHandles}, User: {_illustratorTracker.UserHandles}"
            };

            AddEvent(evt);
            StatsChanged?.Invoke();
        }

        private void OnClipboardChanged(ClipboardSnapshot snapshot)
        {
            long now = Stopwatch.GetTimestamp();
            long latencyMs = -1;

            if (_pendingCopy)
            {
                _timeoutTimer.Stop();
                _pendingCopy = false;
                latencyMs = (now - _lastCopyTimestamp) * 1000 / Stopwatch.Frequency;
            }

            // Check if clipboard is locked by another process
            if (snapshot.IsCurrentlyLocked && snapshot.OpenPid != _illustratorTracker.Pid && snapshot.OpenPid != Process.GetCurrentProcess().Id)
            {
                ClipboardLockCollisions++;
                var lockEvt = new DiagnosticEvent
                {
                    Id = _nextEventId++,
                    Timestamp = DateTime.Now,
                    Type = CopyEventType.ClipboardLocked,
                    Severity = EventSeverity.Warning,
                    LatencyMs = latencyMs,
                    IllustratorPid = _illustratorTracker.Pid,
                    IllustratorVersion = _illustratorTracker.Version,
                    GdiHandles = _illustratorTracker.GdiHandles,
                    UserHandles = _illustratorTracker.UserHandles,
                    Snapshot = snapshot,
                    Summary = $"Clipboard locked by '{snapshot.OpenProcessName}' (PID: {snapshot.OpenPid})",
                    Details = $"External process is holding the clipboard lock. This can cause Illustrator to abort Copy or Paste silently."
                };
                AddEvent(lockEvt);
                StatsChanged?.Invoke();
                return;
            }

            // Check if this clipboard update originated from Illustrator or was correlated to a pending copy
            bool isFromIllustrator = (snapshot.OwnerPid == _illustratorTracker.Pid && _illustratorTracker.Pid > 0) || latencyMs >= 0;

            if (isFromIllustrator)
            {
                if (snapshot.Formats.Count == 0)
                {
                    // CRITICAL CRASH: Illustrator emptied the clipboard but published 0 formats
                    VectorMissingCopies++;
                    var warnEvt = new DiagnosticEvent
                    {
                        Id = _nextEventId++,
                        Timestamp = DateTime.Now,
                        Type = CopyEventType.CopyVectorMissing,
                        Severity = EventSeverity.Warning,
                        LatencyMs = latencyMs,
                        IllustratorPid = _illustratorTracker.Pid,
                        IllustratorVersion = _illustratorTracker.Version,
                        GdiHandles = _illustratorTracker.GdiHandles,
                        UserHandles = _illustratorTracker.UserHandles,
                        Snapshot = snapshot,
                        Summary = "⚠️ CRITICAL: Clipboard is EMPTY after Copy! (0 Formats)",
                        Details = "Illustrator emptied the clipboard on Ctrl+C, but internal vector/text serializer failed and closed clipboard with 0 formats. Paste will not work."
                    };
                    AddEvent(warnEvt);
                }
                else if (snapshot.HasVector)
                {
                    SuccessfulVectorCopies++;
                    var successEvt = new DiagnosticEvent
                    {
                        Id = _nextEventId++,
                        Timestamp = DateTime.Now,
                        Type = CopyEventType.CopySuccess,
                        Severity = EventSeverity.Success,
                        LatencyMs = latencyMs,
                        IllustratorPid = _illustratorTracker.Pid,
                        IllustratorVersion = _illustratorTracker.Version,
                        GdiHandles = _illustratorTracker.GdiHandles,
                        UserHandles = _illustratorTracker.UserHandles,
                        Snapshot = snapshot,
                        Summary = $"Vector Copy Successful ({snapshot.VectorFormatsSummary})",
                        Details = $"Published {snapshot.Formats.Count} formats. Latency: {(latencyMs >= 0 ? latencyMs + " ms" : "Mouse/Menu")}. Formats: {string.Join(", ", snapshot.Formats.ConvertAll(f => f.Name))}"
                    };
                    AddEvent(successEvt);
                }
                else if (snapshot.HasText)
                {
                    SuccessfulTextCopies++;
                    var textEvt = new DiagnosticEvent
                    {
                        Id = _nextEventId++,
                        Timestamp = DateTime.Now,
                        Type = CopyEventType.TextCopySuccess,
                        Severity = EventSeverity.Info,
                        LatencyMs = latencyMs,
                        IllustratorPid = _illustratorTracker.Pid,
                        IllustratorVersion = _illustratorTracker.Version,
                        GdiHandles = _illustratorTracker.GdiHandles,
                        UserHandles = _illustratorTracker.UserHandles,
                        Snapshot = snapshot,
                        Summary = "Text Copy Successful (CF_UNICODETEXT / RTF)",
                        Details = $"Published {snapshot.Formats.Count} text formats. Vector formats are not expected for text-only selection."
                    };
                    AddEvent(textEvt);
                }
                else
                {
                    var otherEvt = new DiagnosticEvent
                    {
                        Id = _nextEventId++,
                        Timestamp = DateTime.Now,
                        Type = CopyEventType.CopySuccess,
                        Severity = EventSeverity.Info,
                        LatencyMs = latencyMs,
                        IllustratorPid = _illustratorTracker.Pid,
                        IllustratorVersion = _illustratorTracker.Version,
                        GdiHandles = _illustratorTracker.GdiHandles,
                        UserHandles = _illustratorTracker.UserHandles,
                        Snapshot = snapshot,
                        Summary = $"Bitmap/Object Copy ({snapshot.Formats.Count} formats)",
                        Details = $"Published formats: {string.Join(", ", snapshot.Formats.ConvertAll(f => f.Name))}"
                    };
                    AddEvent(otherEvt);
                }

                StatsChanged?.Invoke();
            }
        }

        private void OnCopyTimeout(object? sender, EventArgs e)
        {
            _timeoutTimer.Stop();

            if (_pendingCopy)
            {
                _pendingCopy = false;
                FailedNoUpdateCopies++;

                var failEvt = new DiagnosticEvent
                {
                    Id = _nextEventId++,
                    Timestamp = DateTime.Now,
                    Type = CopyEventType.CopyFailNoUpdate,
                    Severity = EventSeverity.Error,
                    LatencyMs = 1200,
                    IllustratorPid = _illustratorTracker.Pid,
                    IllustratorVersion = _illustratorTracker.Version,
                    GdiHandles = _illustratorTracker.GdiHandles,
                    UserHandles = _illustratorTracker.UserHandles,
                    Summary = "CRITICAL: Illustrator Failed to Update Clipboard!",
                    Details = "User pressed Ctrl+C in Illustrator, but NO clipboard update was registered within 1200ms. Illustrator silently failed or aborted the copy operation."
                };

                AddEvent(failEvt);
                StatsChanged?.Invoke();
            }
        }

        public void RecordManualInspect(ClipboardSnapshot snapshot)
        {
            var evt = new DiagnosticEvent
            {
                Id = _nextEventId++,
                Timestamp = DateTime.Now,
                Type = CopyEventType.ManualInspect,
                Severity = EventSeverity.Info,
                IllustratorPid = _illustratorTracker.Pid,
                IllustratorVersion = _illustratorTracker.Version,
                GdiHandles = _illustratorTracker.GdiHandles,
                UserHandles = _illustratorTracker.UserHandles,
                Snapshot = snapshot,
                Summary = $"Manual Clipboard Inspection: {snapshot.Formats.Count} formats ({snapshot.VectorFormatsSummary})",
                Details = $"Owner: {snapshot.OwnerProcessName} (PID: {snapshot.OwnerPid}). Total size: {snapshot.TotalBytes / 1024.0:F1} KB. Formats: {string.Join(", ", snapshot.Formats.ConvertAll(f => f.Name))}"
            };
            AddEvent(evt);
        }

        public void RecordTestInjected(bool success, string message, ClipboardSnapshot? snapshot)
        {
            var evt = new DiagnosticEvent
            {
                Id = _nextEventId++,
                Timestamp = DateTime.Now,
                Type = CopyEventType.TestInjected,
                Severity = success ? EventSeverity.Success : EventSeverity.Error,
                IllustratorPid = _illustratorTracker.Pid,
                IllustratorVersion = _illustratorTracker.Version,
                GdiHandles = _illustratorTracker.GdiHandles,
                UserHandles = _illustratorTracker.UserHandles,
                Snapshot = snapshot,
                Summary = success ? "Injected Test Vector Object (AICB + SVG + Text)" : "Failed to Inject Test Vector",
                Details = message
            };
            AddEvent(evt);
        }

        public void RecordReset(bool success, string message)
        {
            var evt = new DiagnosticEvent
            {
                Id = _nextEventId++,
                Timestamp = DateTime.Now,
                Type = CopyEventType.ClipboardReset,
                Severity = success ? EventSeverity.Success : EventSeverity.Error,
                IllustratorPid = _illustratorTracker.Pid,
                IllustratorVersion = _illustratorTracker.Version,
                GdiHandles = _illustratorTracker.GdiHandles,
                UserHandles = _illustratorTracker.UserHandles,
                Summary = success ? "Clipboard Emptied & Unlocked" : "Failed to Reset Clipboard",
                Details = message
            };
            AddEvent(evt);
        }

        public void ClearEvents()
        {
            lock (Events)
            {
                Events.Clear();
            }
            TotalCopiesAttempted = 0;
            SuccessfulVectorCopies = 0;
            SuccessfulTextCopies = 0;
            VectorMissingCopies = 0;
            FailedNoUpdateCopies = 0;
            ClipboardLockCollisions = 0;
            StatsChanged?.Invoke();
        }

        private void AddEvent(DiagnosticEvent evt)
        {
            lock (Events)
            {
                Events.Insert(0, evt);
                if (Events.Count > 1000)
                {
                    Events.RemoveAt(Events.Count - 1);
                }
            }

            NewEventAdded?.Invoke(evt);
        }

        public void Dispose()
        {
            _timeoutTimer.Stop();
            _timeoutTimer.Dispose();
            _keyboardHook.CopyTriggeredInIllustrator -= OnKeyboardCopy;
            _clipboardMonitor.ClipboardChanged -= OnClipboardChanged;
        }
    }
}
