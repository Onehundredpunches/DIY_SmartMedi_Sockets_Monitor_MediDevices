#pragma once // Chỉ biên dịch file này một lần, tránh lỗi lặp khai báo

#include <WiFi.h>                // Thư viện WiFi cho ESP32, phục vụ kết nối mạng
#include <PubSubClient.h>        // Thư viện MQTT client, dùng để giao tiếp với MQTT broker
#include <EEPROM.h>              // Thư viện EEPROM, dùng lưu dữ liệu lâu dài như thời gian hoạt động
#include "operating_time_manager.h" // Quản lý bộ đếm thời gian hoạt động cho từng thiết bị
#include "MD0630T01A_LeakSensor.h"  // Khai báo cảm biến rò điện
#include "PZEM016_Lib.h"             // Khai báo cảm biến điện năng (điện áp, dòng, công suất...)
#include "ES35-SW.h"                 // Khai báo cảm biến môi trường (nhiệt độ, độ ẩm)
#include "Buzzer.h"                  // Khai báo module cảnh báo âm thanh


// Struct lưu trạng thái thay đổi của cảm biến nhiệt độ, độ ẩm
struct teHuCartChangedFlags {
    bool temperature;                   // Có thay đổi nhiệt độ không
    bool humidity;                      // Có thay đổi độ ẩm không

    bool overRoomTemp;                  // Có thay đổi trạng thái quá nhiệt phòng không
    bool underRoomTemp;                 // Có thay đổi trạng thái dưới nhiệt phòng không
    bool overRoomHumi;                  // Có thay đổi trạng thái quá ẩm phòng không
    bool underRoomHumi;                 // Có thay đổi trạng thái dưới ẩm phòng không

    bool overComDeviceTemp;             // Có thay đổi trạng thái quá nhiệt thiết bị chung không
    bool underComDeviceTemp;            // Có thay đổi trạng thái dưới nhiệt thiết bị chung không
    bool overComDeviceHumi;             // Có thay đổi trạng thái quá ẩm thiết bị chung không
    bool underComDeviceHumi;            // Có thay đổi trạng thái dưới ẩm thiết bị chung không
};

// Struct lưu trạng thái thay đổi của cảm biến nhiệt độ, độ ẩm
struct teHuDecviceChangedFlags { 
    bool overDeviceTemp[NUM_DEVICES];   // Có thay đổi trạng thái quá nhiệt hoạt động từng thiết bị không
    bool underDeviceTemp[NUM_DEVICES];  // Có thay đổi trạng thái dưới nhiệt hoạt động từng thiết bị không
    bool overDeviceHumi[NUM_DEVICES];   // Có thay đổi trạng thái quá ẩm hoạt động từng thiết bị không
    bool underDeviceHumi[NUM_DEVICES];  // Có thay đổi trạng thái dưới ẩm hoạt động từng thiết bị không
};

// Struct lưu trạng thái thay đổi của cảm biến dòng rò
struct acLeakChangedFlags {
    bool changeLeakACCurrent; // Có thay đổi dòng rò AC không
    bool leakStatus;        // Có thay đổi trạng thái rò điện không
    bool acLeakCurrent;    // Có thay đổi dòng rò điện không
    bool softWarning;      // Có thay đổi trạng thái cảnh báo dòng rò nhẹ không
    bool strongWarning;    // Có thay đổi trạng thái cảnh báo dòng rò mạnh không
};

// Struct lưu trạng thái thay đổi của cảm biến điện năng cho từng thiết bị
struct PZEMChangedFlags {
    bool voltage[NUM_DEVICES];        // Có thay đổi điện áp không (mỗi thiết bị)
    bool current[NUM_DEVICES];        // Có thay đổi dòng điện không
    bool power[NUM_DEVICES];          // Có thay đổi công suất không
    bool frequency[NUM_DEVICES];      // Có thay đổi tần số không
    bool pf[NUM_DEVICES];             // Có thay đổi hệ số công suất không
    bool machineState[NUM_DEVICES];          // Có thay đổi trạng thái hoạt động không
    bool overVoltage[NUM_DEVICES];    // Có thay đổi trạng thái quá áp không
    bool overCurrent[NUM_DEVICES];    // Có thay đổi trạng thái quá dòng không
    bool overPower[NUM_DEVICES];      // Có thay đổi trạng thái quá công suất không
    bool underVoltage[NUM_DEVICES];   // Có thay đổi trạng thái dưới áp không
    bool socketState[NUM_DEVICES];      // Có thay đổi trạng thái mất nguồn không
    bool operating_time[NUM_DEVICES]; // Có thay đổi thời gian hoạt động không
};

// Khai báo biến counter thời gian hoạt động cho từng thiết bị, dùng để lưu/đọc EEPROM
extern OperatingTimeCounter op_time_auo_display; // Thiết bị màn hình AUO
extern OperatingTimeCounter op_time_ccu_img1s;     // Thiết bị CCU image1s
extern OperatingTimeCounter op_time_ccu_img1hub;     // Thiết bị CCU image 1 hub
extern OperatingTimeCounter op_time_ccu_imgtricpal;     // Thiết bị CCU image tricam pal
extern OperatingTimeCounter op_time_xenon_300;   // Thiết bị nguồn sáng xenon 300
extern OperatingTimeCounter op_time_UI400;     // Thiết bị bơm CO2 UI400

// Khai báo thời gian chờ beep còi tiếp theo sau khi phát hiện có thông số vượt ngưỡng
extern unsigned long lastWarningBeepTime;
extern const unsigned long warningBeepInterval; 

// Khai báo các hàm xử lý cảm biến và trạng thái thiết bị
extern void SensorHandlers_init(void); // Khởi tạo các biến, struct, trạng thái cảm biến
extern void handleLeakSensor(bool &warning, acLeakChangedFlags &changed); // Xử lý cảm biến rò điện, cập nhật cảnh báo và flag thay đổi
extern void handleES35SW(bool &warning, teHuCartChangedFlags &cartChanged, teHuDecviceChangedFlags &deviceChanged);     // Xử lý cảm biến môi trường, cập nhật cảnh báo và flag thay đổi
extern void handlePZEMSensors(bool &warning, PZEMChangedFlags &changed); // Xử lý cảm biến điện năng, cập nhật cảnh báo và flag thay đổi
extern void printSensorSnapshot(); // In toàn bộ snapshot dữ liệu cảm biến lên Serial để debug, kiểm tra hệ thống
extern void handleWarningBeep(bool warning); // Xử lý cảnh báo còi khi có cảnh báo từ cảm biến