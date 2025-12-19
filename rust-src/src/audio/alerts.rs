//! Audio alert system - plays beeps/alerts for critical and emergency conditions

use crate::data::models::AlertSeverity;
use std::process::Command;
use std::thread;
use std::time::Duration;

/// Audio alerter for critical/emergency notifications
#[derive(Clone)]
pub struct AudioAlerter {
    enabled: bool,
}

impl AudioAlerter {
    /// Create new audio alerter
    pub fn new(enabled: bool) -> Self {
        Self { enabled }
    }

    /// Play alert sound based on severity
    /// This spawns in a background thread and doesn't block
    pub fn play_alert(&self, severity: AlertSeverity) {
        if !self.enabled {
            return;
        }

        match severity {
            AlertSeverity::Critical => {
                // Critical: 3 short beeps
                Self::spawn_alert_thread(AlertPattern::CriticalBeeps);
            }
            AlertSeverity::Emergency => {
                // Emergency: continuous alarm (3 second burst)
                Self::spawn_alert_thread(AlertPattern::EmergencyAlarm);
            }
            AlertSeverity::Warning => {
                // Warning: 2 medium beeps
                Self::spawn_alert_thread(AlertPattern::WarningBeeps);
            }
            AlertSeverity::Notice | AlertSeverity::Info => {
                // Suppress audio for lower severities
                // Can be customized if needed
            }
        }
    }

    /// Spawn audio in background thread
    fn spawn_alert_thread(pattern: AlertPattern) {
        thread::spawn(move || {
            if let Err(e) = pattern.play() {
                log::warn!("Failed to play audio alert: {}", e);
            }
        });
    }
}

/// Audio alert patterns
enum AlertPattern {
    CriticalBeeps,   // 3x beeps at 1000Hz
    EmergencyAlarm,  // Continuous tone for 3 seconds
    WarningBeeps,    // 2x beeps at 800Hz
}

impl AlertPattern {
    /// Play the alert pattern using platform-specific audio
    fn play(&self) -> Result<(), String> {
        match self {
            AlertPattern::CriticalBeeps => {
                // 3 short beeps: 100ms on, 100ms off pattern
                for i in 0..3 {
                    Self::system_beep(1000, 100)?;
                    if i < 2 {
                        thread::sleep(Duration::from_millis(100));
                    }
                }
                Ok(())
            }
            AlertPattern::EmergencyAlarm => {
                // Continuous 800Hz tone for 3 seconds
                Self::system_beep(800, 3000)?;
                Ok(())
            }
            AlertPattern::WarningBeeps => {
                // 2 beeps at 800Hz: 150ms on, 100ms off
                for i in 0..2 {
                    Self::system_beep(800, 150)?;
                    if i < 1 {
                        thread::sleep(Duration::from_millis(100));
                    }
                }
                Ok(())
            }
        }
    }

    /// Generate system beep at frequency and duration
    /// Uses platform-specific commands (beep on Linux, afplay on macOS, speaker on Windows)
    fn system_beep(frequency: u32, duration_ms: u64) -> Result<(), String> {
        #[cfg(target_os = "linux")]
        {
            // Try multiple audio output methods on Linux
            // Method 1: Use 'beep' command if available
            if let Ok(output) = Command::new("beep")
                .arg("-f").arg(frequency.to_string())
                .arg("-l").arg(duration_ms.to_string())
                .output()
            {
                if output.status.success() {
                    return Ok(());
                }
            }

            // Method 2: Try speaker-test if available (ALSA)
            if Command::new("speaker-test")
                .arg("-t").arg("sine")
                .arg("-f").arg(frequency.to_string())
                .arg("-l").arg("1")
                .output()
                .is_ok()
            {
                // Simulate audio playback by sleeping
                thread::sleep(Duration::from_millis(duration_ms));
                return Ok(());
            }

            // Method 3: Fallback to simple printf bell character (works everywhere)
            print!("\x07");
            thread::sleep(Duration::from_millis(duration_ms));
            Ok(())
        }

        #[cfg(target_os = "macos")]
        {
            // macOS: Use afplay with generated sine wave or system alert
            // Create a simple sine wave WAV and play it
            let wav_data = Self::generate_sine_wave(frequency, duration_ms);
            let temp_file = format!("/tmp/hamclock_alert_{}.wav", std::process::id());

            // Write WAV file
            if let Ok(_) = std::fs::write(&temp_file, wav_data) {
                let result = Command::new("afplay")
                    .arg(&temp_file)
                    .output()
                    .is_ok();

                // Clean up temp file
                let _ = std::fs::remove_file(&temp_file);

                if result {
                    return Ok(());
                }
            }

            // Fallback: Use system beep
            print!("\x07");
            thread::sleep(Duration::from_millis(duration_ms));
            Ok(())
        }

        #[cfg(target_os = "windows")]
        {
            // Windows: Use Beep API via PowerShell or direct Windows API
            // Simplified: Use print beep character multiple times
            let beep_count = ((duration_ms / 200) + 1).min(10); // Cap at 10 beeps
            for _ in 0..beep_count {
                print!("\x07");
                thread::sleep(Duration::from_millis(100));
            }
            Ok(())
        }

        #[cfg(not(any(target_os = "linux", target_os = "macos", target_os = "windows")))]
        {
            // Unknown platform: Use ASCII bell character
            print!("\x07");
            thread::sleep(Duration::from_millis(duration_ms));
            Ok(())
        }
    }

