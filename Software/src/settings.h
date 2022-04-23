#pragma once

// Version
#define VERSION                     "V2.1.6"
#define BUILD_DATE                  String((__DATE__) + String(" ") + (__TIME__))

#define DEFAULT_HOSTNAME            "ESP_Monitoring"
#define DEFAULT_MQTTIPSERVER        "192.168.1.201"
#define DEFAULT_MQTTPORTSERVER      1883

#define DEFAULT_SAVE_DATA_INTERVAL_SEC      (1 * 3600)  // Update time from NTP server and save data every 1 hours
#define DEFAULT_SEND_DATA_INTERVAL_SEC      15          // Send data every 15 secondes
#define DEFAULT_CURRENT_CLAMP               30          // 30A current clamp by default

// Mode
#define MODE_MONO   0
#define MODE_TRI_1  1
#define MODE_TRI_2  2
#define MODE_DEBUG  3
#define MODE_CALIB  4

// ATM90E32
#define ATM90E32_CS         D8
#define ATM90E32_PM0        D4
#define ATM90E32_PM1        D3
#define ATM90E32_UGAIN      30500
#define ATM90E32_IGAIN      310     // x capacité de la pince amperemetrique (defaut 30A)

// Relay
#define RELAY_PIN           D0

// LED
#define LED_PIN             LED_BUILTIN
#define LED_TIME_NOMQTT     100
#define LED_TIME_WORK       500

// NTP and Timezone
#define NTP_SERVERS "0.fr.pool.ntp.org", "time.nist.gov", "pool.ntp.org"
// List of timezone: https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"
