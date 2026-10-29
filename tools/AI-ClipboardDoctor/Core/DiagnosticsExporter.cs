using System;
using System.IO;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using AI_ClipboardDoctor.Models;

namespace AI_ClipboardDoctor.Core
{
    public class IntPtrJsonConverter : JsonConverter<IntPtr>
    {
        public override IntPtr Read(ref Utf8JsonReader reader, Type typeToConvert, JsonSerializerOptions options)
        {
            if (reader.TokenType == JsonTokenType.Number)
            {
                return new IntPtr(reader.GetInt64());
            }
            if (reader.TokenType == JsonTokenType.String && long.TryParse(reader.GetString(), out long val))
            {
                return new IntPtr(val);
            }
            return IntPtr.Zero;
        }

        public override void Write(Utf8JsonWriter writer, IntPtr value, JsonSerializerOptions options)
        {
            writer.WriteStringValue($"0x{value.ToInt64():X}");
        }
    }

    public static class DiagnosticsExporter
    {
        public static void ExportToJson(string filePath, CopyEventTracker tracker, IllustratorTracker aiTracker)
        {
            var report = new
            {
                Tool = "Adobe Illustrator Clipboard Doctor & Sentinel",
                Version = "1.0.2",
                ExportTime = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"),
                System = new
                {
                    Environment.OSVersion.VersionString,
                    Is64BitOperatingSystem = Environment.Is64BitOperatingSystem,
                    ProcessorCount = Environment.ProcessorCount,
                    MachineName = Environment.MachineName
                },
                Illustrator = new
                {
                    aiTracker.IsRunning,
                    aiTracker.Pid,
                    aiTracker.Version,
                    aiTracker.GdiHandles,
                    aiTracker.UserHandles,
                    aiTracker.WorkingSetMb,
                    aiTracker.MainWindowTitle
                },
                Statistics = new
                {
                    tracker.TotalCopiesAttempted,
                    tracker.SuccessfulVectorCopies,
                    tracker.VectorMissingCopies,
                    tracker.FailedNoUpdateCopies,
                    tracker.ClipboardLockCollisions,
                    SuccessRate = tracker.TotalCopiesAttempted > 0 
                        ? $"{(double)tracker.SuccessfulVectorCopies / tracker.TotalCopiesAttempted * 100:F1}%" 
                        : "N/A"
                },
                Events = tracker.Events
            };

            var options = new JsonSerializerOptions
            {
                WriteIndented = true,
                Converters =
                {
                    new JsonStringEnumConverter(),
                    new IntPtrJsonConverter()
                }
            };

            string json = JsonSerializer.Serialize(report, options);
            File.WriteAllText(filePath, json, Encoding.UTF8);
        }

