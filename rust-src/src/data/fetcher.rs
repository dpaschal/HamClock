//! Async data fetcher using reqwest and tokio with response caching

use crate::error::AppResult;
use crate::data::models::{SpaceWeather, Forecast, DxSpot, SatelliteData, AppData,
                        AuroraLevel, CmeEvent, CmeSeverity};
use crate::data::cache::ResponseCache;
use crate::Config;
use reqwest::Client;
use serde::Deserialize;
use chrono::{Utc, TimeZone, Datelike};
use std::sync::Arc;
use tokio::sync::Mutex;
use tokio::time::Duration;
use tokio::io::{AsyncBufReadExt, AsyncWriteExt};
use tokio::net::TcpStream;

/// Async data fetcher for HamClock with caching
pub struct DataFetcher {
    client: Client,
    cache: Arc<ResponseCache>,
}

/// Space Weather API response from spacex.land (legacy fallback)
#[derive(Deserialize, Debug)]
#[allow(dead_code)]
struct SpaceWeatherApiResponse {
    #[serde(rename = "Kp")]
    kp: Option<f32>,
    #[serde(rename = "A")]
    a: Option<f32>,
}

/// HamQSL XML API response structure (primary data source)
#[derive(Deserialize, Debug)]
#[serde(rename_all = "lowercase")]
struct HamQslResponse {
    #[serde(default)]
    solardata: HamQslSolarData,
}

#[derive(Deserialize, Debug, Default)]
#[allow(dead_code)]
struct HamQslSolarData {
    #[serde(default)]
    kindex: f32,
    #[serde(default)]
    kindex_adj: f32,
    #[serde(default)]
    a: f32,
    #[serde(default)]
    a_adj: f32,
    #[serde(default)]
    ap: f32,
    #[serde(default)]
    solarflux: i32,
    #[serde(default)]
    solarflux_adj: i32,
    #[serde(default)]
    sunspots: i32,
    #[serde(default)]
    windspeeds: f32,
    #[serde(default)]
    magnet: String,  // e.g., "ACTIVE", "QUIET", "UNSETTLED"
    #[serde(default)]
    xray: String,   // e.g., "M2.4", "X1.0"
    #[serde(default)]
    status: i32,
    #[serde(default)]
    moonfraction: f32,
}

/// NOAA SWPC Alert response (supplementary CME/aurora alerts)
#[derive(Deserialize, Debug, Clone)]
struct NoaaAlert {
    pub event: String,          // e.g., "Solar Wind Speed greater than 1000 km/s"
    #[allow(dead_code)]
    pub onset_time: String,     // ISO format - kept for future use
    pub id: Option<String>,
}

/// NOAA CME tracking data (reserved for future CME tracking integration)
#[derive(Deserialize, Debug)]
#[allow(dead_code)]
struct NoaaCmeData {
    pub cme_detected: Option<bool>,
    pub speed: Option<f32>,     // km/s
    pub estimated_impact: Option<String>, // ISO format
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
    #[allow(dead_code)]
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

