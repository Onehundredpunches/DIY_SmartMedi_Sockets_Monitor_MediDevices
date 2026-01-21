#include "SensorHandlers.h" // Khai báo các hàm, biến quản lý cảm biến và trạng thái thiết bị
#include "IOT_MQTT.h"       // Khai báo các hàm xử lý MQTT (kết nối, publish dữ liệu)
unsigned long lastReadTime = 0;          // Biến lưu thời điểm lần đọc dữ liệu gần nhất
const unsigned long readInterval = 5000; // Chu kỳ đọc dữ liệu (ms), tránh đọc quá nhanh gây quá tải

void setup()
{
    Serial.begin(9600); // Khởi tạo giao tiếp Serial để debug, log trạng thái hệ thống

    // 1. Khởi tạo các thư viện quản lý cảm biến, còi, nút nhấn, trạng thái
    SensorHandlers_init(); // Khởi tạo các biến, struct, trạng thái cảm biến (counter, flag, threshold...)
    initBuzzer();          // Khởi tạo module cảnh báo âm thanh (buzzer)

    // 2. Khởi tạo các module cảm biến phần cứng
    MD0630T01A_init(); // Khởi tạo cảm biến rò điện
    ES35SW_init();     // Khởi tạo cảm biến môi trường (nhiệt độ, độ ẩm)
    PZEM016_init();    // Khởi tạo cảm biến điện năng (điện áp, dòng, công suất...)

    // 3. Khởi tạo kết nối mạng
    IOT_MQTT_setupWifi();           // Kết nối WiFi, retry nếu thất bại, log trạng thái lên Serial
    IOT_MQTT_setupTime();           // Đồng bộ thời gian thực (NTP), phục vụ timestamp cho dữ liệu
    IOT_MQTT_setupMQTT(mqttClient); // Kết nối MQTT server, thiết lập client, topic, callback

    // op_time_counter_reset(&op_time_auo_display);
    // op_time_counter_reset(&op_time_ccu_img1s);
    // op_time_counter_reset(&op_time_ccu_img1hub);
    // op_time_counter_reset(&op_time_ccu_imgtricpal);
    // op_time_counter_reset(&op_time_xenon_300);
    // op_time_counter_reset(&op_time_UI400);

    Serial.println("=== SYSTEM READY ==="); // Thông báo hệ thống đã sẵn sàng
}
void loop()
{
    IOT_MQTT_ensureWifiConnected(); // Đảm bảo kết nối WiFi luôn duy trì, tự động reconnect nếu mất kết nối
    
    // Kiểm tra đủ chu kỳ mới thực hiện polling, tránh spam xử lý ...
    if (millis() - lastReadTime >= readInterval)
    {
        lastReadTime = millis(); // Cập nhật thời điểm polling mới

        bool warning = false; // Biến trạng thái cảnh báo, sẽ được cập nhật bởi các hàm xử lý cảm biến

        acLeakChangedFlags leakChanged = {};   // Struct lưu trạng thái thay đổi của cảm biến rò điện
        teHuCartChangedFlags envCartChanged = {}; // Struct lưu trạng thái thay đổi của cảm biến môi trường (nhiệt độ, độ ẩm)
        teHuDecviceChangedFlags envDeviceChanged = {}; // Struct lưu trạng thái thay đổi        
        PZEMChangedFlags pzemChanged = {}; // Struct lưu trạng thái thay đổi của cảm biến điện năng (điện áp, dòng, công suất...)

        Serial.println("\n================= DATA UPDATE ================="); // Log bắt đầu chu kỳ cập nhật dữ liệu

        handleLeakSensor(warning, leakChanged); // Đọc và xử lý cảm biến rò điện, cập nhật cảnh báo và flag thay đổi
        delay(100);                            // Delay ngắn để tránh nhiễu khi đọc cảm biến

        handleES35SW(warning, envCartChanged, envDeviceChanged); // Đọc và xử lý cảm biến môi trường, cập nhật cảnh báo và flag thay đổi
        delay(100);

        handlePZEMSensors(warning, pzemChanged); // Đọc và xử lý cảm biến điện năng, cập nhật cảnh báo và flag thay đổi
        delay(100);

        IOT_MQTT_ensureConnected(mqttClient); // Đảm bảo kết nối MQTT luôn duy trì, tự động reconnect nếu mất kết nối

        IOT_MQTT_publishAll(mqttClient, leakChanged, envCartChanged, envDeviceChanged, pzemChanged); // Publish dữ liệu lên các topic MQTT nếu có thông số thay đổi

        Serial.print("Warning Status: "); // Log trạng thái cảnh báo hiện tại
        Serial.println(warning ? "YES" : "NO");

        handleWarningBeep(warning); // Xử lý cảnh báo còi (buzzer) dựa trên trạng thái cảnh báo hiện tại

        printSensorSnapshot(); // In toàn bộ snapshot dữ liệu cảm biến lên Serial để debug, kiểm tra hệ thống
    }
}