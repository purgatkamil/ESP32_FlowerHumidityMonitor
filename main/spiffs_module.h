#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_err.h"
#include "esp_spiffs.h"
#include "esp_log.h"


#include "esp_vfs.h"
//#include "esp_vfs_spiffs.h"
#include "dirent.h"


esp_err_t init_spiffs_credentials(void);
esp_err_t init_spiffs_web(void);
void list_spiffs_files();
void read_credentials(char* ssid, char* password);
void get_recent_ip_from_file(char* recent_ip) ;
void save_ip_to_file(const char *ip);


