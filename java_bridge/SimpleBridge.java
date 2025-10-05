import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.concurrent.ThreadLocalRandom;

public class SimpleBridge {
    private static final String RUST_SERVER_URL = "http://localhost:5000";
    private static final int REG_NB = 10;
    private static final int SLEEP_TIME = 2;
    private static final int HTTP_TIMEOUT = 5;
    
    private final HttpClient httpClient;
    
    public SimpleBridge() {
        this.httpClient = HttpClient.newBuilder()
            .connectTimeout(Duration.ofSeconds(HTTP_TIMEOUT))
            .build();
    }
    
    public boolean checkServerHealth() {
        try {
            HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(RUST_SERVER_URL + "/health"))
                .timeout(Duration.ofSeconds(HTTP_TIMEOUT))
                .GET()
                .build();
                
            HttpResponse<String> response = httpClient.send(request, 
                HttpResponse.BodyHandlers.ofString());
            return response.statusCode() == 200;
        } catch (Exception e) {
            return false;
        }
    }
    
    public boolean sendDataStorage(String key, String value) {
        try {
            HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(RUST_SERVER_URL + "/data/" + key))
                .timeout(Duration.ofSeconds(HTTP_TIMEOUT))
                .header("Content-Type", "text/plain")
                .POST(HttpRequest.BodyPublishers.ofString(value))
                .build();
                
            HttpResponse<String> response = httpClient.send(request, 
                HttpResponse.BodyHandlers.ofString());
            return response.statusCode() == 200;
        } catch (Exception e) {
            System.err.println("Error storing data: " + e.getMessage());
            return false;
        }
    }
    
    public String sendCryptoOperation(String operation, String data, Integer length) {
        try {
            StringBuilder payload = new StringBuilder("{\"operation\":\"" + operation + "\"");
            if (data != null && !data.isEmpty()) {
                payload.append(",\"data\":\"").append(data).append("\"");
            }
            if (length != null && length > 0) {
                payload.append(",\"length\":").append(length);
            }
            payload.append("}");
            
            HttpRequest request = HttpRequest.newBuilder()
                .uri(URI.create(RUST_SERVER_URL + "/crypto"))
                .timeout(Duration.ofSeconds(HTTP_TIMEOUT))
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(payload.toString()))
                .build();
                
            HttpResponse<String> response = httpClient.send(request, 
                HttpResponse.BodyHandlers.ofString());
                
            if (response.statusCode() == 200) {
                return response.body();
            }
        } catch (Exception e) {
            System.err.println("Error with crypto operation: " + e.getMessage());
        }
        return null;
    }
    
    public static class ModbusSimulator {
        private List<Integer> registers;
        private List<Integer> lastState;
        private final Random random;
        
        public ModbusSimulator() {
            this.registers = new ArrayList<>(REG_NB);
            this.lastState = new ArrayList<>(REG_NB);
            this.random = ThreadLocalRandom.current();
            
            // Initialize with zeros
            for (int i = 0; i < REG_NB; i++) {
                registers.add(0);
                lastState.add(0);
            }
        }
        
        public void generateRandomValues() {
            for (int i = 0; i < REG_NB; i++) {
                registers.set(i, random.nextInt(101)); // 0-100
            }
            
            // Set the last two registers to create a float value of 1.0
            registers.set(REG_NB - 1, 0x3F80);  // Set the second last register
            registers.set(REG_NB - 2, 0x0000);  // Set the last register
        }
        
        public List<Integer> getRegisters() {
            return new ArrayList<>(registers);
        }
        
        public boolean hasStateChanged() {
            if (!registers.equals(lastState)) {
                lastState = new ArrayList<>(registers);
                return true;
            }
            return false;
        }
    }
    
    private static String getCurrentTimestamp() {
        return LocalDateTime.now().format(DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss"));
    }
    
    public static void main(String[] args) {
        System.out.println("Java HTTP Bridge to Rust Server starting...");
        
        SimpleBridge bridge = new SimpleBridge();
        ModbusSimulator modbus = new ModbusSimulator();
        
        // Check if server is running
        if (!bridge.checkServerHealth()) {
            System.err.println("Error: Rust server is not running at " + RUST_SERVER_URL);
            System.err.println("Please start your rust-app.exe first!");
            System.exit(1);
        }
        
        System.out.println("Rust server is available. Starting bridge...");
        System.out.println("Java Modbus server simulation is online");
        
        try {
            while (true) {
                // Generate random values for all registers
                modbus.generateRandomValues();
                
                String timestamp = getCurrentTimestamp();
                
                // Check if state has changed
                if (modbus.hasStateChanged()) {
                    List<Integer> currentValue = modbus.getRegisters();
                    String message = currentValue.toString() + "_" + timestamp;
                    
                    boolean result = bridge.sendDataStorage("java_message", message);
                    if (result) {
                        System.out.println("✓ Stored message: " + message);
                    } else {
                        System.out.println("✗ Failed to store message");
                    }
                    
                    // Example crypto operations (simplified without JSON parsing)
                    String cryptoResult = bridge.sendCryptoOperation("random_hex", null, 16);
                    if (cryptoResult != null && cryptoResult.contains("\"success\":true")) {
                        System.out.println("✓ Generated hex operation successful");
                    } else {
                        System.out.println("✗ Failed to generate hex");
                    }
                    
                    String hashResult = bridge.sendCryptoOperation("sha256", timestamp, null);
                    if (hashResult != null && hashResult.contains("\"success\":true")) {
                        System.out.println("✓ SHA256 operation successful");
                    } else {
                        System.out.println("✗ Failed to create hash");
                    }
                }
                
                Thread.sleep(SLEEP_TIME * 1000);
            }
        } catch (InterruptedException e) {
            System.err.println("Shutdown server ... " + e.getMessage());
            Thread.currentThread().interrupt();
        } catch (Exception e) {
            System.err.println("Unexpected error: " + e.getMessage());
        }
        
        System.out.println("Java Bridge is offline");
    }
}