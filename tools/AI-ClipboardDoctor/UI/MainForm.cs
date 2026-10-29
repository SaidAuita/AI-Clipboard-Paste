using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using AI_ClipboardDoctor.Core;
using AI_ClipboardDoctor.Models;

namespace AI_ClipboardDoctor.UI
{
    public class MainForm : Form
    {
        private readonly IllustratorTracker _aiTracker;
        private readonly KeyboardHook _keyboardHook;
        private readonly ClipboardMonitor _clipboardMonitor;
        private readonly CopyEventTracker _eventTracker;

        // UI Controls
        private Panel _pnlHeader = null!;
        private Label _lblAppTitle = null!;
        private Label _lblStatusBadge = null!;
        private Label _lblAiStatus = null!;
        private Label _lblGdiStatus = null!;
        private Label _lblClipboardLockStatus = null!;

        // Cards
        private Panel _cardCopies = null!;
        private Label _lblCardCopiesVal = null!;
        private Label _lblCardCopiesSub = null!;

        private Panel _cardFails = null!;
        private Label _lblCardFailsVal = null!;
        private Label _lblCardFailsSub = null!;

        private Panel _cardLatency = null!;
        private Label _lblCardLatencyVal = null!;
        private Label _lblCardLatencySub = null!;

        // Filter buttons
        private Button _btnFilterAll = null!;
        private Button _btnFilterFails = null!;
        private Button _btnFilterVectors = null!;
        private string _currentFilter = "ALL";

        // Lists & Inspector
        private ListView _lvEvents = null!;
        private ListView _lvFormats = null!;
        private Label _lblInspectorTitle = null!;
        private Button _btnViewLog = null!;
        private Button _btnViewFormats = null!;
        private TextBox _txtEventDetails = null!;
        private Panel _pnlRightContent = null!;

        // Action Buttons
        private Button _btnInjectTest = null!;
        private Button _btnResetClipboard = null!;
        private Button _btnRescueCopy = null!;
        private Button _btnInspectNow = null!;
        private Button _btnExportReport = null!;
        private Button _btnClearLog = null!;

        // Tray
        private NotifyIcon _notifyIcon = null!;
        private bool _notifyOnFailure = true;

        public MainForm()
        {
            this.Text = "Adobe Illustrator Clipboard Doctor & Sentinel v1.0.3";
            this.Size = new Size(1220, 760);
            this.MinimumSize = new Size(1020, 650);
            this.StartPosition = FormStartPosition.CenterScreen;
            this.BackColor = Color.FromArgb(15, 18, 24);
            this.ForeColor = Color.White;
            this.Font = new Font("Segoe UI", 9.0f, FontStyle.Regular);

            // Initialize Services
            _aiTracker = new IllustratorTracker();
            _keyboardHook = new KeyboardHook();
            _clipboardMonitor = new ClipboardMonitor();
            _eventTracker = new CopyEventTracker(_keyboardHook, _clipboardMonitor, _aiTracker);

            InitComponents();
            InitTray();

            // Wire Service Events
            _aiTracker.StateChanged += OnAiStateChanged;
            _eventTracker.NewEventAdded += OnNewEventAdded;
            _eventTracker.StatsChanged += OnStatsChanged;

            // Initial manual inspection
            var initialSnapshot = _clipboardMonitor.CaptureSnapshot();
            _eventTracker.RecordManualInspect(initialSnapshot);

            UpdateUiState();
        }

