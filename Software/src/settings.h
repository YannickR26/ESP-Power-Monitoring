#pragma once

// Setup Wifi
#define WIFI_SSID   "Internet-Maison"
#define WIFI_PASS   "MaisonMoreauRichardot"
#define WIFI_SSID2  "WIFI_CBEE"
#define WIFI_PASS2  "iletaitunefoisunpetitbateau"
// Setup Wifi in AP mode
#define WIFI_AP_HOTSTNAME   "ESP_Monitoring"
#define WIFI_AP_PASSWORD    "ESP_Monitoring"

#define DEFAULT_HOSTNAME            "ESP_Monitoring"
#define DEFAULT_MQTTIPSERVER        "192.168.1.10"
#define DEFAULT_MQTTPORTSERVER      1883

#define DEFAULT_NTP_UPDATE_INTERVAL_SEC     (1 * 3600)      // Update time from NTP server every 1 hours
#define DEFAULT_SEND_DATA_INTERVAL_SEC      5//60              // Log data every 1 minutes
#define DEFAULT_SCREEN_UPDATE_INTERVAL_SEC  5               // Update screen every 5 secondes

// Mode
#define MODE_MONO   0
#define MODE_TRI    1

// TFT
#define TFT_CS     D4
#define TFT_DC     D3
#define TFT_RST    -1  // you can also connect this to the Arduino reset

// ATM90E32
#define ATM90E32_CS D8

// Timezone
#define UTC_OFFSET + 1

// change for different ntp (time servers)
#define NTP_SERVERS "0.fr.pool.ntp.org", "time.nist.gov", "pool.ntp.org"
