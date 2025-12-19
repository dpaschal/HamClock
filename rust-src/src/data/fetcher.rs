//! Async data fetcher using reqwest and tokio

use crate::error::AppResult;
use crate::data::models::{SpaceWeather, Forecast, DxSpot, SatelliteData, AppData};
use reqwest::Client;
use std::sync::Arc;
use tokio::sync::Mutex;

/// Async data fetcher for HamClock
pub struct DataFetcher {
    client: Client,
}

impl DataFetcher {
    /// Create a new data fetcher
    pub fn new() -> Self {
        Self {
            client: Client::new(),
        }
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

    /// Fetch space weather data (KP index, solar activity)
    async fn fetch_space_weather(&self) -> AppResult<SpaceWeather> {
        log::debug!("Fetching space weather...");

        // TODO: Replace with actual API endpoint
        // Example: https://api.spacex.land/now/kp
        let space_weather = SpaceWeather::default();
        Ok(space_weather)
    }

    /// Fetch weather forecast
    async fn fetch_forecast(&self) -> AppResult<Vec<Forecast>> {
        log::debug!("Fetching forecast...");

        // TODO: Replace with actual weather API
        // Example: OpenWeatherMap API
        Ok(vec![])
    }

    /// Fetch DX cluster spots
    async fn fetch_dx_cluster(&self) -> AppResult<Vec<DxSpot>> {
        log::debug!("Fetching DX spots...");

        // TODO: Replace with actual DX cluster connection
        // Example: Connect to ARCI DX Cluster or similar
        Ok(vec![])
    }

    /// Fetch satellite tracking data
    async fn fetch_satellite_data(&self) -> AppResult<Vec<SatelliteData>> {
        log::debug!("Fetching satellite data...");

        // TODO: Replace with actual satellite API
        // Example: N2YO API or local TLE data
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