        private void InitComponents()
        {
            this.SuspendLayout();

            // Top Header Panel
            _pnlHeader = new Panel
            {
                Dock = DockStyle.Top,
                Height = 60,
                BackColor = Color.FromArgb(20, 24, 33),
                Padding = new Padding(16, 10, 16, 10)
            };

            _lblAppTitle = new Label
            {
                Text = "⚡ AI-CLIPBOARD DOCTOR SENTINEL",
                Font = new Font("Segoe UI", 12.0f, FontStyle.Bold),
                ForeColor = Color.FromArgb(255, 255, 255),
                AutoSize = true,
                Location = new Point(16, 18)
            };

            var lblVersionBadge = new Label
            {
                Text = "v1.0.1",
                Font = new Font("Segoe UI", 8.0f, FontStyle.Bold),
                ForeColor = Color.FromArgb(0, 210, 106),
                BackColor = Color.FromArgb(10, 40, 25),
                Padding = new Padding(6, 3, 6, 3),
                AutoSize = true,
                Location = new Point(_lblAppTitle.Right + 12, 19)
            };

            _lblStatusBadge = new Label
            {
                Text = "● ACTIVE MONITORING",
                Font = new Font("Segoe UI", 9.0f, FontStyle.Bold),
                ForeColor = Color.FromArgb(0, 210, 106),
                AutoSize = true,
                Location = new Point(lblVersionBadge.Right + 20, 20)
            };

            var pnlHeaderRight = new FlowLayoutPanel
            {
                Dock = DockStyle.Right,
                AutoSize = true,
                FlowDirection = FlowDirection.RightToLeft,
                WrapContents = false,
                BackColor = Color.Transparent,
                Padding = new Padding(0, 6, 8, 0)
            };

            _lblClipboardLockStatus = new Label
            {
                Text = "Clipboard: Free",
                Font = new Font("Segoe UI", 9.0f),
                ForeColor = Color.FromArgb(160, 175, 195),
                AutoSize = true,
                Margin = new Padding(16, 6, 0, 0)
            };

            _lblGdiStatus = new Label
            {
                Text = "GDI Handles: 0 / 10,000",
                Font = new Font("Segoe UI", 9.0f),
                ForeColor = Color.FromArgb(160, 175, 195),
                AutoSize = true,
                Margin = new Padding(16, 6, 0, 0)
            };

            _lblAiStatus = new Label
            {
                Text = "Illustrator: Scanning...",
                Font = new Font("Segoe UI", 9.0f, FontStyle.Bold),
                ForeColor = Color.FromArgb(255, 165, 2),
                AutoSize = true,
                Margin = new Padding(0, 6, 0, 0)
            };

            pnlHeaderRight.Controls.Add(_lblClipboardLockStatus);
            pnlHeaderRight.Controls.Add(_lblGdiStatus);
            pnlHeaderRight.Controls.Add(_lblAiStatus);

            _pnlHeader.Controls.AddRange(new Control[] {
                _lblAppTitle, lblVersionBadge, _lblStatusBadge, pnlHeaderRight
            });

            // KPI Cards Panel (Responsive TableLayoutPanel)
            var pnlCards = new TableLayoutPanel
            {
                Dock = DockStyle.Top,
                Height = 85,
                BackColor = Color.FromArgb(15, 18, 24),
                Padding = new Padding(12, 6, 12, 6),
                ColumnCount = 3,
                RowCount = 1
            };
            pnlCards.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 33.33f));
            pnlCards.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 33.33f));
            pnlCards.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 33.34f));

            _cardCopies = CreateKpiCard("TOTAL COPIES", "0", "0 Successful (0%)", Color.FromArgb(0, 210, 106), out _lblCardCopiesVal, out _lblCardCopiesSub);
            _cardFails = CreateKpiCard("FAILURES DETECTED", "0", "0 No-Update | 0 No-Vectors", Color.FromArgb(255, 71, 87), out _lblCardFailsVal, out _lblCardFailsSub);
            _cardLatency = CreateKpiCard("CLIPBOARD HEALTH", "HEALTHY", "Lock Collisions: 0", Color.FromArgb(52, 152, 219), out _lblCardLatencyVal, out _lblCardLatencySub);

            pnlCards.Controls.Add(_cardCopies, 0, 0);
            pnlCards.Controls.Add(_cardFails, 1, 0);
            pnlCards.Controls.Add(_cardLatency, 2, 0);

            // Filter Bar Panel
            var pnlFilterBar = new Panel
            {
                Dock = DockStyle.Top,
                Height = 42,
                BackColor = Color.FromArgb(20, 24, 33),
                Padding = new Padding(16, 6, 16, 6)
            };

            _btnFilterAll = CreateFilterButton("All Events (0)", "ALL", 16, true);
            _btnFilterFails = CreateFilterButton("Failures & Warnings (0)", "FAILS", _btnFilterAll.Right + 8, false);
            _btnFilterVectors = CreateFilterButton("Vectors Only (0)", "VECTORS", _btnFilterFails.Right + 8, false);

            var lblFilterHint = new Label
            {
                Text = "Кликните на любое событие слева для вывода полного отчета справа",
                Font = new Font("Segoe UI", 8.5f, FontStyle.Italic),
                ForeColor = Color.FromArgb(120, 135, 155),
                AutoSize = true,
                Location = new Point(_btnFilterVectors.Right + 20, 12)
            };

            pnlFilterBar.Controls.AddRange(new Control[] { _btnFilterAll, _btnFilterFails, _btnFilterVectors, lblFilterHint });

            // Action Bar (Bottom) - FlowLayoutPanel guarantees buttons never overlap!
            var pnlActions = new FlowLayoutPanel
            {
                Dock = DockStyle.Bottom,
                Height = 66,
                BackColor = Color.FromArgb(20, 24, 33),
                Padding = new Padding(16, 12, 16, 12),
                FlowDirection = FlowDirection.LeftToRight,
                WrapContents = false,
                AutoScroll = true
            };

            _btnInjectTest = CreateActionButton("⚡ Тест вставки (Ctrl+V)", Color.FromArgb(0, 150, 75));
            _btnInjectTest.Click += (s, e) => OnInjectTestVector();

            // Highlighted Reset & Unlock Button - clearly visible!
            _btnResetClipboard = CreateActionButton("🔓 СБРОС БУФЕРА (Reset & Unlock)", Color.FromArgb(215, 65, 35));
            _btnResetClipboard.Font = new Font("Segoe UI", 9.0f, FontStyle.Bold);
            _btnResetClipboard.Click += (s, e) => OnResetClipboard();

            // Rescue Copy (.jsx) Button - direct memory transfer!
            _btnRescueCopy = CreateActionButton("🛟 Rescue Copy (.jsx)", Color.FromArgb(142, 68, 173));
            _btnRescueCopy.Font = new Font("Segoe UI", 9.0f, FontStyle.Bold);
            _btnRescueCopy.Click += (s, e) => OnExecuteRescueCopy();

            _btnInspectNow = CreateActionButton("📋 Инспекция буфера", Color.FromArgb(45, 58, 80));
            _btnInspectNow.Click += (s, e) => OnInspectNow();

            _btnExportReport = CreateActionButton("💾 Экспорт отчета (.TXT / .JSON / .CSV)", Color.FromArgb(0, 120, 215));
            _btnExportReport.Click += (s, e) => OnExportReport();

            _btnClearLog = CreateActionButton("🗑️ Очистить", Color.FromArgb(70, 28, 35));
            _btnClearLog.Click += (s, e) => OnClearEvents();

            pnlActions.Controls.AddRange(new Control[] {
                _btnInjectTest, _btnResetClipboard, _btnRescueCopy, _btnInspectNow, _btnExportReport, _btnClearLog
            });

            // Main Split Panel (Left = Events ListView, Right = Formats & Full Height Log)
            var splitMain = new SplitContainer
            {
                Dock = DockStyle.Fill,
                Orientation = Orientation.Vertical,
                SplitterDistance = 600,
                SplitterWidth = 6,
                BackColor = Color.FromArgb(15, 18, 24)
            };

            // Left: Events ListView
            _lvEvents = new ListView
            {
                Dock = DockStyle.Fill,
                View = View.Details,
                FullRowSelect = true,
                MultiSelect = false,
                GridLines = true,
                BackColor = Color.FromArgb(18, 22, 30),
                ForeColor = Color.White,
                BorderStyle = BorderStyle.None,
                Font = new Font("Segoe UI", 9.0f)
            };

            _lvEvents.Columns.Add("Time", 85);
            _lvEvents.Columns.Add("Type", 135);
            _lvEvents.Columns.Add("Latency", 75);
            _lvEvents.Columns.Add("GDI", 65);
            _lvEvents.Columns.Add("Summary", 320);
            _lvEvents.SelectedIndexChanged += OnEventSelectionChanged;

            splitMain.Panel1.Controls.Add(_lvEvents);
            splitMain.Panel1.Padding = new Padding(16, 8, 4, 8);

            // Right: Inspector Panel (Full Height Log)
            var pnlRight = new Panel
            {
                Dock = DockStyle.Fill,
                BackColor = Color.FromArgb(18, 22, 30),
                Padding = new Padding(8)
            };

            var pnlRightHeader = new Panel
            {
                Dock = DockStyle.Top,
                Height = 36,
                BackColor = Color.FromArgb(24, 29, 40),
                Padding = new Padding(8, 4, 8, 4)
            };

            _lblInspectorTitle = new Label
            {
                Text = "📋 ДЕТАЛЬНЫЙ ЛОГ СОБЫТИЯ",
                Font = new Font("Segoe UI", 9.5f, FontStyle.Bold),
                ForeColor = Color.FromArgb(0, 210, 106),
                Dock = DockStyle.Left,
                AutoSize = true,
                Padding = new Padding(0, 4, 0, 0)
            };

            var pnlRightViews = new FlowLayoutPanel
            {
                Dock = DockStyle.Right,
                AutoSize = true,
                FlowDirection = FlowDirection.RightToLeft,
                WrapContents = false,
                BackColor = Color.Transparent
            };

            _btnViewFormats = new Button
            {
                Text = "📦 Таблица форматов",
                Font = new Font("Segoe UI", 8.0f, FontStyle.Bold),
                BackColor = Color.FromArgb(35, 42, 56),
                ForeColor = Color.FromArgb(180, 195, 215),
                FlatStyle = FlatStyle.Flat,
                Size = new Size(140, 28),
                Margin = new Padding(4, 0, 0, 0),
                Cursor = Cursors.Hand
            };
            _btnViewFormats.FlatAppearance.BorderSize = 0;
            _btnViewFormats.Click += (s, e) => SwitchRightView(showLog: false);

            _btnViewLog = new Button
            {
                Text = "📋 Лог на всю высоту",
                Font = new Font("Segoe UI", 8.0f, FontStyle.Bold),
                BackColor = Color.FromArgb(0, 120, 215),
                ForeColor = Color.White,
                FlatStyle = FlatStyle.Flat,
                Size = new Size(140, 28),
                Margin = new Padding(4, 0, 4, 0),
                Cursor = Cursors.Hand
            };
            _btnViewLog.FlatAppearance.BorderSize = 0;
            _btnViewLog.Click += (s, e) => SwitchRightView(showLog: true);

            pnlRightViews.Controls.Add(_btnViewFormats);
            pnlRightViews.Controls.Add(_btnViewLog);

            pnlRightHeader.Controls.Add(_lblInspectorTitle);
            pnlRightHeader.Controls.Add(pnlRightViews);

            _pnlRightContent = new Panel
            {
                Dock = DockStyle.Fill,
                BackColor = Color.FromArgb(18, 22, 30),
                Padding = new Padding(0, 6, 0, 0)
            };

            // Full Height Text Log
            _txtEventDetails = new TextBox
            {
                Dock = DockStyle.Fill,
                Multiline = true,
                ReadOnly = true,
                ScrollBars = ScrollBars.Both,
                WordWrap = true,
                BackColor = Color.FromArgb(12, 15, 20),
                ForeColor = Color.FromArgb(220, 230, 245),
                BorderStyle = BorderStyle.FixedSingle,
                Font = new Font("Consolas", 9.5f)
            };

            // Formats ListView (Alternative View)
            _lvFormats = new ListView
            {
                Dock = DockStyle.Fill,
                View = View.Details,
                FullRowSelect = true,
                GridLines = true,
                BackColor = Color.FromArgb(24, 29, 39),
                ForeColor = Color.White,
                BorderStyle = BorderStyle.FixedSingle,
                Font = new Font("Segoe UI", 9.0f),
                Visible = false
            };

            _lvFormats.Columns.Add("Format Name", 240);
            _lvFormats.Columns.Add("Kind", 90);
            _lvFormats.Columns.Add("Size", 90);

            _pnlRightContent.Controls.Add(_txtEventDetails);
            _pnlRightContent.Controls.Add(_lvFormats);

            pnlRight.Controls.Add(_pnlRightContent);
            pnlRight.Controls.Add(pnlRightHeader);

            splitMain.Panel2.Controls.Add(pnlRight);
            splitMain.Panel2.Padding = new Padding(4, 8, 16, 8);

            // Assemble main form layout
            this.Controls.Add(splitMain);
            this.Controls.Add(pnlFilterBar);
            this.Controls.Add(pnlCards);
            this.Controls.Add(_pnlHeader);
            this.Controls.Add(pnlActions);

            this.ResumeLayout(false);
        }

        private void SwitchRightView(bool showLog)
        {
            _txtEventDetails.Visible = showLog;
            _lvFormats.Visible = !showLog;

            _btnViewLog.BackColor = showLog ? Color.FromArgb(0, 120, 215) : Color.FromArgb(35, 42, 56);
            _btnViewLog.ForeColor = showLog ? Color.White : Color.FromArgb(180, 195, 215);

            _btnViewFormats.BackColor = !showLog ? Color.FromArgb(0, 120, 215) : Color.FromArgb(35, 42, 56);
            _btnViewFormats.ForeColor = !showLog ? Color.White : Color.FromArgb(180, 195, 215);
        }

        private Panel CreateKpiCard(string title, string initialVal, string sub, Color accentColor, out Label lblVal, out Label lblSub)
        {
            var pnl = new Panel
            {
                Dock = DockStyle.Fill,
                Margin = new Padding(4, 2, 4, 2),
                BackColor = Color.FromArgb(20, 24, 33),
                BorderStyle = BorderStyle.FixedSingle
            };

            var accentStrip = new Panel
            {
                Dock = DockStyle.Left,
                Width = 4,
                BackColor = accentColor
            };

            var lblT = new Label
            {
                Text = title,
                Font = new Font("Segoe UI", 8.0f, FontStyle.Bold),
                ForeColor = Color.FromArgb(140, 155, 175),
                Location = new Point(14, 8),
                AutoSize = true
            };

            lblVal = new Label
            {
                Text = initialVal,
                Font = new Font("Segoe UI", 14.0f, FontStyle.Bold),
                ForeColor = Color.White,
                Location = new Point(12, 26),
                AutoSize = true
            };

            lblSub = new Label
            {
                Text = sub,
                Font = new Font("Segoe UI", 8.0f),
                ForeColor = Color.FromArgb(140, 155, 175),
                Location = new Point(14, 52),
                AutoSize = true
            };

            pnl.Controls.AddRange(new Control[] { accentStrip, lblT, lblVal, lblSub });
            return pnl;
        }

        private Button CreateFilterButton(string text, string filterTag, int left, bool active)
        {
            var btn = new Button
            {
                Text = text,
                Tag = filterTag,
                Location = new Point(left, 6),
                Height = 30,
                AutoSize = true,
                FlatStyle = FlatStyle.Flat,
                Font = new Font("Segoe UI", 8.5f, FontStyle.Bold),
                BackColor = active ? Color.FromArgb(0, 120, 215) : Color.FromArgb(30, 36, 48),
                ForeColor = Color.White,
                Cursor = Cursors.Hand
            };
            btn.FlatAppearance.BorderSize = 0;
            btn.Click += (s, e) => SetFilter(filterTag);
            return btn;
        }

        private Button CreateActionButton(string text, Color bg)
        {
            var btn = new Button
            {
                Text = text,
                Height = 38,
                AutoSize = true,
                FlatStyle = FlatStyle.Flat,
                Font = new Font("Segoe UI", 9.0f, FontStyle.Bold),
                BackColor = bg,
                ForeColor = Color.White,
                Cursor = Cursors.Hand,
                Padding = new Padding(14, 4, 14, 4),
                Margin = new Padding(0, 0, 12, 0)
            };
            btn.FlatAppearance.BorderSize = 0;
            return btn;
        }

        private void InitTray()
        {
            _notifyIcon = new NotifyIcon
            {
                Text = "AI-ClipboardDoctor Sentinel",
                Icon = SystemIcons.Shield,
                Visible = true
            };

            var menu = new ContextMenuStrip();
            menu.Items.Add("Open Dashboard", null, (s, e) => { this.Show(); this.WindowState = FormWindowState.Normal; this.BringToFront(); });
            menu.Items.Add("Test Vector Inject", null, (s, e) => OnInjectTestVector());
            menu.Items.Add("Reset Clipboard", null, (s, e) => OnResetClipboard());
            menu.Items.Add("-");
            menu.Items.Add("Exit", null, (s, e) => { _notifyIcon.Visible = false; Application.Exit(); });

            _notifyIcon.ContextMenuStrip = menu;
            _notifyIcon.DoubleClick += (s, e) => { this.Show(); this.WindowState = FormWindowState.Normal; this.BringToFront(); };
        }

        private void OnAiStateChanged()
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new Action(OnAiStateChanged));
                return;
            }

            if (_aiTracker.IsRunning)
            {
                _lblAiStatus.Text = $"Illustrator: v{_aiTracker.Version} (PID: {_aiTracker.Pid})";
                _lblAiStatus.ForeColor = Color.FromArgb(0, 210, 106);

                string gdiWarn = _aiTracker.GdiHandles > 7000 ? " ⚠️ HIGH" : "";
                _lblGdiStatus.Text = $"GDI: {_aiTracker.GdiHandles:N0} / 10,000{gdiWarn} | User: {_aiTracker.UserHandles:N0}";
                _lblGdiStatus.ForeColor = _aiTracker.GdiHandles > 7000 ? Color.FromArgb(255, 71, 87) : Color.FromArgb(160, 175, 195);
            }
            else
            {
                _lblAiStatus.Text = "Illustrator: Not Running";
                _lblAiStatus.ForeColor = Color.FromArgb(140, 150, 165);
                _lblGdiStatus.Text = "GDI Handles: --";
                _lblGdiStatus.ForeColor = Color.FromArgb(140, 150, 165);
            }
        }

        private void OnStatsChanged()
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new Action(OnStatsChanged));
                return;
            }

            UpdateUiState();
        }

        private void UpdateUiState()
        {
            int total = _eventTracker.TotalCopiesAttempted;
            int vectorSuccess = _eventTracker.SuccessfulVectorCopies;
            int textSuccess = _eventTracker.SuccessfulTextCopies;
            int fails = _eventTracker.FailedNoUpdateCopies + _eventTracker.VectorMissingCopies;
            double rate = total > 0 ? (double)(vectorSuccess + textSuccess) / total * 100 : 0;

            _lblCardCopiesVal.Text = total.ToString();
            _lblCardCopiesSub.Text = $"{vectorSuccess} Vectors | {textSuccess} Text ({rate:F1}%)";

            _lblCardFailsVal.Text = fails.ToString();
            _lblCardFailsVal.ForeColor = fails > 0 ? Color.FromArgb(255, 71, 87) : Color.FromArgb(0, 210, 106);
            _lblCardFailsSub.Text = $"{_eventTracker.FailedNoUpdateCopies} No-Update | {_eventTracker.VectorMissingCopies} Empty-Clipboard";

            string lockText = _eventTracker.ClipboardLockCollisions > 0 
                ? $"Lock Collisions: {_eventTracker.ClipboardLockCollisions}" 
                : "No Lock Collisions";
            _lblCardLatencyVal.Text = fails == 0 ? "HEALTHY" : "ANOMALIES";
            _lblCardLatencyVal.ForeColor = fails == 0 ? Color.FromArgb(0, 210, 106) : Color.FromArgb(255, 71, 87);
            _lblCardLatencySub.Text = lockText;

            _btnFilterAll.Text = $"All Events ({_eventTracker.Events.Count})";
            _btnFilterFails.Text = $"Failures & Warnings ({_eventTracker.Events.Count(e => e.Severity == EventSeverity.Error || e.Severity == EventSeverity.Warning)})";
            _btnFilterVectors.Text = $"Vectors Only ({_eventTracker.Events.Count(e => e.Snapshot?.HasVector == true)})";
        }

        private void OnNewEventAdded(DiagnosticEvent evt)
        {
            if (this.InvokeRequired)
            {
                this.BeginInvoke(new Action<DiagnosticEvent>(OnNewEventAdded), evt);
                return;
            }

            // Notification on copy failure
            if (_notifyOnFailure && evt.Severity == EventSeverity.Error)
            {
                _notifyIcon.ShowBalloonTip(3000, "⚠️ Illustrator Clipboard Failure!", evt.Summary, ToolTipIcon.Error);
            }

            RefreshEventsList();
        }

        private void SetFilter(string filter)
        {
            _currentFilter = filter;
            _btnFilterAll.BackColor = filter == "ALL" ? Color.FromArgb(0, 120, 215) : Color.FromArgb(30, 36, 48);
            _btnFilterFails.BackColor = filter == "FAILS" ? Color.FromArgb(0, 120, 215) : Color.FromArgb(30, 36, 48);
            _btnFilterVectors.BackColor = filter == "VECTORS" ? Color.FromArgb(0, 120, 215) : Color.FromArgb(30, 36, 48);
            RefreshEventsList();
        }

        private void RefreshEventsList()
        {
            _lvEvents.BeginUpdate();
            _lvEvents.Items.Clear();

            List<DiagnosticEvent> filtered;
            lock (_eventTracker.Events)
            {
                filtered = _currentFilter switch
                {
                    "FAILS" => _eventTracker.Events.Where(e => e.Severity == EventSeverity.Error || e.Severity == EventSeverity.Warning).ToList(),
                    "VECTORS" => _eventTracker.Events.Where(e => e.Snapshot?.HasVector == true).ToList(),
                    _ => _eventTracker.Events.ToList()
                };
            }

            foreach (var evt in filtered)
            {
                var item = new ListViewItem(evt.Timestamp.ToString("HH:mm:ss.fff"));
                item.SubItems.Add(evt.TypeBadge);
                item.SubItems.Add(evt.LatencyMs >= 0 ? $"{evt.LatencyMs} ms" : "--");
                item.SubItems.Add(evt.GdiHandles > 0 ? evt.GdiHandles.ToString() : "--");
                item.SubItems.Add(evt.Summary);
                item.Tag = evt;

                item.ForeColor = evt.Severity switch
                {
                    EventSeverity.Success => Color.FromArgb(0, 210, 106),
                    EventSeverity.Warning => Color.FromArgb(255, 165, 2),
                    EventSeverity.Error => Color.FromArgb(255, 71, 87),
                    _ => Color.FromArgb(200, 210, 225)
                };

                _lvEvents.Items.Add(item);
            }

            _lvEvents.EndUpdate();

            if (_lvEvents.Items.Count > 0 && _lvEvents.SelectedIndices.Count == 0)
            {
                _lvEvents.Items[0].Selected = true;
            }
        }

        private void OnEventSelectionChanged(object? sender, EventArgs e)
        {
            if (_lvEvents.SelectedItems.Count == 0)
            {
                _lvFormats.Items.Clear();
                _txtEventDetails.Text = "Выберите событие в списке слева для просмотра подробного отчёта.";
                _lblInspectorTitle.Text = "📋 ДЕТАЛЬНЫЙ ЛОГ СОБЫТИЯ";
                return;
            }

            if (_lvEvents.SelectedItems[0].Tag is DiagnosticEvent evt)
            {
                _lblInspectorTitle.Text = $"[{evt.TypeBadge}] #{evt.Id} ({evt.Timestamp:HH:mm:ss})";
                _txtEventDetails.Text = BuildRichEventDetails(evt);

                _lvFormats.BeginUpdate();
                _lvFormats.Items.Clear();

                if (evt.Snapshot != null)
                {
                    foreach (var f in evt.Snapshot.Formats)
                    {
                        var item = new ListViewItem(f.Name);
                        string kind = f.IsVector ? "⭐ Vector" : (f.IsAdobe ? "Adobe" : "Standard");
                        item.SubItems.Add(kind);
                        item.SubItems.Add(f.FormattedSize);

                        if (f.IsVector)
                        {
                            item.ForeColor = Color.FromArgb(0, 210, 106);
                            item.Font = new Font(_lvFormats.Font, FontStyle.Bold);
                        }
                        else
                        {
                            item.ForeColor = Color.FromArgb(200, 210, 225);
                        }

                        _lvFormats.Items.Add(item);
                    }
                }

                _lvFormats.EndUpdate();
            }
        }

        private string BuildRichEventDetails(DiagnosticEvent evt)
        {
            var sb = new StringBuilder();
            sb.AppendLine("================================================================================");
            sb.AppendLine($"  СОБЫТИЕ #{evt.Id} — [{evt.TypeBadge}] — {evt.Severity.ToString().ToUpper()}");
            sb.AppendLine($"  Время: {evt.Timestamp:yyyy-MM-dd HH:mm:ss.fff}  |  Задержка реакции: {(evt.LatencyMs >= 0 ? evt.LatencyMs + " мс" : "N/A")}");
            sb.AppendLine("================================================================================");
            sb.AppendLine();

            sb.AppendLine("▶ [1] ДИАГНОЗ И АНАЛИЗ СБОЯ:");
            sb.AppendLine($"  Сводка: {evt.Summary}");
            sb.AppendLine($"  Детали: {evt.Details}");
            sb.AppendLine();

            switch (evt.Type)
            {
                case CopyEventType.CopyVectorMissing:
                    sb.AppendLine("  ⚠️ АНАЛИЗ ПРИЧИНЫ:");
                    sb.AppendLine("  - Illustrator зафиксировал Ctrl+C и вызвал EmptyClipboard() (буфер был очищен).");
                    sb.AppendLine("  - При попытке сериализации векторных контуров в PDF/AICB/SVG во внутреннем");
                    sb.AppendLine("    модуле Illustrator произошёл сбой, и он закрыл буфер, оставив его ПУСТЫМ (0 форматов).");
                    sb.AppendLine("  - Так как в буфере нет векторных данных, вставка Ctrl+V не работает.");
                    sb.AppendLine();
                    sb.AppendLine("  💡 ЧТО ДЕЛАТЬ:");
                    sb.AppendLine("  1. Нажмите внизу яркую оранжевую кнопку: [ 🔓 СБРОС БУФЕРА ].");
                    sb.AppendLine("  2. Если сбой повторяется даже в новом чистом документе (Ctrl+N):");
                    sb.AppendLine("     внутренний OLE-модуль Illustrator вошёл в невалидное состояние.");
                    sb.AppendLine("     Сохраните открытые макеты (Ctrl+S) и ПЕРЕЗАПУСТИТЕ Adobe Illustrator.");
                    break;

                case CopyEventType.CopyFailNoUpdate:
                    sb.AppendLine("  ❌ АНАЛИЗ ПРИЧИНЫ:");
                    sb.AppendLine("  - Нажатие Ctrl+C перехвачено в окне Illustrator, но за 1200 мс от Windows");
                    sb.AppendLine("    не поступило системного события обновления буфера (WM_CLIPBOARDUPDATE).");
                    sb.AppendLine("  - Illustrator проигнорировал команду копирования (завис, либо модальное окно перехватило фокус).");
                    break;

                case CopyEventType.ClipboardLocked:
                    sb.AppendLine("  🔒 АНАЛИЗ ПРИЧИНЫ:");
                    sb.AppendLine($"  - Буфер обмена Windows заблокирован процессом '{evt.Snapshot?.OpenProcessName}' (PID: {evt.Snapshot?.OpenPid}).");
                    sb.AppendLine("  - Стороннее приложение удерживает системный замок OpenClipboard(), блокируя Illustrator.");
                    sb.AppendLine("  - Нажмите [ 🔓 СБРОС БУФЕРА ] для принудительного освобождения замка.");
                    break;

                case CopyEventType.CopySuccess:
                    sb.AppendLine("  ✅ СТАТУС: Вектор успешно скопирован в системный буфер Windows!");
                    sb.AppendLine($"  - Опубликованы форматы: {evt.Snapshot?.VectorFormatsSummary}.");
                    sb.AppendLine("  - Вставка Ctrl+V в Illustrator и другие приложения должна работать штатно.");
                    break;

                case CopyEventType.TestInjected:
                    sb.AppendLine("  ⚡ СТАТУС: Тестовый векторный объект (зелёная звезда) внедрён в буфер обмена.");
                    sb.AppendLine("  - Переключитесь в Adobe Illustrator и нажмите Ctrl+V для проверки движка вставки.");
                    break;

                case CopyEventType.ClipboardReset:
                    sb.AppendLine("  🔓 СТАТУС: Буфер обмена очищен, все системные замки сброшены.");
                    break;
            }

            sb.AppendLine();
            sb.AppendLine("▶ [2] ПРОЦЕСС ADOBE ILLUSTRATOR:");
            sb.AppendLine($"  PID:             {evt.IllustratorPid}");
            sb.AppendLine($"  Версия:          {(string.IsNullOrEmpty(evt.IllustratorVersion) ? "Не определена" : evt.IllustratorVersion)}");
            string gdiWarn = evt.GdiHandles > 7000 ? " [⚠️ КРИТИЧЕСКИ ВЫСОКИЙ]" : " [Норма < 7000]";
            sb.AppendLine($"  GDI Handles:     {evt.GdiHandles} / 10,000{gdiWarn}");
            sb.AppendLine($"  User Handles:    {evt.UserHandles} / 10,000");
            sb.AppendLine();

            sb.AppendLine("▶ [3] СОСТОЯНИЕ БУФЕРА WINDOWS:");
            if (evt.Snapshot != null)
            {
                sb.AppendLine($"  Владелец буфера: {evt.Snapshot.OwnerProcessName} (PID: {evt.Snapshot.OwnerPid})");
                sb.AppendLine($"  Заблокирован:    {(evt.Snapshot.IsCurrentlyLocked ? $"ДА, процессом '{evt.Snapshot.OpenProcessName}' (PID: {evt.Snapshot.OpenPid})" : "НЕТ (Свободен)")}");
                sb.AppendLine($"  Номер ревизии:   Sequence #{evt.Snapshot.SequenceNumber}");
                sb.AppendLine($"  Всего форматов:  {evt.Snapshot.Formats.Count}");
                sb.AppendLine($"  Общий объём:     {evt.Snapshot.TotalBytes / 1024.0:F1} КБ");
                sb.AppendLine();

                sb.AppendLine("▶ [4] ОПУБЛИКОВАННЫЕ ФОРМАТЫ:");
                if (evt.Snapshot.Formats.Count == 0)
                {
                    sb.AppendLine("  ⚠️ В БУФЕРЕ НЕТ НИ ОДНОГО ФОРМАТА (0 форматов, буфер пуст!)");
                }
                else
                {
                    for (int i = 0; i < evt.Snapshot.Formats.Count; i++)
                    {
                        var f = evt.Snapshot.Formats[i];
                        string marker = f.IsVector ? "⭐ [VECTOR] " : (f.IsAdobe ? "🔹 [ADOBE]  " : "   [SYSTEM] ");
                        sb.AppendLine($"  {i + 1,2}. {marker}{f.Name,-36} ({f.FormattedSize})");
                    }
                }
            }
            else
            {
                sb.AppendLine("  (Снимок буфера для данного события отсутствует)");
            }

            sb.AppendLine();
            sb.AppendLine("================================================================================");
            return sb.ToString();
        }

        private void OnInjectTestVector()
        {
            bool success = ClipboardInjector.InjectTestVector(this.Handle, out string error);
            var snapshot = _clipboardMonitor.CaptureSnapshot();

            if (success)
            {
                _eventTracker.RecordTestInjected(true, "Injected valid test vector star into Windows Clipboard. Switch to Adobe Illustrator now and press Ctrl+V to test if paste functions!", snapshot);
                MessageBox.Show("✅ Verified Test Vector Star injected into Windows Clipboard!\r\n\r\nNow switch to Adobe Illustrator and press Ctrl+V (Paste).\r\n\r\n• If Illustrator pastes the test star: Illustrator's Paste engine is WORKING healthy, proving the bug is 100% in the Copy engine.\r\n• If Illustrator Paste remains greyed out: Illustrator's clipboard parser is locked/hung.", "Vector Injection Test", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            else
            {
                _eventTracker.RecordTestInjected(false, $"Failed to inject test vector: {error}", snapshot);
                MessageBox.Show($"❌ Failed to inject test vector: {error}", "Injection Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void OnResetClipboard()
        {
            bool success = ClipboardInjector.EmergencyReset(this.Handle, out string error);
            if (success)
            {
                _eventTracker.RecordReset(true, "Clipboard successfully cleared and verified free.");
                MessageBox.Show("✅ Windows Clipboard has been emptied and all locks released.", "Clipboard Reset", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            else
            {
                _eventTracker.RecordReset(false, $"Failed to reset clipboard: {error}");
                MessageBox.Show($"❌ Could not reset clipboard: {error}", "Reset Error", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }

        private void OnExecuteRescueCopy()
        {
            try
            {
                string scriptPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "scripts", "AI_RescueCopy.jsx");
                if (!File.Exists(scriptPath))
                {
                    scriptPath = @"C:\_CODE\illustrator\AI-ClipboardDoctor\scripts\AI_RescueCopy.jsx";
                }

                if (!File.Exists(scriptPath))
                {
                    MessageBox.Show("Файл скрипта AI_RescueCopy.jsx не найден!", "Rescue Copy", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    return;
                }

                string aiExe = @"C:\Program Files\Adobe\Adobe Illustrator 2026\Support Files\Contents\Windows\Illustrator.exe";
                if (!File.Exists(aiExe) && _aiTracker.IsRunning)
                {
                    try
                    {
                        using var p = Process.GetProcessById(_aiTracker.Pid);
                        if (!string.IsNullOrEmpty(p.MainModule?.FileName))
                        {
                            aiExe = p.MainModule.FileName;
                        }
                    }
                    catch { }
                }

                if (File.Exists(aiExe))
                {
                    Process.Start(new ProcessStartInfo
                    {
                        FileName = aiExe,
                        Arguments = $"\"{scriptPath}\"",
                        UseShellExecute = true
                    });
                }
                else
                {
                    Process.Start("explorer.exe", $"/select,\"{scriptPath}\"");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"❌ Ошибка запуска Rescue Copy: {ex.Message}", "Rescue Copy", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void OnInspectNow()
        {
            var snapshot = _clipboardMonitor.CaptureSnapshot();
            _eventTracker.RecordManualInspect(snapshot);
        }

        private void OnExportReport()
        {
            using var sfd = new SaveFileDialog
            {
                Title = "Export Clipboard Diagnostics Report",
                Filter = "Text Diagnostic Log (*.txt)|*.txt|JSON Report (*.json)|*.json|CSV Log (*.csv)|*.csv",
                FileName = $"Illustrator_Clipboard_Report_{DateTime.Now:yyyyMMdd_HHmmss}.txt"
            };

            if (sfd.ShowDialog() == DialogResult.OK)
            {
                try
                {
                    if (sfd.FilterIndex == 1)
                    {
                        DiagnosticsExporter.ExportToText(sfd.FileName, _eventTracker, _aiTracker);
                    }
                    else if (sfd.FilterIndex == 2)
                    {
                        DiagnosticsExporter.ExportToJson(sfd.FileName, _eventTracker, _aiTracker);
                    }
                    else
                    {
                        DiagnosticsExporter.ExportToCsv(sfd.FileName, _eventTracker);
                    }

                    MessageBox.Show($"✅ Diagnostic report successfully exported to:\r\n{sfd.FileName}", "Export Completed", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                catch (Exception ex)
                {
                    MessageBox.Show($"❌ Failed to export report: {ex.Message}", "Export Failed", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }
        }

        private void OnClearEvents()
        {
            if (MessageBox.Show("Are you sure you want to clear the diagnostic event log?", "Confirm Clear", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
            {
                _eventTracker.ClearEvents();
                RefreshEventsList();
            }
        }

        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            try
            {
                _notifyIcon.Visible = false;
                _notifyIcon.Dispose();
                _keyboardHook.Dispose();
                _clipboardMonitor.Dispose();
                _eventTracker.Dispose();
                _aiTracker.Dispose();
            }
            catch { }

            base.OnFormClosing(e);
        }
    }
}
