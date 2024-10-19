#include "html_handling.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048
static const char *TAG = "HTTP_CLIENT";

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    const int buffer_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        output_buffer = (char *) malloc(buffer_len);
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (buffer_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}

esp_err_t update_duckdns_domain_ip(void)
{
    esp_http_client_config_t config = {
        .url = "https://nouser:e8bd3b4f-95e4-4846-b9c6-3925af85ae7b@www.duckdns.org/nic/update?hostname=fhsserver&myip=&wildcard=NOCHG&mx=NOCHG&backmx=NOCHG",
        .event_handler = _http_event_handler,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTPS Status = %d, content_length = %"PRIu64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
        return ESP_FAIL;
    }
    esp_http_client_cleanup(client);
    
    return ESP_OK;
}

char public_ip[16] = {0};

esp_err_t _http_event_handler_for_getting_ip(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        if (!esp_http_client_is_chunked_response(evt->client)) {
            // Wyświetl pobrane IP
            strncpy(public_ip, (char*)evt->data, evt->data_len);
            public_ip[evt->data_len] = '\0';  // Dodaj terminator łańcucha
            printf("Public IP: %.*s\n", evt->data_len, (char*)evt->data);
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

#define TAG2 "Public_IP"

void get_current_public_ip_address() {
    // Konfiguracja klienta HTTP
    esp_http_client_config_t config2 = {
        .url = "https://api.ipify.org",  // Serwis zwracający publiczny IP
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = _http_event_handler_for_getting_ip,
    };
    
    esp_http_client_handle_t client2 = esp_http_client_init(&config2);

    // Wykonanie zapytania HTTP
    esp_err_t err = esp_http_client_perform(client2);
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG2, "HTTP GET Status = %d, content_length = %lld",
                 esp_http_client_get_status_code(client2),
                 esp_http_client_get_content_length(client2));
    } else {
        ESP_LOGE(TAG2, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    // Zakończenie klienta HTTP
    esp_http_client_cleanup(client2);
}

