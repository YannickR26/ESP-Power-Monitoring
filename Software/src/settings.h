#pragma once

// Version
#define VERSION                     "V1.5"

#define DEFAULT_HOSTNAME            "ESP_Monitoring"
#define DEFAULT_MQTTIPSERVER        "192.168.1.75"
#define DEFAULT_MQTTPORTSERVER      1883

#define DEFAULT_NTP_UPDATE_INTERVAL_SEC     (1 * 3600)      // Update time from NTP server every 1 hours
#define DEFAULT_SEND_DATA_INTERVAL_SEC      5               // Log data every 5 secondes

// Mode
#define MODE_MONO   0
#define MODE_TRI_1  1
#define MODE_TRI_2  2
#define MODE_DEBUG  3

// ATM90E32
#define ATM90E32_CS     D8
#define ATM90E32_PM0    D4
#define ATM90E32_PM1    D3
#define ATM90E32_UGAIN  30000
#define ATM90E32_IGAIN  9500

// Relay
#define RELAY_PIN   D0

// LED
#define LEDTIME_NOMQTT  100
#define LEDTIME_WORK    500

// Timezone
#define UTC_OFFSET + 1

// change for different ntp (time servers)
#define NTP_SERVERS "0.fr.pool.ntp.org", "time.nist.gov", "pool.ntp.org"
