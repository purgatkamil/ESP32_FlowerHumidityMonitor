#include "rtc_handling.h"

void set_time(void)
{
    struct timeval tv;
    struct tm timeinfo;

    timeinfo.tm_year = 2024 - 1900; // Rok 2024
    timeinfo.tm_mon = 10 - 1;       // Październik (miesiące zaczynają się od 0)
    timeinfo.tm_mday = 6;           // Dzień miesiąca
    timeinfo.tm_hour = 17;           // Godzina
    timeinfo.tm_min = 4;            // Minuta
    timeinfo.tm_sec = 0;            // Sekunda

    time_t t = mktime(&timeinfo);

    tv.tv_sec = t;
    tv.tv_usec = 0;

    settimeofday(&tv, NULL);

}

void read_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    time_t now = tv.tv_sec;

    struct tm timeinfo;

    localtime_r(&now, &timeinfo);

    printf("Aktualny czas: %d-%02d-%02d %02d:%02d:%02d\n", timeinfo.tm_year, timeinfo.tm_mon,
    timeinfo.tm_mday, timeinfo.tm_hour,
    timeinfo.tm_min, timeinfo.tm_sec);
}

static volatile bool sntp_synced = false;

// Funkcja callback wywoływana po zakończeniu synchronizacji
void sntp_sync_callback(struct timeval *tv)
{
    sntp_synced = true;
}

void sntp_sync_time_own(void)
{
    // Ustawienia SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    // Czekaj na synchronizację
    int attempts = 0;
    while (!sntp_synced && attempts < 10) // Oczekiwanie maksymalnie 10 razy
    {
        for (volatile int i = 0; i < 1000000; i++) {} // Prosta pętla opóźniająca
        attempts++;
    }

    if (!sntp_synced) {
        printf("Nie udało się zsynchronizować czasu.\n");
        return;
    }

    // Odczytaj czas po synchronizacji
    struct timeval tv;
    gettimeofday(&tv, NULL);

    time_t now = tv.tv_sec;

    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    // Poprawne obliczenie roku i miesiąca
    printf("Aktualny czas: %d-%02d-%02d %02d:%02d:%02d\n",
           timeinfo.tm_year + 1900,
           timeinfo.tm_mon + 1,
           timeinfo.tm_mday,
           timeinfo.tm_hour,
           timeinfo.tm_min,
           timeinfo.tm_sec);
}