    /// Generate a simple WAV file with sine wave (for macOS/afplay)
    fn generate_sine_wave(frequency: u32, duration_ms: u64) -> Vec<u8> {
        const SAMPLE_RATE: u32 = 44100;
        const AMPLITUDE: f32 = 16000.0;

        let samples: usize = ((SAMPLE_RATE as u64 * duration_ms) / 1000) as usize;
        let mut audio_data: Vec<i16> = Vec::with_capacity(samples);

        for i in 0..samples {
            let t = i as f32 / SAMPLE_RATE as f32;
            let sample = (2.0 * std::f32::consts::PI * frequency as f32 * t).sin() * AMPLITUDE;
            audio_data.push(sample as i16);
        }

        // Create minimal WAV header
        let mut wav = Vec::new();

        // RIFF header
        wav.extend_from_slice(b"RIFF");
        let file_size = 36 + (audio_data.len() * 2) as u32;
        wav.extend_from_slice(&file_size.to_le_bytes());
        wav.extend_from_slice(b"WAVE");

        // fmt subchunk
        wav.extend_from_slice(b"fmt ");
        wav.extend_from_slice(&16u32.to_le_bytes()); // Subchunk1Size
        wav.extend_from_slice(&1u16.to_le_bytes());  // AudioFormat (PCM)
        wav.extend_from_slice(&1u16.to_le_bytes());  // NumChannels (mono)
        wav.extend_from_slice(&SAMPLE_RATE.to_le_bytes()); // SampleRate
        let byte_rate = SAMPLE_RATE * 2; // SampleRate * NumChannels * BytesPerSample
        wav.extend_from_slice(&byte_rate.to_le_bytes());
        wav.extend_from_slice(&2u16.to_le_bytes()); // BlockAlign
        wav.extend_from_slice(&16u16.to_le_bytes()); // BitsPerSample

        // data subchunk
        wav.extend_from_slice(b"data");
        wav.extend_from_slice(&(audio_data.len() as u32 * 2).to_le_bytes());

        // Audio samples (little-endian i16)
        for sample in audio_data {
            wav.extend_from_slice(&sample.to_le_bytes());
        }

        wav
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_audio_alerter_creation() {
        let alerter = AudioAlerter::new(true);
        assert_eq!(alerter.enabled, true);

        let disabled = AudioAlerter::new(false);
        assert_eq!(disabled.enabled, false);
    }

    #[test]
    fn test_critical_alert_pattern() {
        // Should not panic
        let pattern = AlertPattern::CriticalBeeps;
        let _ = pattern.play();
    }

    #[test]
    fn test_wav_generation() {
        let wav = AlertPattern::generate_sine_wave(440, 100);

        // Verify basic WAV structure
        assert!(wav.len() > 44); // Minimum WAV file size
        assert_eq!(&wav[0..4], b"RIFF");
        assert_eq!(&wav[8..12], b"WAVE");
        assert_eq!(&wav[12..16], b"fmt ");
        assert_eq!(&wav[36..40], b"data");
    }
}
