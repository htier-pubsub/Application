//! Library module for the Rust application
//! Provides common functionality and utilities

pub mod crypto;
pub mod server;

use thiserror::Error;

/// Application-specific error types
#[derive(Error, Debug)]
pub enum AppError {
    #[error("Cryptographic operation failed: {0}")]
    CryptoError(String),
    
    #[error("Server error: {0}")]
    ServerError(String),
    
    #[error("Configuration error: {0}")]
    ConfigError(String),
    
    #[error("IO error: {0}")]
    IoError(#[from] std::io::Error),
    
    #[error("Serialization error: {0}")]
    SerializationError(#[from] serde_json::Error),
}

pub type Result<T> = std::result::Result<T, AppError>;

/// Application configuration
#[derive(Debug, Clone)]
pub struct Config {
    pub server_host: String,
    pub server_port: u16,
    pub log_level: String,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            server_host: "0.0.0.0".to_string(),
            server_port: 5000,
            log_level: "info".to_string(),
        }
    }
}

impl Config {
    /// Load configuration from environment variables
    pub fn from_env() -> Self {
        Self {
            server_host: std::env::var("HOST").unwrap_or_else(|_| "0.0.0.0".to_string()),
            server_port: std::env::var("PORT")
                .unwrap_or_else(|_| "5000".to_string())
                .parse()
                .unwrap_or(5000),
            log_level: std::env::var("LOG_LEVEL").unwrap_or_else(|_| "info".to_string()),
        }
    }
}
