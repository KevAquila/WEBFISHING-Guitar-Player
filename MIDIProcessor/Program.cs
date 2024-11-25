using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using System.Threading.Tasks;
using Melanchall.DryWetMidi.Smf;
using Melanchall.DryWetMidi.Smf.Interaction;
using Melanchall.DryWetMidi.Common;

namespace miditotxt
{
    class Program
    {
        public struct SoundEvent
        {
            public int timestamp;
            public List<byte> notes;
            public SoundEvent(int time, byte note)
            {
                notes = new List<byte>();
                timestamp = time;
                notes.Add(note);
            }
        }

        static async Task Main(string[] args)
        {
            string[] files = Directory.GetFiles("midi/", "*.mid");
            string statsFilePath = "conversion_stats.txt";

            // Create or overwrite the stats file
            await File.WriteAllTextAsync(statsFilePath, "Conversion Statistics\n" + new string('=', 50) + "\n");

            // Process MIDI files in parallel
            List<Task> tasks = new List<Task>();
            foreach (string file in files)
            {
                tasks.Add(Task.Run(() =>
                    ConvertMidiToText(file, file.Replace(".mid", "").Replace("midi/", "songs/"), statsFilePath)));
            }

            await Task.WhenAll(tasks);

            Console.WriteLine($"Conversion complete. Statistics written to {statsFilePath}");
        }

        public static void ConvertMidiToText(string midiFilePath, string textFilePath, string statsFilePath)
        {
            // Configure ReadingSettings to handle invalid events
            var readingSettings = new ReadingSettings
            {
                InvalidChannelEventParameterValuePolicy = InvalidChannelEventParameterValuePolicy.SnapToLimits
            };

            MidiFile midiFile;

            try
            {
                // Read the MIDI file with the custom settings
                midiFile = MidiFile.Read(midiFilePath, readingSettings);
            }
            catch (Exception ex)
            {
                // Log the error to the console or stats file and skip the problematic file
                Console.WriteLine($"Error reading file {Path.GetFileName(midiFilePath)}: {ex.Message}");
                lock (statsFilePath)
                {
                    File.AppendAllText(statsFilePath, $"Error reading file {Path.GetFileName(midiFilePath)}: {ex.Message}\n" + new string('-', 50) + "\n");
                }
                return;
            }

            var tempoMap = midiFile.GetTempoMap();
            List<SoundEvent> Song = new List<SoundEvent>();

            // Extract notes from MIDI file
            foreach (var n in midiFile.GetNotes())
            {
                int timestampNote = (n.TimeAs<MetricTimeSpan>(tempoMap).Minutes * 60 * 1000) +
                                    (n.TimeAs<MetricTimeSpan>(tempoMap).Seconds * 1000) +
                                    (n.TimeAs<MetricTimeSpan>(tempoMap).Milliseconds);

                if (Song.Count > 0)
                {
                    if (timestampNote <= Song[Song.Count - 1].timestamp + 30)
                    {
                        Song[Song.Count - 1].notes.Add(n.NoteNumber);
                    }
                    else
                    {
                        Song.Add(new SoundEvent(timestampNote, n.NoteNumber));
                    }
                }
                else
                {
                    Song.Add(new SoundEvent(timestampNote, n.NoteNumber));
                }
            }

            // Calculate optimal shift
            List<byte> allNotes = Song.SelectMany(s => s.notes).ToList();
            int optimalShift = CalculateOptimalShift(allNotes);

            int totalNotes = allNotes.Count;
            int omittedNotes = allNotes.Count(n => n + optimalShift < 40 || n + optimalShift > 79);

            float notesInRangePercentage = (float)(totalNotes - omittedNotes) / totalNotes * 100;

            // Ensure the directory exists
            string directory = Path.GetDirectoryName(textFilePath);
            if (!string.IsNullOrEmpty(directory))
            {
                Directory.CreateDirectory(directory);
            }

            // Ensure the Song list is not empty
            if (Song.Count == 0)
            {
                Console.WriteLine($"No valid notes found in {Path.GetFileName(midiFilePath)}. Skipping file.");

                lock (statsFilePath)
                {
                    File.AppendAllText(statsFilePath, $"File: {Path.GetFileName(midiFilePath)} - No valid notes found. Skipped.\n" + new string('-', 50) + "\n");
                }

                return;
            }

            // Apply shift and write to file
            using (StreamWriter writer = new StreamWriter(textFilePath))
            {
                int offsetNotes = Song[0].timestamp;

                foreach (var soundEvent in Song)
                {
                    List<int> shiftedNotes = soundEvent.notes
                        .Select(n => (int)n + optimalShift)
                        .Where(n => n >= 40 && n <= 79)
                        .ToList();

                    if (shiftedNotes.Any())
                    {
                        writer.Write($"{soundEvent.timestamp - offsetNotes}");
                        foreach (int note in shiftedNotes)
                        {
                            writer.Write($" {note}");
                        }
                        writer.WriteLine();
                    }
                }
            }

            // Write statistics to file
            lock (statsFilePath)
            {
                File.AppendAllText(statsFilePath, $"File: {Path.GetFileName(midiFilePath)}\n" +
                                                  $"Optimal shift: {optimalShift}\n" +
                                                  $"Total notes: {totalNotes}\n" +
                                                  $"Omitted notes: {omittedNotes} ({(float)omittedNotes / totalNotes:P2})\n" +
                                                  $"Notes in range after shift: {totalNotes - omittedNotes} ({notesInRangePercentage:F2}%)\n" +
                                                  new string('-', 50) + "\n");
            }

            // Copy files based on criteria
            string oneHundredFolder = "OneHundredPercent";
            string perfectFolder = "Perfect";

            if (notesInRangePercentage == 100.00f)
            {
                Directory.CreateDirectory(oneHundredFolder);
                string destinationFile = Path.Combine(oneHundredFolder, Path.GetFileName(textFilePath));
                File.Copy(textFilePath, destinationFile, true);

                if (optimalShift == 0 && omittedNotes == 0)
                {
                    Directory.CreateDirectory(perfectFolder);
                    string perfectDestinationFile = Path.Combine(perfectFolder, Path.GetFileName(textFilePath));
                    File.Copy(textFilePath, perfectDestinationFile, true);
                }
            }

            Console.WriteLine($"Converted: {Path.GetFileName(midiFilePath)}");
        }

