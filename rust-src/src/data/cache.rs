//! Response caching with TTL (Time-To-Live) support

use crate::data::models::{SpaceWeather, Forecast, DxSpot, SatelliteData};
use chrono::{DateTime, Duration, Utc};
use std::sync::{Arc, Mutex};

/// Cache entry with TTL
#[derive(Clone, Debug)]
pub struct CacheEntry<T: Clone> {
    data: T,
    created_at: DateTime<Utc>,
    ttl: Duration,
}

impl<T: Clone> CacheEntry<T> {
    /// Create a new cache entry with TTL
    pub fn new(data: T, ttl: Duration) -> Self {
        Self {
            data,
            created_at: Utc::now(),
            ttl,
        }
    }

    /// Check if cache entry is still valid
    pub fn is_valid(&self) -> bool {
        let age = Utc::now() - self.created_at;
        age < self.ttl
    }

    /// Get the remaining TTL in seconds (0 if expired)
    pub fn remaining_ttl_secs(&self) -> i64 {
        let age = Utc::now() - self.created_at;
        let remaining = self.ttl - age;
        remaining.num_seconds().max(0)
    }

    /// Get the cached data if still valid
    pub fn get(&self) -> Option<T> {
        if self.is_valid() {
            Some(self.data.clone())
        } else {
            None
        }
    }
}

/// Response cache for all data types
pub struct ResponseCache {
    space_weather: Arc<Mutex<Option<CacheEntry<SpaceWeather>>>>,
    forecast: Arc<Mutex<Option<CacheEntry<Vec<Forecast>>>>>,
    dx_spots: Arc<Mutex<Option<CacheEntry<Vec<DxSpot>>>>>,
    satellites: Arc<Mutex<Option<CacheEntry<Vec<SatelliteData>>>>>,
}

impl ResponseCache {
    /// Create a new response cache
    pub fn new() -> Self {
        Self {
            space_weather: Arc::new(Mutex::new(None)),
            forecast: Arc::new(Mutex::new(None)),
            dx_spots: Arc::new(Mutex::new(None)),
            satellites: Arc::new(Mutex::new(None)),
        }
    }

    /// Cache space weather data with default TTL of 30 minutes
    pub fn cache_space_weather(&self, data: SpaceWeather) {
        let ttl = Duration::minutes(30);
        let entry = CacheEntry::new(data, ttl);
        *self.space_weather.lock().unwrap() = Some(entry);
        log::debug!("Cached space weather (TTL: 30 minutes)");
    }

    /// Get cached space weather if available and valid
    pub fn get_space_weather(&self) -> Option<SpaceWeather> {
        let cache = self.space_weather.lock().unwrap();
        if let Some(entry) = cache.as_ref() {
            if let Some(data) = entry.get() {
                log::debug!("Cache HIT: space weather (remaining: {}s)", entry.remaining_ttl_secs());
                return Some(data);
            } else {
                log::debug!("Cache EXPIRED: space weather");
            }
        }
        log::debug!("Cache MISS: space weather");
        None
    }

    /// Cache forecast data with default TTL of 2 hours
    pub fn cache_forecast(&self, data: Vec<Forecast>) {
        let ttl = Duration::hours(2);
        let entry = CacheEntry::new(data, ttl);
        *self.forecast.lock().unwrap() = Some(entry);
        log::debug!("Cached forecast (TTL: 2 hours)");
    }

    /// Get cached forecast if available and valid
    pub fn get_forecast(&self) -> Option<Vec<Forecast>> {
        let cache = self.forecast.lock().unwrap();
        if let Some(entry) = cache.as_ref() {
            if let Some(data) = entry.get() {
                log::debug!("Cache HIT: forecast (remaining: {}s)", entry.remaining_ttl_secs());
                return Some(data);
            } else {
                log::debug!("Cache EXPIRED: forecast");
            }
        }
        log::debug!("Cache MISS: forecast");
        None
    }

    /// Cache DX spots with default TTL of 10 minutes
    pub fn cache_dx_spots(&self, data: Vec<DxSpot>) {
        let ttl = Duration::minutes(10);
        let entry = CacheEntry::new(data, ttl);
        *self.dx_spots.lock().unwrap() = Some(entry);
        log::debug!("Cached DX spots (TTL: 10 minutes)");
    }

