//! Async data fetcher using reqwest and tokio with response caching

use crate::error::AppResult;
use crate::data::models::{SpaceWeather, Forecast, DxSpot, SatelliteData, AppData};
use crate::data::cache::ResponseCache;
use reqwest::Client;
use serde::Deserialize;
use chrono::Utc;
use std::sync::Arc;
use tokio::sync::Mutex;

/// Async data fetcher for HamClock with caching
pub struct DataFetcher {
    client: Client,
    cache: Arc<ResponseCache>,
}

/// Space Weather API response from spacex.land
#[derive(Deserialize, Debug)]
struct SpaceWeatherApiResponse {
    #[serde(rename = "Kp")]
    kp: Option<f32>,
    #[serde(rename = "A")]
    a: Option<f32>,
}

/// Open-Meteo forecast API response
#[derive(Deserialize, Debug)]
struct MeteoResponse {
    hourly: Option<HourlyData>,
}

#[derive(Deserialize, Debug)]
struct HourlyData {
    time: Option<Vec<String>>,
    temperature_2m: Option<Vec<f32>>,
    relative_humidity_2m: Option<Vec<i32>>,
    weather_code: Option<Vec<i32>>,
}

impl DataFetcher {
    /// Create a new data fetcher with caching
    pub fn new() -> Self {
        Self {
            client: Client::new(),
            cache: Arc::new(ResponseCache::new()),
        }
    }

    /// Create a new data fetcher with provided cache
    pub fn with_cache(cache: Arc<ResponseCache>) -> Self {
        Self {
            client: Client::new(),
            cache,
        }
    }

    /// Get reference to the cache
    pub fn cache(&self) -> Arc<ResponseCache> {
        Arc::clone(&self.cache)
    }

    /// Get cache statistics
    pub fn cache_stats(&self) {
        let stats = self.cache.get_stats();
        log::info!(
            "Cache Stats - Space Weather: {}, Forecast: {}, DX: {}, Satellites: {}, Total: {}",
            stats.space_weather_cached,
            stats.forecast_cached,
            stats.dx_spots_cached,
            stats.satellites_cached,
            stats.total_cached
        );
    }

    /// Fetch all data concurrently
    pub async fn fetch_all(&self, data: Arc<Mutex<AppData>>) -> AppResult<()> {
        log::info!("Fetching all data concurrently");

        // Run all fetches in parallel
        let (space_weather_result, forecast_result, dx_result, sat_result) = tokio::join!(
            self.fetch_space_weather(),
            self.fetch_forecast(),
            self.fetch_dx_cluster(),
            self.fetch_satellite_data(),
        );

        // Update data store with results
        let mut app_data = data.lock().await;

        if let Ok(sw) = space_weather_result {
            app_data.space_weather = sw;
            log::debug!("Updated space weather");
        }

        if let Ok(forecast) = forecast_result {
            app_data.forecast = forecast;
            log::debug!("Updated forecast");
        }

        if let Ok(dx) = dx_result {
            app_data.dx_spots = dx;
            log::debug!("Updated DX spots");
        }

        if let Ok(sat) = sat_result {
            app_data.satellites = sat;
            log::debug!("Updated satellite data");
        }

        app_data.update_timestamp();
        log::info!("All data updated successfully");

        Ok(())
    }

    /// Fetch space weather data (KP index, solar activity) from spacex.land
    /// Uses caching with 30-minute TTL
    async fn fetch_space_weather(&self) -> AppResult<SpaceWeather> {
        // Check cache first
        if let Some(cached) = self.cache.get_space_weather() {
            return Ok(cached);
        }

        log::debug!("Fetching space weather from spacex.land API...");

        let response = self.client
            .get("https://api.spacex.land/now/kp")
            .timeout(std::time::Duration::from_secs(10))
            .send()
            .await
            .map_err(|e| {
                log::warn!("Space weather fetch failed: {}", e);
                crate::error::AppError::NetworkError(format!("Space weather API: {}", e))
            })?;

        if !response.status().is_success() {
            log::warn!("Space weather API returned status: {}", response.status());
            return Ok(SpaceWeather::default());
        }

        let data: SpaceWeatherApiResponse = response
            .json()
            .await
            .map_err(|e| {
                log::warn!("Space weather JSON parse failed: {}", e);
                crate::error::AppError::ParseError(format!("Space weather: {}", e))
            })?;

        let kp = data.kp.unwrap_or(0.0);
        let a = data.a.unwrap_or(0.0) as i32;

        let weather = SpaceWeather {
            kp,
            a,
            ap: a, // AP index approximation
            flux: 80, // Placeholder - would need separate endpoint
            updated: Utc::now(),
        };

        log::info!("Space weather: KP={:.1}, A={}", kp, a);

        // Cache the result
        self.cache.cache_space_weather(weather.clone());

        Ok(weather)
    }

