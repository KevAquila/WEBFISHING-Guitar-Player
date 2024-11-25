using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
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

        static void Main(string[] args)
        {
            string[] files = Directory.GetFiles("midi/", "*.mid");
            string statsFilePath = "conversion_stats.txt";

            // Create or overwrite the stats file
            using (StreamWriter statsWriter = new StreamWriter(statsFilePath, false))
            {
                statsWriter.WriteLine("Conversion Statistics");
                statsWriter.WriteLine(new string('=', 50));
            }

            foreach (string file in files)
            {
                ConvertMidiToText(file, file.Replace(".mid", "").Replace("midi/", "songs/"), statsFilePath);
            }
            Console.WriteLine($"Conversion complete. Statistics written to {statsFilePath}");
            Console.Read();
        }
        public static void ConvertMidiToText(string midiFilePath, string textFilePath, string statsFilePath)
        {
            // Configure ReadingSettings to handle invalid events
            var readingSettings = new ReadingSettings
            {
                // Skip events with invalid parameter values
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

                // Optionally log the error to the stats file
                using (StreamWriter statsWriter = new StreamWriter(statsFilePath, true))
                {
                    statsWriter.WriteLine($"Error reading file {Path.GetFileName(midiFilePath)}: {ex.Message}");
                    statsWriter.WriteLine(new string('-', 50));
                }
                return; // Skip further processing for this file
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

                // Optionally log to stats file
                using (StreamWriter statsWriter = new StreamWriter(statsFilePath, true))
                {
                    statsWriter.WriteLine($"File: {Path.GetFileName(midiFilePath)} - No valid notes found. Skipped.");
                    statsWriter.WriteLine(new string('-', 50));
                }

                return; // Skip further processing
            }

            // Safely access the first timestamp
            int offsetNotes = Song[0].timestamp;

            // Apply shift and write to file
            using (StreamWriter writer = new StreamWriter(textFilePath))
            {
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
            using (StreamWriter statsWriter = new StreamWriter(statsFilePath, true))
            {
                statsWriter.WriteLine($"File: {Path.GetFileName(midiFilePath)}");
                statsWriter.WriteLine($"Optimal shift: {optimalShift}");
                statsWriter.WriteLine($"Total notes: {totalNotes}");
                statsWriter.WriteLine($"Omitted notes: {omittedNotes} ({(float)omittedNotes / totalNotes:P2})");
                statsWriter.WriteLine($"Notes in range after shift: {totalNotes - omittedNotes} ({(float)(totalNotes - omittedNotes) / totalNotes:P2})");
                statsWriter.WriteLine(new string('-', 50));
            }

            // Print a simple progress message to console
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