    /// Get cached DX spots if available and valid
    pub fn get_dx_spots(&self) -> Option<Vec<DxSpot>> {
        let cache = self.dx_spots.lock().unwrap();
        if let Some(entry) = cache.as_ref() {
            if let Some(data) = entry.get() {
                log::debug!("Cache HIT: DX spots (remaining: {}s)", entry.remaining_ttl_secs());
                return Some(data);
            } else {
                log::debug!("Cache EXPIRED: DX spots");
            }
        }
        log::debug!("Cache MISS: DX spots");
        None
    }

    /// Cache satellite data with default TTL of 15 minutes
    pub fn cache_satellites(&self, data: Vec<SatelliteData>) {
        let ttl = Duration::minutes(15);
        let entry = CacheEntry::new(data, ttl);
        *self.satellites.lock().unwrap() = Some(entry);
        log::debug!("Cached satellite data (TTL: 15 minutes)");
    }

    /// Get cached satellite data if available and valid
    pub fn get_satellites(&self) -> Option<Vec<SatelliteData>> {
        let cache = self.satellites.lock().unwrap();
        if let Some(entry) = cache.as_ref() {
            if let Some(data) = entry.get() {
                log::debug!("Cache HIT: satellites (remaining: {}s)", entry.remaining_ttl_secs());
                return Some(data);
            } else {
                log::debug!("Cache EXPIRED: satellites");
            }
        }
        log::debug!("Cache MISS: satellites");
        None
    }

    /// Clear all caches
    pub fn clear_all(&self) {
        *self.space_weather.lock().unwrap() = None;
        *self.forecast.lock().unwrap() = None;
        *self.dx_spots.lock().unwrap() = None;
        *self.satellites.lock().unwrap() = None;
        log::info!("All caches cleared");
    }

    /// Get cache statistics
    pub fn get_stats(&self) -> CacheStats {
        let sw_valid = self.space_weather
            .lock()
            .unwrap()
            .as_ref()
            .map(|e| e.is_valid())
            .unwrap_or(false);
        let fc_valid = self.forecast
            .lock()
            .unwrap()
            .as_ref()
            .map(|e| e.is_valid())
            .unwrap_or(false);
        let dx_valid = self.dx_spots
            .lock()
            .unwrap()
            .as_ref()
            .map(|e| e.is_valid())
            .unwrap_or(false);
        let sat_valid = self.satellites
            .lock()
            .unwrap()
            .as_ref()
            .map(|e| e.is_valid())
            .unwrap_or(false);

        CacheStats {
            space_weather_cached: sw_valid,
            forecast_cached: fc_valid,
            dx_spots_cached: dx_valid,
            satellites_cached: sat_valid,
            total_cached: (sw_valid as u32) + (fc_valid as u32) + (dx_valid as u32) + (sat_valid as u32),
        }
    }
}

impl Default for ResponseCache {
    fn default() -> Self {
        Self::new()
    }
}

/// Cache statistics
#[derive(Debug, Clone)]
pub struct CacheStats {
    pub space_weather_cached: bool,
    pub forecast_cached: bool,
    pub dx_spots_cached: bool,
    pub satellites_cached: bool,
    pub total_cached: u32,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_cache_entry_ttl() {
        let data = 42;
        let entry = CacheEntry::new(data, Duration::seconds(1));

        assert!(entry.is_valid());
        assert_eq!(entry.get(), Some(42));

        // Simulate passage of time
        std::thread::sleep(std::time::Duration::from_millis(1100));

        assert!(!entry.is_valid());
        assert_eq!(entry.get(), None);
    }

    #[test]
    fn test_response_cache() {
        let cache = ResponseCache::new();

        // Initially empty
        assert!(cache.get_space_weather().is_none());

        // Cache some data
        let weather = SpaceWeather {
            kp: 5.0,
            a: 20,
            ap: 20,
            flux: 85,
            updated: Utc::now(),
        };
        cache.cache_space_weather(weather.clone());

        // Should be retrievable
        assert!(cache.get_space_weather().is_some());

        // Clear cache
        cache.clear_all();
        assert!(cache.get_space_weather().is_none());
    }

    #[test]
    fn test_cache_stats() {
        let cache = ResponseCache::new();

        let stats = cache.get_stats();
        assert_eq!(stats.total_cached, 0);

        let weather = SpaceWeather {
            kp: 5.0,
            a: 20,
            ap: 20,
            flux: 85,
            updated: Utc::now(),
        };
        cache.cache_space_weather(weather);

        let stats = cache.get_stats();
        assert_eq!(stats.total_cached, 1);
        assert!(stats.space_weather_cached);
    }
}
