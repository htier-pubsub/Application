#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include "config_parameters.h"

#define MAX_RESPONSE_SIZE 4096
#define MAX_MESSAGE_SIZE 1024

typedef struct {
    char* memory;
    size_t size;
} MemoryStruct;

typedef struct {
    int registers[REG_NB];
    int lastState[REG_NB];
    int stateChanged;
} ModbusSimulator;

// Callback function to capture HTTP response
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    MemoryStruct *mem = (MemoryStruct *)userp;
    
    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0; // Ensure null termination
    
    return realsize;
}

int check_server_health() {
    CURL *curl;
    CURLcode res = CURLE_FAILED_INIT;  // Initialize here
    MemoryStruct chunk;
    long response_code = 0;
	
    
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    curl = curl_easy_init();
    if (curl) {
        char url[256];
        snprintf(url, sizeof(url), "%s/health", RUST_SERVER_URL);
        
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, HTTP_TIMEOUT);
        
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        curl_easy_cleanup(curl);
    }
    
    if (chunk.memory) {
        free(chunk.memory);
    }
    
    return (res == CURLE_OK && response_code == 200);
}

int send_data_storage(const char* key, const char* value) {
    CURL *curl;
    CURLcode res = CURLE_FAILED_INIT;;
    MemoryStruct chunk;
    long response_code = 0;
    
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    curl = curl_easy_init();
    if (curl) {
        char url[256];
        snprintf(url, sizeof(url), "%s/data/%s", RUST_SERVER_URL, key);
        
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: text/plain");
        
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, value);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, HTTP_TIMEOUT);
        
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    
    if (chunk.memory) {
        free(chunk.memory);
    }
    
    return (res == CURLE_OK && response_code == 200);
}

cJSON* send_crypto_operation(const char* operation, const char* data, int length) {
    CURL *curl;
    CURLcode res = CURLE_FAILED_INIT;  // Initialize
    MemoryStruct chunk;
    long response_code = 0;
    cJSON *json = NULL;
    cJSON *response_json = NULL;
    
    chunk.memory = malloc(1);
    chunk.size = 0;
    
    if (!chunk.memory) {
        printf("Failed to allocate memory for crypto operation\n");
        return NULL;
    }
    
    // Create JSON payload
    json = cJSON_CreateObject();
    if (!json) {
        printf("Failed to create JSON object\n");
        free(chunk.memory);
        return NULL;
    }
    
    cJSON_AddStringToObject(json, "operation", operation);
    
    if (data && strlen(data) > 0) {
        cJSON_AddStringToObject(json, "data", data);
    }
    if (length > 0) {
        cJSON_AddNumberToObject(json, "length", length);
    }
    
    char *json_string = cJSON_Print(json);
    if (!json_string) {
        printf("Failed to print JSON\n");
        cJSON_Delete(json);
        free(chunk.memory);
        return NULL;
    }
    
    curl = curl_easy_init();
    if (curl) {
        char url[256];
        snprintf(url, sizeof(url), "%s/crypto", RUST_SERVER_URL);
        
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_string);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, HTTP_TIMEOUT);
        
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    // Parse response if successful
	 if (res == CURLE_OK && response_code == 200 && chunk.memory && chunk.size > 0) {
        printf("Crypto response received: %s\n", chunk.memory);
        printf("Response length: %zu\n", chunk.size);
        printf("Response last char: %d\n", (int)chunk.memory[chunk.size-1]);
        printf("Attempting to parse JSON...\n");
        
        response_json = cJSON_Parse(chunk.memory);
        if (response_json) {
            printf("JSON parsed successfully\n");
        } else {
            printf("JSON parsing failed\n");
        }
    } else {
        printf("Crypto request failed: res=%d, code=%ld\n", res, response_code);
    }
    
    // Cleanup with debug output
    printf("Starting cleanup...\n");
    
	printf("About to free JSON string...\n");
	if (json_string) {
		printf("JSON string pointer: %p\n", (void*)json_string);
		printf("JSON string first char: %c\n", json_string[0]);
		
		// Check if the pointer looks valid
		if ((uintptr_t)json_string > 0x1000) {  // Basic sanity check
			free(json_string);
			printf("JSON string freed\n");
		} else {
			printf("JSON string pointer looks invalid, skipping free\n");
		}
	} else {
		printf("JSON string was NULL\n");
	}

	// Don't manually free the json_string, let cJSON handle memory
	printf("Starting cleanup...\n");

	printf("About to delete JSON object...\n");
	if (json) {
		cJSON_Delete(json);  // This should also free the json_string
		printf("JSON object deleted\n");
	} else {
		printf("JSON object was NULL\n");
	}

	// DON'T MANUALLY FREE json_string - cJSON_Delete should handle it
	printf("Skipping manual json_string free\n");

	printf("About to free chunk memory...\n");
	if (chunk.memory) {
		free(chunk.memory);
		printf("Chunk memory freed\n");
	} else {
		printf("Chunk memory was NULL\n");
	}

	printf("Cleanup completed, returning response\n");
	return response_json;
}

void get_current_timestamp(char* buffer, size_t buffer_size) {
    time_t rawtime;
    struct tm * timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", timeinfo);
}

