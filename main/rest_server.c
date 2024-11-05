/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_chip_info.h"
#include "esp_random.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

#include "esp_sntp.h"

#include "rtc_handling.h"
#include "html_handling.h"

#include "cJSON.h"

static const char *TAG = "REST_SERVER";

void initialize_sntp() {
    // Initialize SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
}

#define BUFFER_SIZE 48         // Define the buffer size

int adc_buffer[BUFFER_SIZE];   // Create an array to store ADC values
int buffer_index = 0;          // Track the current index in the buffer

// Function to add ADC value to the buffer (circular buffer logic)
void add_adc_value_to_buffer(int value) {
    adc_buffer[buffer_index] = value;  // Add the new value at the current index
    buffer_index = (buffer_index + 1) % BUFFER_SIZE;  // Move to the next index in circular manner
}

static const char *REST_TAG = "esp-rest";
#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

/* Nowa funkcja obsługi dla endpointu /temp */
static esp_err_t temp_post_handler(httpd_req_t *req)
{
    char content[100];  // Bufor na odebrane dane; dopasuj rozmiar do potrzeb
    int ret;

    /* Odczytaj treść żądania do bufora */
    ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    content[ret] = '\0';  // Dodaj znak końca stringa

    /* Parsowanie JSON */
    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Nieprawidłowy format JSON");
        return ESP_FAIL;
    }

    /* Pobierz wartość wilgotności */
    cJSON *humidity_item = cJSON_GetObjectItem(json, "humidity");
    if (!cJSON_IsNumber(humidity_item)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Brak poprawnej wartości wilgotności");
        cJSON_Delete(json);
        return ESP_FAIL;
    }

    float humidity = humidity_item->valuedouble;  // Odczytaj wartość wilgotności

    /* Drukowanie otrzymanej wartości wilgotności */
    ESP_LOGI(REST_TAG, "Otrzymana wilgotność: %.2f", humidity);
    add_adc_value_to_buffer(humidity);
    /* Wygeneruj odpowiedź JSON */
    cJSON *response_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(response_json, "received_humidity", humidity);
    const char *response_str = cJSON_Print(response_json);

    /* Wyślij odpowiedź */
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response_str, strlen(response_str));

    /* Zwolnij pamięć */
    cJSON_Delete(json);
    cJSON_Delete(response_json);
    free((void *)response_str);

    return ESP_OK;
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];

    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(REST_TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(REST_TAG, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(REST_TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(REST_TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t data_post_handler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;

    // Wczytujemy dane z zapytania, ale w tym przypadku ich nie używamy
    while (remaining > 0) {
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req); // Timeout
            }
            return ESP_FAIL;
        }
        remaining -= ret;
    }

    // Tworzymy obiekt JSON reprezentujący bufor
    cJSON *json_array = cJSON_CreateArray();
    for (int i = 0; i < BUFFER_SIZE; i++) {
        // Dodajemy wartość z bufora do tablicy JSON, zaczynając od najstarszej
        cJSON_AddItemToArray(json_array, cJSON_CreateNumber(adc_buffer[(buffer_index + i) % BUFFER_SIZE]));
    }

    // Konwertujemy tablicę JSON na string
    const char *response_str = cJSON_Print(json_array);

    // Ustawiamy nagłówek odpowiedzi na JSON
    httpd_resp_set_type(req, "application/json");

    // Dodajemy nagłówki CORS
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");

    httpd_resp_send(req, response_str, strlen(response_str));

    // Czyszczenie pamięci
    cJSON_Delete(json_array);
    free((void *)response_str);

    ESP_LOGI(TAG, "POST request handled, sent buffer data as JSON");

    return ESP_OK;
}

esp_err_t start_rest_server(const char *base_path)
{
    REST_CHECK(base_path, "wrong base path", err);
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(REST_TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

        /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &common_get_uri);

        /* Nowy URI handler dla endpointu /temp */
    httpd_uri_t temp_get_uri = {
        .uri = "/temp",
        .method = HTTP_POST,
        .handler = temp_post_handler,
        .user_ctx = NULL  // jeśli nie potrzebujemy dodatkowego kontekstu
    };
    httpd_register_uri_handler(server, &temp_get_uri);

    httpd_uri_t data_uri = {
        .uri      = "/data",
        .method   = HTTP_POST,
        .handler  = data_post_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &data_uri);
    ESP_LOGI(TAG, "HTTP server started and /data endpoint registered");

    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}
