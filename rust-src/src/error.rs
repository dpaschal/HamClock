//! Error types for HamClock

use thiserror::Error;

/// HamClock error type
#[derive(Error, Debug)]
pub enum AppError {
    #[error("Network error: {0}")]
    NetworkError(String),

    #[error("JSON parsing error: {0}")]
    ParseError(String),

    #[error("GPU error: {0}")]
    GpuError(String),

    #[error("Configuration error: {0}")]
    ConfigError(String),

    #[error("IO error: {0}")]
    IoError(#[from] std::io::Error),

    #[error("Serialization error: {0}")]
    SerdeError(#[from] serde_json::Error),

    #[error("Async error: {0}")]
    AsyncError(String),
}

/// Result type for HamClock operations
pub type AppResult<T> = Result<T, AppError>;
