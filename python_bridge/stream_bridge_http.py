import requests
import json
import time
from datetime import datetime
from pyModbusTCP.server import ModbusServer
from random import uniform
import config_parameters

# Configuration for the Rust server
RUST_SERVER_URL = "http://localhost:5000"

def send_to_rust_server(endpoint, data):
    """Send data to the Rust server via HTTP POST"""
    try:
        url = f"{RUST_SERVER_URL}/{endpoint}"
        response = requests.post(url, 
                               data=data,
                               headers={'Content-Type': 'application/json'},
                               timeout=5)
        return response
    except requests.exceptions.RequestException as e:
        print(f"Error sending to Rust server: {e}")
        return None

def send_crypto_operation(operation, data=None, length=None):
    """Send cryptographic operation request to Rust server"""
    payload = {"operation": operation}
    if data:
        payload["data"] = data
    if length:
        payload["length"] = length
    
    try:
        response = requests.post(f"{RUST_SERVER_URL}/crypto",
                               json=payload,
                               headers={'Content-Type': 'application/json'},
                               timeout=5)
        return response.json() if response.status_code == 200 else None
    except requests.exceptions.RequestException as e:
        print(f"Error with crypto operation: {e}")
        return None

def send_data_storage(key, value):
    """Store data in the Rust server"""
    try:
        response = requests.post(f"{RUST_SERVER_URL}/data/{key}",
                               data=value,
                               headers={'Content-Type': 'text/plain'},
                               timeout=5)
        return response.json() if response.status_code == 200 else None
    except requests.exceptions.RequestException as e:
        print(f"Error storing data: {e}")
        return None

def check_server_health():
    """Check if the Rust server is running"""
    try:
        response = requests.get(f"{RUST_SERVER_URL}/health", timeout=5)
        return response.status_code == 200
    except requests.exceptions.RequestException:
        return False

print("HTTP Bridge to Rust Server starting...")

# Check if server is running
if not check_server_health():
    print("Error: Rust server is not running at http://localhost:5000")
    print("Please start your rust-app.exe first!")
    exit(1)

print("Rust server is available. Starting bridge...")



"""
# Main loop - sends various requests to demonstrate functionality
while True:
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    # Example 1: Store timestamped message
    message = f"Hello from Python bridge at {timestamp}"
    result = send_data_storage("python_message", message)
    if result:
        print(f"✓ Stored message: {message}")
    else:
        print("✗ Failed to store message")
    
    # Example 2: Generate random hex
    crypto_result = send_crypto_operation("random_hex", length=16)
    if crypto_result and crypto_result.get('success'):
        hex_value = crypto_result['data']['result']
        print(f"✓ Generated hex: {hex_value}")
    else:
        print("✗ Failed to generate hex")
    
    # Example 3: Create SHA256 hash of current timestamp
    hash_result = send_crypto_operation("sha256", data=timestamp)
    if hash_result and hash_result.get('success'):
        hash_value = hash_result['data']['result']
        print(f"✓ SHA256 of timestamp: {hash_value[:16]}...")
    else:
        print("✗ Failed to create hash")
    
    print(f"--- Cycle completed at {timestamp} ---")
    time.sleep(5)  # Wait 5 seconds before next cycle
"""


# Create an instance of ModbusServer
server = ModbusServer("127.0.0.1", 12345, no_block=True)
try:
    print("Start server...")
    server.start()
    print("Server is online")
    state = [0]
    while True:
        # Generate random values for all registers
        random_values = [int(uniform(0, 100)) for _ in range(config_parameters.REG_NB)]

        # Set the last two registers to create a float value of 1.0
        random_values[config_parameters.REG_NB - 1] = 0x3F80  # Set the second last register to 0x3F80
        random_values[config_parameters.REG_NB - 2] = 0x0000  # Set the last register to 0x0000


        # Update the holding registers with the generated values
        server.data_bank.set_holding_registers(
            config_parameters.REG_ADDR, random_values
        )

        current_value = server.data_bank.get_holding_registers(
            config_parameters.REG_ADDR, config_parameters.REG_NB
        )  # Read all registers
        
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
        # Example 1: Store timestamped message        

        if current_value and state != current_value:  # Check if read was successful
            state = current_value
            message = f"{state}_{timestamp}"
            result = send_data_storage("python_message", message)
            if result:
                print(f"✓ Stored message: {message}")
            else:
                print("✗ Failed to store message")
        time.sleep(config_parameters.SLEEP_TIME)

except Exception as e:
    print(f"Shutdown server ... {e}")
    server.stop()
    print("Server is offline")