        public class ShiftParameters
        {
            public double NoShiftBonus { get; set; } = 0.11;
            public double OctaveShiftBonus { get; set; } = 0.15;
            public double MaxShiftPenalty { get; set; } = 0.3;
            public double PlayableNoteWeight { get; set; } = 2.2;
        }

        private static int CalculateOptimalShift(List<byte> notes)
        {
            ShiftParameters parameters = new ShiftParameters();
            int bestShift = 0;
            int maxPlayableNotes = 0;
            double bestScore = double.MinValue;

            for (int shift = -127; shift <= 127; shift++)
            {
                int playableNotes = notes.Count(n => n + shift >= 40 && n + shift <= 79);

                // Calculate a normalized score between 0 and 1 for playable notes
                double normalizedPlayableScore = (double)playableNotes / notes.Count;

                // Start with the weighted normalized playable score
                double score = normalizedPlayableScore * parameters.PlayableNoteWeight;

                // Apply bonuses for preferred shifts
                if (shift == 0)
                {
                    score += parameters.NoShiftBonus;
                }
                else if (shift % 12 == 0)
                {
                    score += parameters.OctaveShiftBonus;
                }

                // Penalize larger shifts
                score -= Math.Abs(shift) * parameters.MaxShiftPenalty / 127.0;

                if (score > bestScore || (score == bestScore && Math.Abs(shift) < Math.Abs(bestShift)))
                {
                    bestScore = score;
                    maxPlayableNotes = playableNotes;
                    bestShift = shift;
                }
            }

            return bestShift;
        }
    }
}
