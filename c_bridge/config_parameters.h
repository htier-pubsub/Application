#ifndef CONFIG_PARAMETERS_H
#define CONFIG_PARAMETERS_H

// Modbus configuration
#define REG_ADDR 0
#define REG_NB 10
#define SLEEP_TIME 2

// Server configuration
#define RUST_SERVER_URL "http://localhost:5000"
#define MODBUS_PORT 12345
#define MODBUS_HOST "127.0.0.1"

// HTTP timeout in seconds
#define HTTP_TIMEOUT 5

#endif // CONFIG_PARAMETERS_H