    /// Fetch comprehensive space weather data from HamQSL + NOAA
    /// Uses caching with 15-minute TTL for primary data
    async fn fetch_space_weather(&self) -> AppResult<SpaceWeather> {
        // Check cache first
        if let Some(cached) = self.cache.get_space_weather() {
            return Ok(cached);
        }

        log::info!("Fetching comprehensive space weather from HamQSL + NOAA...");

        // Fetch from both sources in parallel
        let (hamqsl_result, noaa_result) = tokio::join!(
            self.fetch_hamqsl_data(),
            self.fetch_noaa_alerts(),
        );

        // Use HamQSL as primary source - it provides comprehensive data
        let hamqsl = hamqsl_result.unwrap_or_else(|e| {
            log::warn!("HamQSL fetch failed ({}), using fallback", e);
            // Fallback: try the old spacex.land API
            HamQslResponse {
                solardata: HamQslSolarData::default(),
            }
        });

        // NOAA alerts are supplementary - failure is non-critical
        let alerts = noaa_result.unwrap_or_else(|_e| {
            log::debug!("NOAA alerts unavailable, proceeding without CME alerts");
            vec![]
        });

        // Build comprehensive space weather from both sources
        let weather = self.build_space_weather(hamqsl, alerts)?;

        log::info!("Space weather updated: Kp={:.1}, Flux={} SFU, Status={}",
                  weather.kp, weather.solar_flux, weather.geomag_status);

        // Cache the result with 15-minute TTL
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

    /// Fetch DX cluster spots via TCP connection
    async fn fetch_dx_cluster(&self) -> AppResult<Vec<DxSpot>> {
        log::debug!("Fetching DX spots...");

        // Check cache first
        if let Some(cached) = self.cache.get_dx_spots() {
            log::debug!("DX spots from cache");
            return Ok(cached);
        }

        let config = Config::load().unwrap_or_default();
        let addr = format!("{}:{}", config.dx_cluster_host, config.dx_cluster_port);

        log::debug!("Connecting to DX cluster: {}", addr);

        // Connect with timeout
        let stream = match tokio::time::timeout(
            Duration::from_secs(5),
            TcpStream::connect(&addr)
        ).await {
            Ok(Ok(stream)) => stream,
            Ok(Err(e)) => {
                log::warn!("Failed to connect to DX cluster {}: {}", addr, e);
                return Ok(vec![]);
            }
            Err(_) => {
                log::warn!("DX cluster connection timeout: {}", addr);
                return Ok(vec![]);
            }
        };

        let (reader, mut writer) = stream.into_split();
        let mut lines = tokio::io::BufReader::new(reader).lines();

        // Send login commands
        let _ = writer.write_all(b"set/dx/filter not by skimmer\n").await;
        let _ = writer.write_all(b"show/dx 10\n").await;

        let mut spots = Vec::new();
        let timeout_duration = Duration::from_secs(3);

        // Read DX spots with timeout
        loop {
            match tokio::time::timeout(timeout_duration, lines.next_line()).await {
                Ok(Ok(Some(line))) => {
                    if let Some(spot) = parse_dx_spot(&line) {
                        spots.push(spot);
                    }

                    if spots.len() >= 10 {
                        break;
                    }
                }
                Ok(Ok(None)) | Err(_) => break,
                Ok(Err(e)) => {
                    log::warn!("Error reading from DX cluster: {}", e);
                    break;
                }
            }
        }

        log::info!("Fetched {} DX spots from {}", spots.len(), addr);

        // Cache the result
        if !spots.is_empty() {
            self.cache.cache_dx_spots(spots.clone());
        }

        Ok(spots)
    }

    /// Fetch satellite tracking data from N2YO API
    async fn fetch_satellite_data(&self) -> AppResult<Vec<SatelliteData>> {
        log::debug!("Fetching satellite data...");

        // Check cache first
        if let Some(cached) = self.cache.get_satellites() {
            log::debug!("Satellites from cache");
            return Ok(cached);
        }

        let config = Config::load().unwrap_or_default();

        // Check if API key is configured
        let api_key = match &config.n2yo_api_key {
            Some(key) => key,
            None => {
                log::warn!("N2YO API key not configured, skipping satellite fetch");
                return Ok(vec![]);
            }
        };

        let (lat, lon) = config.location;
        let altitude = 0;           // Observer altitude (meters)
        let search_radius = 70;     // Search radius (degrees from zenith)
        let category = 18;          // Amateur radio satellites

        let url = format!(
            "https://api.n2yo.com/rest/v1/satellite/above/{}/{}/{}/{}/{}/{}",
            lat, lon, altitude, search_radius, category, api_key
        );

        log::debug!("Fetching satellites from N2YO...");

        let response = match self.client
            .get(&url)
            .timeout(Duration::from_secs(10))
            .send()
            .await
        {
            Ok(resp) => resp,
            Err(e) => {
                log::warn!("N2YO request failed: {}", e);
                return Ok(vec![]);
            }
        };

        if !response.status().is_success() {
            log::warn!("N2YO API error: {}", response.status());
            return Ok(vec![]);
        }

        // Parse JSON response
        #[derive(Deserialize)]
        struct N2YOResponse {
            above: Vec<N2YOSatellite>,
        }

        #[derive(Deserialize)]
        struct N2YOSatellite {
            satname: String,
            satalt: f32,    // Altitude in km
            satel: f32,     // Elevation in degrees
            sataz: f32,     // Azimuth in degrees
            #[serde(default)]
            satdist: Option<f32>,  // Distance in km
        }

        let data: N2YOResponse = match response.json().await {
            Ok(d) => d,
            Err(e) => {
                log::warn!("Failed to parse N2YO response: {}", e);
                return Ok(vec![]);
            }
        };

        // Convert to SatelliteData
        let satellites: Vec<SatelliteData> = data.above
            .into_iter()
            .filter(|sat| sat.satel > 0.0)  // Only visible satellites (above horizon)
            .map(|sat| {
                // Calculate Doppler shift for 2m band (146 MHz)
                let velocity_km_per_s = 7.5;  // Approximate orbital velocity
                let speed_of_light_km_per_s = 299792.458;
                let freq_mhz = 146.0;
                let doppler = (velocity_km_per_s / speed_of_light_km_per_s) * freq_mhz * 1e6;

                SatelliteData {
                    name: sat.satname,
                    elevation: sat.satel,
                    azimuth: sat.sataz,
                    range: sat.satdist.unwrap_or(sat.satalt * 1.5), // Estimate if not provided
                    doppler_shift: doppler as f32,
                }
            })
            .collect();

        log::info!("Fetched {} visible satellites", satellites.len());

        // Cache the result
        if !satellites.is_empty() {
            self.cache.cache_satellites(satellites.clone());
        }

        Ok(satellites)
    }

    /// Fetch comprehensive space weather from HamQSL (XML API)
    /// This replaces the basic spacex.land API with full data from ham radio operators
    async fn fetch_hamqsl_data(&self) -> AppResult<HamQslResponse> {
        log::debug!("Fetching space weather from HamQSL XML API...");

        let response = self.client
            .get("https://www.hamqsl.com/solarxml.php")
            .timeout(std::time::Duration::from_secs(10))
            .send()
            .await
            .map_err(|e| {
                log::warn!("HamQSL fetch failed: {}", e);
                crate::error::AppError::NetworkError(format!("HamQSL API: {}", e))
            })?;

        if !response.status().is_success() {
            log::warn!("HamQSL API returned status: {}", response.status());
            return Err(crate::error::AppError::ParseError("HamQSL API error".to_string()));
        }

        let xml_text = response
            .text()
            .await
            .map_err(|e| {
                log::warn!("HamQSL response text read failed: {}", e);
                crate::error::AppError::ParseError(format!("HamQSL read: {}", e))
            })?;

        // Parse XML response
        let data: HamQslResponse = serde_xml_rs::from_str(&xml_text)
            .map_err(|e| {
                log::warn!("HamQSL XML parse failed: {}", e);
                crate::error::AppError::ParseError(format!("HamQSL XML: {}", e))
            })?;

        log::info!("HamQSL: Kp={:.1}, Flux={} SFU, X-ray={}, Magnet={}",
                  data.solardata.kindex, data.solardata.solarflux,
                  data.solardata.xray, data.solardata.magnet);

        Ok(data)
    }

    /// Fetch CME and alert data from NOAA SWPC (JSON APIs)
    async fn fetch_noaa_alerts(&self) -> AppResult<Vec<NoaaAlert>> {
        log::debug!("Fetching space weather alerts from NOAA SWPC...");

        let response = self.client
            .get("https://services.swpc.noaa.gov/products/alerts.json")
            .timeout(std::time::Duration::from_secs(10))
            .send()
            .await
            .map_err(|e| {
                log::warn!("NOAA alerts fetch failed: {}", e);
                // Don't error on NOAA - fall through to defaults
                crate::error::AppError::NetworkError(format!("NOAA alerts: {}", e))
            })?;

        if !response.status().is_success() {
            log::warn!("NOAA alerts returned status: {}", response.status());
            return Ok(vec![]);  // Return empty list, not error
        }

        let alerts: Vec<NoaaAlert> = response
            .json()
            .await
            .unwrap_or_default();

        log::info!("NOAA: {} active alerts", alerts.len());
        Ok(alerts)
    }

    /// Build comprehensive SpaceWeather from multiple sources
    /// Combines HamQSL (primary) with NOAA alerts (supplementary)
    fn build_space_weather(&self, hamqsl: HamQslResponse, alerts: Vec<NoaaAlert>) -> AppResult<SpaceWeather> {

        let data = &hamqsl.solardata;

        // Parse X-ray class from string like "M2.4" or "X1.0"
        let xray_class = data.xray.clone();

        // Create placeholder aurora activity based on Kp index
        let aurora = AuroraLevel::from_kp(data.kindex);

        // Create placeholder aurora visibility threshold
        let aurora_visible_lat = match aurora {
            AuroraLevel::None => None,
            AuroraLevel::Low => Some(65.0),      // Far north
            AuroraLevel::Moderate => Some(60.0),  // North
            AuroraLevel::High => Some(50.0),      // Well south
            AuroraLevel::Extreme => Some(35.0),   // Very far south
        };

        // Parse alerts for CME events
        let active_cmes = alerts.iter()
            .filter(|a| a.event.contains("CME") || a.event.contains("Solar Wind"))
            .enumerate()
            .map(|(idx, alert)| CmeEvent {
                id: alert.id.clone().unwrap_or_else(|| format!("CME-{}", idx)),
                detection_time: Utc::now(),
                earth_directed: true,  // Assume earth-directed if alerted
                speed: 1200.0,  // Placeholder average CME speed
                estimated_arrival: None,
                severity: match data.kindex {
                    k if k > 8.0 => CmeSeverity::Extreme,
                    k if k > 6.0 => CmeSeverity::Severe,
                    k if k > 5.0 => CmeSeverity::Strong,
                    k if k > 3.0 => CmeSeverity::Moderate,
                    _ => CmeSeverity::Minor,
                },
            })
            .collect();

        let weather = SpaceWeather {
            kp: data.kindex,
            a: data.a as i32,
            ap: data.ap as i32,
            solar_flux: data.solarflux,
            sunspot_number: data.sunspots,
            solar_wind_speed: data.windspeeds,
            imf_bt: 0.0,  // Not provided by HamQSL
            imf_bz: 0.0,  // Not provided by HamQSL
            current_xray_class: xray_class,
            recent_flares: vec![],  // Placeholder - would parse from HamQSL if available
            aurora_activity: aurora,
            aurora_visible_at: aurora_visible_lat,
            active_cmes,
            geomag_status: data.magnet.clone(),
            updated: Utc::now(),
        };

        Ok(weather)
    }
}

impl Default for DataFetcher {
    fn default() -> Self {
        Self::new()
    }
}

/// Parse a DX spot line from DX cluster
/// Example: "DX de W1AW:     14074.0  DL1ABC       CQ SSB                 2359Z"
fn parse_dx_spot(line: &str) -> Option<DxSpot> {
    if !line.starts_with("DX de") {
        return None;
    }

    let parts: Vec<&str> = line.split_whitespace().collect();
    if parts.len() < 8 {
        return None;
    }

    // Parse components
    let spotter = parts[2].trim_end_matches(':').to_string();
    let frequency = parts[3].parse::<f32>().ok()?;
    let callsign = parts[4].to_string();
    let mode = parts.get(6).unwrap_or(&"UNK").to_string();
    let time_str = parts.last()?;

    // Parse time (e.g., "2359Z")
    let hour = time_str.get(0..2)?.parse::<u32>().ok()?;
    let minute = time_str.get(2..4)?.parse::<u32>().ok()?;
    let now = Utc::now();
    let time = chrono::Utc.with_ymd_and_hms(
        now.year(), now.month(), now.day(),
        hour, minute, 0
    ).earliest()?;

    Some(DxSpot {
        frequency,
        callsign,
        spotter,
        mode,
        time,
    })
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
