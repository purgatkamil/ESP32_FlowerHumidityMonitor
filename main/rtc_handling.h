#include <stdio.h>

#include "sys/time.h"
#include "time.h"
#include "esp_sntp.h"
#include "esp_log.h"


void set_time(void);
void read_time(void);
void sntp_sync_time_own(void);