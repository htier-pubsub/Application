//! Pure Rust cryptographic utilities
//! Uses the `ring` crate for cryptographic operations to avoid native dependencies

use anyhow::Result;
use thiserror::Error;

/// Application-specific error types
#[derive(Error, Debug)]
pub enum AppError {
    #[error("Cryptographic operation failed: {0}")]
    CryptoError(String),
    
    #[error("Server error: {0}")]
    ServerError(String),
}
use ring::{
    digest::{self, SHA256},
    hmac,
    rand::{SecureRandom, SystemRandom},
};
use base64::{engine::general_purpose::STANDARD as BASE64, Engine};

/// Cryptographic utilities using pure Rust implementations
#[derive(Debug)]
pub struct Crypto {
    rng: SystemRandom,
}

impl Crypto {
    /// Create a new crypto instance
    pub fn new() -> Self {
        Self {
            rng: SystemRandom::new(),
        }
    }
    
    /// Generate a random key of specified length
    pub fn generate_random_bytes(&self, len: usize) -> Result<Vec<u8>> {
        let mut bytes = vec![0u8; len];
        self.rng
            .fill(&mut bytes)
            .map_err(|e| anyhow::anyhow!("Random generation failed: {:?}", e))?;
        Ok(bytes)
    }
    
    /// Generate a random hex string
    pub fn generate_random_hex(&self, len: usize) -> Result<String> {
        let bytes = self.generate_random_bytes(len)?;
        Ok(hex::encode(bytes))
    }
    
    /// Generate a random base64 string
    pub fn generate_random_base64(&self, len: usize) -> Result<String> {
        let bytes = self.generate_random_bytes(len)?;
        Ok(BASE64.encode(bytes))
    }
    
    /// Compute SHA256 hash
    pub fn sha256(&self, data: &[u8]) -> String {
        let hash = digest::digest(&SHA256, data);
        hex::encode(hash.as_ref())
    }
    
    /// Compute SHA256 hash of a string
    pub fn sha256_string(&self, data: &str) -> String {
        self.sha256(data.as_bytes())
    }
    
    /// Create HMAC-SHA256
    pub fn hmac_sha256(&self, key: &[u8], data: &[u8]) -> Result<String> {
        let key = hmac::Key::new(hmac::HMAC_SHA256, key);
        let signature = hmac::sign(&key, data);
        Ok(hex::encode(signature.as_ref()))
    }
    
    /// Verify HMAC-SHA256
    pub fn verify_hmac_sha256(&self, key: &[u8], data: &[u8], expected: &str) -> Result<bool> {
        let computed = self.hmac_sha256(key, data)?;
        Ok(computed == expected)
    }
    
    /// Generate a secure token
    pub fn generate_token(&self, length: usize) -> Result<String> {
        let bytes = self.generate_random_bytes(length)?;
        Ok(BASE64.encode(bytes).chars().take(length).collect())
    }
}

impl Default for Crypto {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    
    #[test]
    fn test_random_generation() {
        let crypto = Crypto::new();
        
        let bytes1 = crypto.generate_random_bytes(32).unwrap();
        let bytes2 = crypto.generate_random_bytes(32).unwrap();
        
        assert_eq!(bytes1.len(), 32);
        assert_eq!(bytes2.len(), 32);
        assert_ne!(bytes1, bytes2);
    }
    
    #[test]
    fn test_sha256() {
        let crypto = Crypto::new();
        let hash = crypto.sha256_string("hello world");
        
        // Known SHA256 hash of "hello world"
        assert_eq!(hash, "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9");
    }
    
    #[test]
    fn test_hmac() {
        let crypto = Crypto::new();
        let key = b"secret_key";
        let data = b"test data";
        
        let hmac = crypto.hmac_sha256(key, data).unwrap();
        assert!(crypto.verify_hmac_sha256(key, data, &hmac).unwrap());
    }
}
