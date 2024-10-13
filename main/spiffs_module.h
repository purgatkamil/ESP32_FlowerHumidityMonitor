#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_spiffs.h"


#include "esp_vfs.h"
//#include "esp_vfs_spiffs.h"
#include "dirent.h"


void init_spiffs();
void list_spiffs_files();
void read_credentials(char* ssid, char* password);


