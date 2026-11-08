#pragma once

// Copy to secrets.h, enter your own values, then enable the cloud connection.
// Never commit secrets.h to GitHub.
#ifndef IOT_ENABLE_CLOUD
#define IOT_ENABLE_CLOUD 0
#endif
#ifndef IOT_ENABLE_SENSOR
#define IOT_ENABLE_SENSOR 1
#endif
#ifndef IOT_ENABLE_BUTTONS
#define IOT_ENABLE_BUTTONS 1
#endif
#define BLYNK_TEMPLATE_ID "TMPL6JTpyMoNy"
#define BLYNK_TEMPLATE_NAME "IoT ESP32 De 2"
#define BLYNK_AUTH_TOKEN "REPLACE_WITH_DEVICE_AUTH_TOKEN"
#define WIFI_SSID "REPLACE_WITH_WIFI_SSID"
#define WIFI_PASSWORD "REPLACE_WITH_WIFI_PASSWORD"