void init_modbus_simulator(ModbusSimulator* modbus) {
    for (int i = 0; i < REG_NB; i++) {
        modbus->registers[i] = 0;
        modbus->lastState[i] = 0;
    }
    modbus->stateChanged = 0;
}

void generate_random_values(ModbusSimulator* modbus) {
    srand(time(NULL));
    
    for (int i = 0; i < REG_NB; i++) {
        modbus->registers[i] = rand() % 101; // 0-100
    }
    
    // Set the last two registers to create a float value of 1.0
    modbus->registers[REG_NB - 1] = 0x3F80;  // Set the second last register
    modbus->registers[REG_NB - 2] = 0x0000;  // Set the last register
}

int has_state_changed(ModbusSimulator* modbus) {
    for (int i = 0; i < REG_NB; i++) {
        if (modbus->registers[i] != modbus->lastState[i]) {
            // Update last state
            for (int j = 0; j < REG_NB; j++) {
                modbus->lastState[j] = modbus->registers[j];
            }
            return 1;
        }
    }
    return 0;
}

void registers_to_string(const int* registers, char* buffer) {
    strcpy(buffer, "[");
    for (int i = 0; i < REG_NB; i++) {
        char temp[32];
        if (i > 0) {
            strcat(buffer, ", ");
        }
        snprintf(temp, sizeof(temp), "%d", registers[i]);
        strcat(buffer, temp);
    }
    strcat(buffer, "]");
}

int main() {
    printf("C HTTP Bridge to Rust Server starting...\n");
    
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    ModbusSimulator modbus;
    init_modbus_simulator(&modbus);
    
    // Check if server is running
    if (!check_server_health()) {
        fprintf(stderr, "Error: Rust server is not running at %s\n", RUST_SERVER_URL);
        fprintf(stderr, "Please start your rust-app.exe first!\n");
        curl_global_cleanup();
        return 1;
    }
    
    printf("Rust server is available. Starting bridge...\n");
    printf("C Modbus server simulation is online\n");
    
    while (1) {
        // Generate random values for all registers
        generate_random_values(&modbus);
        
        char timestamp[64];
        get_current_timestamp(timestamp, sizeof(timestamp));
        
        // Check if state has changed
        if (has_state_changed(&modbus)) {
            char registers_str[512];
            //registers_to_string(modbus.registers, registers_str, sizeof(registers_str));
            registers_to_string(modbus.registers, registers_str);
            char message[MAX_MESSAGE_SIZE];
            snprintf(message, sizeof(message), "%s_%s", registers_str, timestamp);
            
            int result = send_data_storage("c_message", message);
            if (result) {
                printf("✓ Stored message: %s\n", message);
            } else {
                printf("✗ Failed to store message\n");
            }
            
            // Example crypto operations - WITH EXTENSIVE LOGGING
		printf("=== Starting crypto operations ===\n");

		printf("Calling first crypto operation...\n");
		// Minimal crypto test - just call and delete
		printf("=== Minimal crypto test ===\n");
		cJSON *crypto_result = send_crypto_operation("random_hex", "", 16);
		if (crypto_result) {
			printf("✓ Got JSON result, deleting immediately\n");
			cJSON_Delete(crypto_result);
			printf("✓ JSON deleted successfully\n");
		} else {
			printf("✗ No JSON result\n");
		}
		printf("=== Minimal crypto test completed ===\n");

		// Add a small delay to see if timing is an issue
		printf("Waiting before second crypto operation...\n");
		sleep(1);
		/*
		printf("Calling second crypto operation...\n");
		cJSON *hash_result = send_crypto_operation("sha256", timestamp, 0);
		printf("Second crypto operation returned: %p\n", (void*)hash_result);

		if (hash_result) {
			printf("Parsing second JSON response...\n");
			cJSON *success = cJSON_GetObjectItem(hash_result, "success");
			
			if (success && cJSON_IsTrue(success)) {
				printf("Second success is true, getting data...\n");
				cJSON *data = cJSON_GetObjectItem(hash_result, "data");
				
				if (data) {
					printf("Getting result from second data...\n");
					cJSON *result_item = cJSON_GetObjectItem(data, "result");
					
					if (result_item && cJSON_IsString(result_item)) {
						char *hash_value = result_item->valuestring;
						if (hash_value) {
								size_t hash_len = strlen(hash_value);
								if (hash_len >= 16) {
									printf("✓ SHA256 of timestamp: %.16s...\n", hash_value);
								} else {
									printf("✗ Hash too short: %zu chars\n", hash_len);
								}
							} else {
								printf("✗ Hash value string is NULL\n");
							}
					} else {
						printf("✗ Invalid hash result format\n");
					}
				} else {
					printf("✗ No data in hash response\n");
				}
			} else {
				printf("✗ Failed to create hash\n");
			}
			
			printf("Deleting second crypto result...\n");
			cJSON_Delete(hash_result);
			printf("Second crypto result deleted.\n");
		} else {
			printf("✗ Second crypto operation returned NULL\n");
		}
		*/
				}
		
		printf("=== All crypto operations completed ===\n");
        
        sleep(SLEEP_TIME);
    }
    
    // Cleanup
    curl_global_cleanup();
    printf("C Bridge is offline\n");
    return 0;
}
