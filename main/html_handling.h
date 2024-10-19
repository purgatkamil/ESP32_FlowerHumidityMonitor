#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "esp_crt_bundle.h"

#include "esp_http_client.h"

#include "spiffs_module.h"

esp_err_t _http_event_handler(esp_http_client_event_t *evt);
esp_err_t update_duckdns_domain_ip(void);

esp_err_t _http_event_handler_for_getting_ip(esp_http_client_event_t *evt);
void get_current_public_ip_address();