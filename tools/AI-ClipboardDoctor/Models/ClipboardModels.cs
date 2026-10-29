using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.Json.Serialization;

namespace AI_ClipboardDoctor.Models
{
    public class ClipboardFormatItem
    {
        public uint FormatId { get; set; }
        public string Name { get; set; } = string.Empty;
        public long SizeBytes { get; set; }
        public bool IsVector { get; set; }
        public bool IsAdobe { get; set; }

        public string FormattedSize => SizeBytes switch
        {
            < 0 => "Unknown",
            0 => "0 B",
            < 1024 => $"{SizeBytes} B",
            < 1024 * 1024 => $"{SizeBytes / 1024.0:F1} KB",
            _ => $"{SizeBytes / (1024.0 * 1024.0):F2} MB"
        };
    }

    public class ClipboardSnapshot
    {
        public DateTime Timestamp { get; set; } = DateTime.Now;
        public uint SequenceNumber { get; set; }

        [JsonIgnore]
        public IntPtr OwnerHwnd { get; set; }
        public string OwnerHwndHex => $"0x{OwnerHwnd.ToInt64():X}";

        public int OwnerPid { get; set; }
        public string OwnerProcessName { get; set; } = "Unknown";

        [JsonIgnore]
        public IntPtr OpenHwnd { get; set; }
        public string OpenHwndHex => $"0x{OpenHwnd.ToInt64():X}";

        public int OpenPid { get; set; }
        public string OpenProcessName { get; set; } = "None";
        public bool IsCurrentlyLocked => OpenHwnd != IntPtr.Zero;

        public List<ClipboardFormatItem> Formats { get; set; } = new();

        public bool HasVector => Formats.Any(f => f.IsVector);
        public bool HasText => Formats.Any(f => f.Name.Contains("TEXT", StringComparison.OrdinalIgnoreCase));
        public bool HasBitmap => Formats.Any(f => f.Name.Contains("DIB", StringComparison.OrdinalIgnoreCase) || f.Name.Contains("BITMAP", StringComparison.OrdinalIgnoreCase));
        public bool HasAdobePrivate => Formats.Any(f => f.Name.Contains("Illustrator", StringComparison.OrdinalIgnoreCase));
        public bool HasPdf => Formats.Any(f => f.Name.Contains("PDF", StringComparison.OrdinalIgnoreCase));
        public bool HasSvg => Formats.Any(f => f.Name.Contains("SVG", StringComparison.OrdinalIgnoreCase));
        public bool HasAicb => Formats.Any(f => f.Name.Equals("AICB", StringComparison.OrdinalIgnoreCase));

        public long TotalBytes => Formats.Where(f => f.SizeBytes > 0).Sum(f => f.SizeBytes);

        public string VectorFormatsSummary
        {
            get
            {
                var list = new List<string>();
                if (HasAdobePrivate) list.Add("AI-Private");
                if (HasAicb) list.Add("AICB");
                if (HasPdf) list.Add("PDF");
                if (HasSvg) list.Add("SVG");
                if (Formats.Any(f => f.Name.Equals("CF_ENHMETAFILE", StringComparison.OrdinalIgnoreCase))) list.Add("EMF");
                if (list.Count > 0) return string.Join(" + ", list);
                if (HasText) return "Text Only";
                if (Formats.Count == 0) return "Empty (0 Formats)";
                return "None";
            }
        }
    }

    public enum EventSeverity
    {
        Success,
        Warning,
        Error,
        Info
    }

    public enum CopyEventType
    {
        CtrlCPressed,
        CopySuccess,
        TextCopySuccess,
        CopyFailNoUpdate,
        CopyVectorMissing,
        ClipboardLocked,
        TestInjected,
        ManualInspect,
        ClipboardReset
    }

    public class DiagnosticEvent
    {
        public int Id { get; set; }
        public DateTime Timestamp { get; set; } = DateTime.Now;
        public CopyEventType Type { get; set; }
        public EventSeverity Severity { get; set; }
        public long LatencyMs { get; set; } = -1;
        public int IllustratorPid { get; set; }
        public string IllustratorVersion { get; set; } = string.Empty;
        public int GdiHandles { get; set; }
        public int UserHandles { get; set; }
        public ClipboardSnapshot? Snapshot { get; set; }
        public string Summary { get; set; } = string.Empty;
        public string Details { get; set; } = string.Empty;

        public string TypeBadge => Type switch
        {
            CopyEventType.CtrlCPressed => "CTRL+C",
            CopyEventType.CopySuccess => "SUCCESS",
            CopyEventType.TextCopySuccess => "TEXT COPY",
            CopyEventType.CopyFailNoUpdate => "FAIL: NO UPDATE",
            CopyEventType.CopyVectorMissing => "WARN: NO VECTORS",
            CopyEventType.ClipboardLocked => "LOCKED",
            CopyEventType.TestInjected => "TEST INJECT",
            CopyEventType.ManualInspect => "INSPECT",
            CopyEventType.ClipboardReset => "RESET",
            _ => "EVENT"
        };
    }
}
