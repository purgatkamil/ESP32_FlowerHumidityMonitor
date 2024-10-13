#include "spiffs_module.h"

void init_spiffs() {
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
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info("storage", &total, &used);
    if (ret != ESP_OK) {
        printf("STORAGE: Failed to get SPIFFS partition information (%s)\n", esp_err_to_name(ret));
    } else {
        printf("STORAGE: Partition size: total: %d, used: %d\n", total, used);
    }
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