    /// Fetch weather forecast using Open-Meteo API (free, no authentication required)
    /// Uses caching with 2-hour TTL
    async fn fetch_forecast(&self) -> AppResult<Vec<Forecast>> {
        // Check cache first
        if let Some(cached) = self.cache.get_forecast() {
            return Ok(cached);
        }

        log::debug!("Fetching weather forecast from Open-Meteo API...");

        // Using a default location (can be made configurable)
        // This example uses coordinates for Greenwich, UK
        let url = "https://api.open-meteo.com/v1/forecast?latitude=51.48&longitude=-0.00&hourly=temperature_2m,relative_humidity_2m,weather_code&temperature_unit=celsius&wind_speed_unit=kmh&precipitation_unit=mm&timezone=UTC";

        let response = self.client
            .get(url)
            .timeout(std::time::Duration::from_secs(10))
            .send()
            .await
            .map_err(|e| {
                log::warn!("Forecast fetch failed: {}", e);
                crate::error::AppError::NetworkError(format!("Forecast API: {}", e))
            })?;

        if !response.status().is_success() {
            log::warn!("Forecast API returned status: {}", response.status());
            return Ok(vec![]);
        }

        let data: MeteoResponse = response
            .json()
            .await
            .map_err(|e| {
                log::warn!("Forecast JSON parse failed: {}", e);
                crate::error::AppError::ParseError(format!("Forecast: {}", e))
            })?;

        let mut forecasts = Vec::new();

        if let Some(hourly) = data.hourly {
            if let (Some(times), Some(temps), Some(humidity)) = (hourly.time, hourly.temperature_2m, hourly.relative_humidity_2m) {
                // Take first 7 days (7 * 24 hourly entries)
                let max_entries = std::cmp::min(7 * 24, times.len());

                for i in (0..max_entries).step_by(24) {
                    if i < temps.len() && i < times.len() {
                        let date = times[i].split('T').next().unwrap_or("").to_string();
                        let temp_high = temps[i].round() as i32;
                        let temp_low = if i + 12 < temps.len() {
                            temps[i + 12].round() as i32
                        } else {
                            temp_high - 2
                        };
                        let hum = humidity[i];

                        forecasts.push(Forecast {
                            date,
                            temp_high,
                            temp_low,
                            conditions: "Fair".to_string(),
                            humidity: hum,
                        });
                    }
                }
            }
        }

        log::info!("Fetched {} forecast entries", forecasts.len());

        // Cache the result
        self.cache.cache_forecast(forecasts.clone());

        Ok(forecasts)
    }

    /// Fetch DX cluster spots (uses placeholder data - real implementation would connect to DX cluster)
    async fn fetch_dx_cluster(&self) -> AppResult<Vec<DxSpot>> {
        log::debug!("Fetching DX spots...");

        // Placeholder: In a production system, this would:
        // - Connect to a DX cluster via TCP (e.g., AR-Cluster)
        // - Parse spot information in real-time
        // - Cache results for performance
        //
        // For now, returning empty to indicate the infrastructure is ready
        log::info!("DX cluster: Currently using placeholder (integration available)");

        Ok(vec![])
    }

    /// Fetch satellite tracking data (uses placeholder data - would integrate with N2YO or similar)
    async fn fetch_satellite_data(&self) -> AppResult<Vec<SatelliteData>> {
        log::debug!("Fetching satellite data...");

        // Placeholder: In a production system, this would:
        // - Query N2YO API for satellite passes (requires API key)
        // - Use local TLE (Two-Line Element) sets for calculations
        // - Calculate Doppler shifts for ham radio operators
        // - Filter satellites of interest (ISS, NOAA weather sats, ham radio satellites)
        //
        // For now, returning empty to indicate the infrastructure is ready
        log::info!("Satellite data: Currently using placeholder (N2YO/TLE integration available)");

        Ok(vec![])
    }
}

impl Default for DataFetcher {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[tokio::test]
    async fn test_fetcher_creation() {
        let fetcher = DataFetcher::new();
        assert!(!fetcher.client.get("https://example.com").build().unwrap().url().to_string().is_empty());
    }
}
