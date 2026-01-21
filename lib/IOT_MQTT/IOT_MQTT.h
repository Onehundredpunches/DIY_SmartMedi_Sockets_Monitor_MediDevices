#pragma once // Đảm bảo file header chỉ được biên dịch một lần, tránh lỗi lặp khai báo

#include <WiFi.h>                // Thư viện WiFi cho ESP32, phục vụ kết nối mạng không dây
#include <time.h>                // Thư viện thời gian thực, dùng cho đồng bộ NTP và timestamp
#include <PubSubClient.h>        // Thư viện MQTT client, giao tiếp với MQTT broker
#include "SensorHandlers.h"      // Khai báo các struct, biến, hàm xử lý cảm biến và trạng thái thiết bị
#include <ArduinoJson.h>         // Thư viện ArduinoJson, dùng để đóng gói dữ liệu gửi lên MQTT
#include "MD0630T01A_LeakSensor.h" // Khai báo cảm biến rò điện
#include "ES35-SW.h"               // Khai báo cảm biến môi trường (nhiệt độ, độ ẩm)
#include "PZEM016_Lib.h"           // Khai báo cảm biến điện năng (điện áp, dòng, công suất...)

extern WiFiClient espClient;       // Đối tượng quản lý kết nối TCP/IP cho ESP32
extern PubSubClient mqttClient;    // Đối tượng MQTT client, dùng để publish/subscribe dữ liệu

extern const char* WIFI_SSID;      // Tên mạng WiFi cần kết nối
extern const char* WIFI_PASSWORD;  // Mật khẩu WiFi
extern const char* MQTT_SERVER;    // Địa chỉ MQTT broker/server
extern const int   MQTT_PORT;      // Cổng kết nối MQTT broker

// Khai báo hệ thống topic MQTT để publish dữ liệu
extern const char* topic_elec_cart;         // Topic dữ liệu dòng rò tổng
extern const char* topic_env_cart;          // Topic dữ liệu môi trường phongf và ngưỡng chung toàn thiết bị

extern const char* topic_elec_auo;          // Topic dữ liệu điện màn hình AUO
extern const char* topic_env_auo;           // Topic dữ liệu môi trường màn hình AUO

extern const char* topic_elec_img1s;        // Topic dữ liệu điện ccu image1 s
extern const char* topic_env_img1s;         // Topic dữ liệu môi trường ccu image1 s

extern const char* topic_elec_img1hub;      // Topic dữ liệu điện ccu image 1 hub
extern const char* topic_env_img1hub;       // Topic dữ liệu môi trường ccu image 1 hub

extern const char* topic_elec_imgtripal;    // Topic dữ liệu điện ccu image tricam pal
extern const char* topic_env_imgtripal;     // Topic dữ liệu môi trường ccu image tricam pal

extern const char* topic_elec_xenon300;     // Topic dữ liệu điện nguồn sáng Xenon 300
extern const char* topic_env_xenon300;      // Topic dữ liệu môi trường nguồn sáng Xenon 300

extern const char* topic_elec_co2ui400;     // Topic dữ liệu điện thiết bị bơm CO2 UI400
extern const char* topic_env_co2ui400;      // Topic dữ liệu môi trường thiết bị bơm CO2 UI400

// Khai báo các hàm xử lý chính cho module IoT MQTT
extern void IOT_MQTT_setupWifi(); // Hàm kết nối WiFi, tự động retry nếu thất bại, log trạng thái lên Serial
extern void IOT_MQTT_setupTime(); // Hàm đồng bộ thời gian thực (NTP), phục vụ timestamp cho dữ liệu
extern void IOT_MQTT_setupMQTT(PubSubClient& client); // Hàm kết nối MQTT server, thiết lập client, topic, callback
extern void IOT_MQTT_publishAll(PubSubClient& client, const acLeakChangedFlags&, const teHuCartChangedFlags&, const teHuDecviceChangedFlags&, const PZEMChangedFlags&); // Publish toàn bộ dữ liệu cảm biến nếu có thay đổi
extern void IOT_MQTT_ensureWifiConnected(); // Đảm bảo kết nối WiFi luôn duy trì, tự động reconnect nếu mất kết nối
extern void IOT_MQTT_ensureConnected(PubSubClient& client); // Đảm bảo kết nối MQTT luôn duy trì, tự động reconnect nếu mất kết nối
extern String IOT_MQTT_getTimestamp(); // Lấy timestamp hiện tại dạng chuỗi ISO8601 để gắn vào dữ liệu publish
// extern void IOT_MQTT_loadOperatingTime(); // (Đã loại bỏ) Hàm cũ dùng để load thời gian hoạt động từ EEPROM, không dùng nữa