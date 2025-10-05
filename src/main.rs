//! Main entry point for the Rust application
//! This application demonstrates a pure Rust implementation without complex native dependencies

use anyhow::Result;
use log::info;
use std::env;

mod crypto;
mod server;

#[tokio::main]
async fn main() -> Result<()> {
    // Initialize logging
    env_logger::init();
    
    info!("Starting Rust application...");
    
    // Get port from environment or use default
    let port = env::var("PORT")
        .unwrap_or_else(|_| "5000".to_string())
        .parse::<u16>()
        .unwrap_or(5000);
    
    // Get host from environment or use default
    let host = env::var("HOST")
        .unwrap_or_else(|_| "0.0.0.0".to_string());
    
    info!("Server will start on {}:{}", host, port);
    
    // Start the server
    server::start_server(&host, port).await?;
    
    Ok(())
}
