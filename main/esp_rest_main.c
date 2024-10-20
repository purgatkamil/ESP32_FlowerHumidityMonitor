/* HTTP Restful API Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_vfs_semihost.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "sdmmc_cmd.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/apps/netbiosns.h"

#include "wifi_module.h"

#include "rtc_handling.h"
#include "html_handling.h"
#include "spiffs_module.h"

esp_err_t start_rest_server(const char *base_path);


void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    netbiosns_init();
    netbiosns_set_name(CONFIG_EXAMPLE_MDNS_HOST_NAME);

    ESP_ERROR_CHECK(init_spiffs_credentials());

    ESP_ERROR_CHECK(example_connect());

    char recent_ip[50];
    get_recent_ip_from_file(recent_ip);

    extern char public_ip[16];
    get_current_public_ip_address();
    

    if (strcmp(recent_ip, public_ip) == 0)
    {
        printf("Adresy IP są identyczne: %s\n", public_ip);
    }
    else
    {
        printf("Adresy IP są różne: Recent IP: %s, Current IP: %s\n", recent_ip, public_ip);
        if(update_duckdns_domain_ip() == ESP_OK)
        {
            save_ip_to_file(public_ip);
        }
        else
        {
            printf("Nie udało się zaaktualizować IP w domenie duckdns");
        }
        
    }
    
    ESP_ERROR_CHECK(init_spiffs_web());
    ESP_ERROR_CHECK(start_rest_server(CONFIG_EXAMPLE_WEB_MOUNT_POINT));
}
