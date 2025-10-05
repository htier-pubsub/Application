//! HTTP server implementation using pure Rust dependencies
//! Uses warp for HTTP server functionality

use crate::crypto::Crypto;
use anyhow::Result;
use log::{error, info, warn};
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::convert::Infallible;
use std::sync::Arc;
use tokio::sync::RwLock;
use warp::{http::StatusCode, Filter, Rejection, Reply};

/// Application state
#[derive(Debug, Clone)]
pub struct AppState {
    pub crypto: Arc<Crypto>,
    pub data: Arc<RwLock<HashMap<String, String>>>,
}

impl AppState {
    pub fn new() -> Self {
        Self {
            crypto: Arc::new(Crypto::new()),
            data: Arc::new(RwLock::new(HashMap::new())),
        }
    }
}

/// Health check response
#[derive(Serialize)]
struct HealthResponse {
    status: String,
    timestamp: u64,
    version: String,
}

/// Generic API response
#[derive(Serialize)]
struct ApiResponse<T> {
    success: bool,
    data: Option<T>,
    error: Option<String>,
}

/// Crypto operation request
#[derive(Deserialize)]
struct CryptoRequest {
    operation: String,
    data: Option<String>,
    length: Option<usize>,
}

/// Crypto operation response
#[derive(Serialize)]
struct CryptoResponse {
    result: String,
    operation: String,
}

/// Start the HTTP server
pub async fn start_server(host: &str, port: u16) -> Result<()> {
    let state = AppState::new();
    
    // Health check endpoint
    let health = warp::path("health")
        .and(warp::get())
        .map(|| {
            let response = HealthResponse {
                status: "healthy".to_string(),
                timestamp: std::time::SystemTime::now()
                    .duration_since(std::time::UNIX_EPOCH)
                    .unwrap()
                    .as_secs(),
                version: env!("CARGO_PKG_VERSION").to_string(),
            };
            warp::reply::json(&ApiResponse {
                success: true,
                data: Some(response),
                error: None,
            })
        });
    
    // Crypto operations endpoint
    let crypto_ops = warp::path("crypto")
        .and(warp::post())
        .and(warp::body::json())
        .and(with_state(state.clone()))
        .and_then(handle_crypto_operation);
    
    // Data storage endpoints
    let store_data = warp::path("data")
        .and(warp::path::param::<String>())
        .and(warp::post())
        .and(warp::body::bytes())
        .and(with_state(state.clone()))
        .and_then(handle_store_data);
    
    let get_data = warp::path("data")
        .and(warp::path::param::<String>())
        .and(warp::get())
        .and(with_state(state.clone()))
        .and_then(handle_get_data);
    
    // Static files for frontend
    let static_files = warp::path("static")
        .and(warp::fs::dir("static"));
    
    // Root endpoint serves a simple HTML page
    let root = warp::path::end()
        .and(warp::get())
        .map(serve_index);
    
    // Combine all routes
    let routes = health
        .or(crypto_ops)
        .or(store_data)
        .or(get_data)
        .or(static_files)
        .or(root)
        .with(warp::cors().allow_any_origin())
        .recover(handle_rejection);
    
    info!("Server starting on {}:{}", host, port);
    
    let addr: std::net::SocketAddr = format!("{}:{}", host, port)
        .parse()
        .map_err(|e| anyhow::anyhow!("Invalid address: {}", e))?;
    
    warp::serve(routes)
        .run(addr)
        .await;
    
    Ok(())
}

/// Helper to pass state to handlers
fn with_state(
    state: AppState,
) -> impl Filter<Extract = (AppState,), Error = Infallible> + Clone {
    warp::any().map(move || state.clone())
}

/// Handle crypto operations
async fn handle_crypto_operation(
    req: CryptoRequest,
    state: AppState,
) -> std::result::Result<impl Reply, Rejection> {
    info!("Crypto operation requested: {}", req.operation);
    
    let result = match req.operation.as_str() {
        "random_hex" => {
            let len = req.length.unwrap_or(16);
            state.crypto.generate_random_hex(len)
        }
        "random_base64" => {
            let len = req.length.unwrap_or(16);
            state.crypto.generate_random_base64(len)
        }
        "sha256" => {
            if let Some(data) = req.data {
                Ok(state.crypto.sha256_string(&data))
            } else {
                Err(anyhow::anyhow!("No data provided for hash"))
            }
        }
        "token" => {
            let len = req.length.unwrap_or(32);
            state.crypto.generate_token(len)
        }
        _ => Err(anyhow::anyhow!("Unknown operation: {}", req.operation)),
    };
    
    match result {
        Ok(result) => {
            let response = CryptoResponse {
                result,
                operation: req.operation,
            };
            Ok(warp::reply::json(&ApiResponse {
                success: true,
                data: Some(response),
                error: None,
            }))
        }
        Err(e) => {
            error!("Crypto operation failed: {}", e);
            Ok(warp::reply::json(&ApiResponse::<()> {
                success: false,
                data: None,
                error: Some(e.to_string()),
            }))
        }
    }
}

/// Handle data storage
async fn handle_store_data(
    key: String,
    data: bytes::Bytes,
    state: AppState,
) -> std::result::Result<impl Reply, Rejection> {
    let data_str = String::from_utf8_lossy(&data).to_string();
    
    {
        let mut storage = state.data.write().await;
        storage.insert(key.clone(), data_str);
    }
    
    info!("Data stored for key: {}", key);
    
    Ok(warp::reply::json(&ApiResponse {
        success: true,
        data: Some(format!("Data stored for key: {}", key)),
        error: None,
    }))
}

/// Handle data retrieval
async fn handle_get_data(
    key: String,
    state: AppState,
) -> std::result::Result<impl Reply, Rejection> {
    let storage = state.data.read().await;
    
    match storage.get(&key) {
        Some(data) => {
            info!("Data retrieved for key: {}", key);
            Ok(warp::reply::json(&ApiResponse {
                success: true,
                data: Some(data.clone()),
                error: None,
            }))
        }
        None => {
            warn!("No data found for key: {}", key);
            Ok(warp::reply::json(&ApiResponse::<String> {
                success: false,
                data: None,
                error: Some(format!("No data found for key: {}", key)),
            }))
        }
    }
}

/// Serve the main HTML page
fn serve_index() -> impl Reply {
    warp::reply::html(include_str!("../web/index.html"))	
}

/// Handle rejections
async fn handle_rejection(err: Rejection) -> std::result::Result<impl Reply, Infallible> {
    error!("Request rejected: {:?}", err);
    
    let (code, message) = if err.is_not_found() {
        (StatusCode::NOT_FOUND, "Not Found")
    } else if err.find::<warp::filters::body::BodyDeserializeError>().is_some() {
        (StatusCode::BAD_REQUEST, "Invalid JSON")
    } else {
        (StatusCode::INTERNAL_SERVER_ERROR, "Internal Server Error")
    };
    
    let json = warp::reply::json(&ApiResponse::<()> {
        success: false,
        data: None,
        error: Some(message.to_string()),
    });
    
    Ok(warp::reply::with_status(json, code))
}