        public static void ExportToText(string filePath, CopyEventTracker tracker, IllustratorTracker aiTracker)
        {
            var sb = new StringBuilder();
            sb.AppendLine("================================================================================");
            sb.AppendLine("       ADOBE ILLUSTRATOR CLIPBOARD DOCTOR — DIAGNOSTIC AUDIT LOG");
            sb.AppendLine($"  Экспорт отчета: {DateTime.Now:yyyy-MM-dd HH:mm:ss.fff}");
            sb.AppendLine("================================================================================");
            sb.AppendLine();
            sb.AppendLine("▶ [1] СВОДКА СИСТЕМЫ И ПРОЦЕССА:");
            sb.AppendLine($"  ОС:              {Environment.OSVersion.VersionString} ({(Environment.Is64BitOperatingSystem ? "64-bit" : "32-bit")})");
            sb.AppendLine($"  Компьютер:       {Environment.MachineName}");
            sb.AppendLine($"  Illustrator:     {(aiTracker.IsRunning ? $"Запущен (PID: {aiTracker.Pid}, Версия: {aiTracker.Version})" : "Не запущен")}");
            sb.AppendLine($"  Дескрипторы:     GDI: {aiTracker.GdiHandles} / 10,000 | User: {aiTracker.UserHandles} / 10,000");
            sb.AppendLine($"  Память процесса: {aiTracker.WorkingSetMb} МБ");
            sb.AppendLine();
            sb.AppendLine("▶ [2] СТАТИСТИКА БУФЕРА ОБМЕНА:");
            sb.AppendLine($"  Всего копирований:    {tracker.TotalCopiesAttempted}");
            sb.AppendLine($"  Успешных векторов:    {tracker.SuccessfulVectorCopies}");
            sb.AppendLine($"  Сбоев без векторов:   {tracker.VectorMissingCopies}");
            sb.AppendLine($"  Сбоев без обновления: {tracker.FailedNoUpdateCopies}");
            sb.AppendLine($"  Блокировок буфера:    {tracker.ClipboardLockCollisions}");
            sb.AppendLine($"  Процент успеха:       {(tracker.TotalCopiesAttempted > 0 ? $"{(double)tracker.SuccessfulVectorCopies / tracker.TotalCopiesAttempted * 100:F1}%" : "N/A")}");
            sb.AppendLine();
            sb.AppendLine("================================================================================");
            sb.AppendLine("                         ХРОНОЛОГИЯ СОБЫТИЙ");
            sb.AppendLine("================================================================================");

            lock (tracker.Events)
            {
                foreach (var evt in tracker.Events)
                {
                    sb.AppendLine();
                    sb.AppendLine("================================================================================");
                    sb.AppendLine($"  СОБЫТИЕ #{evt.Id} — [{evt.TypeBadge}] — {evt.Severity.ToString().ToUpperInvariant()}");
                    sb.AppendLine($"  Время: {evt.Timestamp:yyyy-MM-dd HH:mm:ss.fff}  |  Задержка реакции: {(evt.LatencyMs >= 0 ? $"{evt.LatencyMs} мс" : "N/A")}");
                    sb.AppendLine("================================================================================");
                    sb.AppendLine();
                    sb.AppendLine("▶ [1] ДИАГНОЗ И АНАЛИЗ СБОЯ:");
                    sb.AppendLine($"  Сводка: {evt.Summary}");
                    sb.AppendLine($"  Детали: {evt.Details}");
                    sb.AppendLine();
                    sb.AppendLine("▶ [2] ПРОЦЕСС ADOBE ILLUSTRATOR:");
                    sb.AppendLine($"  PID:             {evt.IllustratorPid}");
                    sb.AppendLine($"  Версия:          {evt.IllustratorVersion}");
                    sb.AppendLine($"  GDI Handles:     {evt.GdiHandles} / 10,000");
                    sb.AppendLine($"  User Handles:    {evt.UserHandles} / 10,000");
                    sb.AppendLine();
                    sb.AppendLine("▶ [3] СОСТОЯНИЕ БУФЕРА WINDOWS:");
                    if (evt.Snapshot != null)
                    {
                        sb.AppendLine($"  Владелец буфера: {evt.Snapshot.OwnerProcessName} (PID: {evt.Snapshot.OwnerPid}, HWND: {evt.Snapshot.OwnerHwndHex})");
                        sb.AppendLine($"  Заблокирован:    {(evt.Snapshot.IsCurrentlyLocked ? $"ДА ({evt.Snapshot.OpenProcessName}, PID: {evt.Snapshot.OpenPid})" : "НЕТ (Свободен)")}");
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
                            foreach (var fmt in evt.Snapshot.Formats)
                            {
                                string tag = fmt.IsVector ? "[ВЕКТОР]" : (fmt.IsAdobe ? "[ADOBE]" : "[ДАННЫЕ]");
                                sb.AppendLine($"  - {tag,-8} ID: 0x{fmt.FormatId:X4} ({fmt.FormatId,-5}) | {fmt.Name,-35} | {fmt.FormattedSize}");
                            }
                        }
                    }
                    else
                    {
                        sb.AppendLine("  (Снимок буфера для данного события отсутствует)");
                    }
                }
            }

            File.WriteAllText(filePath, sb.ToString(), Encoding.UTF8);
        }

        public static void ExportToCsv(string filePath, CopyEventTracker tracker)
        {
            var sb = new StringBuilder();
            sb.AppendLine("ID,Timestamp,Type,Severity,LatencyMs,IllustratorPID,GdiHandles,UserHandles,Summary,Details,FormatsCount,VectorFormats,TotalBytes");

            lock (tracker.Events)
            {
                foreach (var evt in tracker.Events)
                {
                    string id = evt.Id.ToString();
                    string time = evt.Timestamp.ToString("yyyy-MM-dd HH:mm:ss.fff");
                    string type = EscapeCsv(evt.TypeBadge);
                    string severity = evt.Severity.ToString();
                    string latency = evt.LatencyMs >= 0 ? evt.LatencyMs.ToString() : "";
                    string pid = evt.IllustratorPid.ToString();
                    string gdi = evt.GdiHandles.ToString();
                    string user = evt.UserHandles.ToString();
                    string summary = EscapeCsv(evt.Summary);
                    string details = EscapeCsv(evt.Details);
                    string formatsCount = evt.Snapshot != null ? evt.Snapshot.Formats.Count.ToString() : "0";
                    string vectorSummary = evt.Snapshot != null ? EscapeCsv(evt.Snapshot.VectorFormatsSummary) : "N/A";
                    string totalBytes = evt.Snapshot != null ? evt.Snapshot.TotalBytes.ToString() : "0";

                    sb.AppendLine($"{id},{time},{type},{severity},{latency},{pid},{gdi},{user},{summary},{details},{formatsCount},{vectorSummary},{totalBytes}");
                }
            }

            File.WriteAllText(filePath, sb.ToString(), Encoding.UTF8);
        }

        private static string EscapeCsv(string str)
        {
            if (string.IsNullOrEmpty(str)) return "\"\"";
            return "\"" + str.Replace("\"", "\"\"") + "\"";
        }
    }
}
