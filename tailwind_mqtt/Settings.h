// ===============================================================================
// Update/Add any #define values to match your build and board type if not using D1 Mini
// ===============================================================================

#define WIFIMODE 2                            // 0 = Only Soft Access Point, 1 = Only connect to local WiFi network with UN/PW, 2 = Both
#define MQTTMODE 1                            // 0 = Disable MQTT, 1 = Enable (will only be enabled if WiFi mode = 1 or 2 - broker must be on same network)
#define MQTTCLIENT "TailwindClient"           // MQTT Client Name
#define MQTT_TOPIC_SUB "cmnd/tailwind"        // Default MQTT subscribe topic
#define MQTT_TOPIC_PUB "stat/tailwind"        // Default MQTT publish topic
#define OTA_HOSTNAME "tailwindOTA"            // Hostname to broadcast as port in the IDE of OTA Updates

// ---------------------------------------------------------------------------------------------------------------
// Options - Defaults upon boot-up or any other custom ssttings
// ---------------------------------------------------------------------------------------------------------------
bool ota_flag = true;                    // Must leave this as true for board to broadcast port to IDE upon boot
uint16_t ota_boot_time_window = 2500;    // minimum time on boot for IP address to show in IDE ports, in millisecs
uint16_t ota_time_window = 20000;        // time to start file upload when ota_flag set to true (after initial boot), in millsecs
uint16_t tailwind_poll = 10000;          // How often (in millis) to poll Tailwind for status- DO NOT SET TO LESS THAN 1000, 5000 or higher recommended
