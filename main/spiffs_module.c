#include "spiffs_module.h"

#define TAG "spiffs_module"

esp_err_t init_spiffs_credentials(void) {
    esp_vfs_spiffs_conf_t pass_conf = {
        .base_path = "/storage",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&pass_conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            printf("STORAGE: Failed to mount or format filesystem\n");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            printf("STORAGE: Failed to find SPIFFS partition\n");
        } else {
            printf("STORAGE: Failed to initialize SPIFFS (%s)\n", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info("storage", &total, &used);
    if (ret != ESP_OK) {
        printf("STORAGE: Failed to get SPIFFS partition information (%s)\n", esp_err_to_name(ret));
    } else {
        printf("STORAGE: Partition size: total: %d, used: %d\n", total, used);
    }
    return ESP_OK;
}

esp_err_t init_spiffs_web(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = CONFIG_EXAMPLE_WEB_MOUNT_POINT,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ESP_OK;
}

void list_spiffs_files() {
    struct dirent *entry;
    DIR *dp = opendir("/storage");

    if (dp == NULL) {
        printf("Failed to open directory\n");
        return;
    }

    while ((entry = readdir(dp))) {
        printf("Found file: %s\n", entry->d_name);
    }

    closedir(dp);
}

void read_credentials(char* ssid, char* password) {
    FILE *file = fopen("/storage/secrets.conf", "r");
    if (file == NULL) {
        printf("Failed to open secrets.conf\n");
        return;
    }

    char line[100];
    while (fgets(line, sizeof(line), file)) {
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "\n");

        if (strcmp(key, "SSID") == 0) {
            strcpy(ssid, value);
        } else if (strcmp(key, "PASSWORD") == 0) {
            strcpy(password, value);
        }
    }

    fclose(file);
}

void get_recent_ip_from_file(char* recent_ip) {
    FILE *file = fopen("/storage/secrets.conf", "r");
    if (file == NULL) {
        printf("Failed to open secrets.conf\n");
        return;
    }

    char line[100];
    while (fgets(line, sizeof(line), file)) {
        char *key = strtok(line, "=");
        char *value = strtok(NULL, "\n");

        if (strcmp(key, "RECENT_IP") == 0) {
            strcpy(recent_ip, value);
        }
    }

    fclose(file);
}

#define IP_BUFFER_SIZE 30  // Rozmiar bufora dla adresu IP
#define MAX_LINES 4

void save_ip_to_file(const char *ip) {
    ESP_LOGI(TAG, "Opening file for reading...");
    FILE *f = fopen("/storage/secrets.conf", "r");

    // Dynamicznie alokowana tablica wskaźników na linie
    char *lines[MAX_LINES] = {0};
    int line_count = 0;
    bool ip_replaced = false;

    if (f) {
        ESP_LOGI(TAG, "File opened successfully for reading.");
        char buffer[IP_BUFFER_SIZE];
        while (fgets(buffer, sizeof(buffer), f) && line_count < MAX_LINES) {
            // Alokujemy pamięć dla każdej linii na podstawie jej długości
            lines[line_count] = strdup(buffer);
            
            // Sprawdzenie, czy linia zawiera "RECENT_IP"
            if (strstr(lines[line_count], "RECENT_IP=") != NULL) {
                ESP_LOGI(TAG, "Found RECENT_IP line, replacing with new IP.");
                free(lines[line_count]);  // Zwolnij starą linię
                asprintf(&lines[line_count], "RECENT_IP=%s\n", ip);  // Alokuj nową linię z nowym IP
                ip_replaced = true;
            }
            line_count++;
        }
        fclose(f);
        ESP_LOGI(TAG, "Finished reading file.");
    } else {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }

    // Jeśli nie znaleziono linii z "RECENT_IP", dodaj ją na końcu
    if (!ip_replaced && line_count < MAX_LINES) {
        ESP_LOGI(TAG, "RECENT_IP not found, adding new entry.");
        asprintf(&lines[line_count], "RECENT_IP=%s\n", ip);  // Dodaj nową linię
        line_count++;
    }

    ESP_LOGI(TAG, "Opening file for writing...");
    f = fopen("/storage/secrets.conf", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        // Zwolnij pamięć
        for (int i = 0; i < line_count; i++) {
            free(lines[i]);
        }
        return;
    }

    ESP_LOGI(TAG, "Writing updated lines to file...");
    for (int i = 0; i < line_count; i++) {
        fprintf(f, "%s", lines[i]);  // Zapisz każdą linię do pliku
        free(lines[i]);  // Zwolnij pamięć po zapisaniu linii
    }
    fclose(f);
    ESP_LOGI(TAG, "File writing complete. Saved or updated IP: RECENT_IP=\"%s\"", ip);
}




