#include "IOT_MQTT.h"

// Khởi tạo đối tượng WiFiClient để giao tiếp TCP/IP qua WiFi
WiFiClient espClient;

// Khởi tạo đối tượng PubSubClient để giao tiếp với MQTT broker qua espClient
PubSubClient mqttClient(espClient);

// Thông tin cấu hình WiFi
const char *WIFI_SSID = "HOPT-R&D_2.4G"; // Tên mạng WiFi cần kết nối
const char *WIFI_PASSWORD = "H_rd2024t"; // Mật khẩu WiFi

// Thông tin cấu hình WiFi backup
const char *BK_WIFI_SSID = "OptiCareX-HOPT"; // Tên mạng WiFi cần kết nối
const char *BK_WIFI_PASSWORD = "Iomt2025t";  // Mật khẩu WiFi

// // Test
// // Thông tin cấu hình WiFi
// const char *BK_WIFI_SSID = "HOPT-R&D_2.4G"; // Tên mạng WiFi cần kết nối
// const char *BK_WIFI_PASSWORD = "H_rd2024t"; // Mật khẩu WiFi

// const char *WIFI_SSID = "OptiCareX-HOPT"; // Tên mạng WiFi cần kết nối
// const char *WIFI_PASSWORD = "Iomt2025t";  // Mật khẩu WiFi

// Thông tin cấu hình MQTT broker
const char *MQTT_SERVER = "broker.hivemq.com"; // Địa chỉ MQTT broker
const int MQTT_PORT = 1883;                    // Cổng MQTT broker (mặc định 1883)

// Khai báo hệ thống topic MQTT để publish dữ liệu
const char *topic_elec_cart = "hopt/floor2/rd/cart01/elec/cart"; // Topic dữ liệu dòng rò tổng
const char *topic_env_cart = "hopt/floor2/rd/cart01/envi/cart";  // Topic dữ liệu môi trường phongf và ngưỡng chung toàn thiết bị

const char *topic_elec_auo = "hopt/floor2/rd/cart01/elec/auo"; // Topic dữ liệu điện màn hình AUO
const char *topic_env_auo = "hopt/floor2/rd/cart01/envi/auo";  // Topic dữ liệu môi trường màn hình AUO

const char *topic_elec_img1s = "hopt/floor2/rd/cart01/elec/image1s"; // Topic dữ liệu điện ccu image1 s
const char *topic_env_img1s = "hopt/floor2/rd/cart01/envi/image1s";  // Topic dữ liệu môi trường ccu image1 s

const char *topic_elec_img1hub = "hopt/floor2/rd/cart01/elec/image1hub"; // Topic dữ liệu điện ccu image 1 hub
const char *topic_env_img1hub = "hopt/floor2/rd/cart01/envi/image1hub";  // Topic dữ liệu môi trường ccu image 1 hub

const char *topic_elec_imgtripal = "hopt/floor2/rd/cart01/elec/imagetricpal"; // Topic dữ liệu điện ccu image tricam pal
const char *topic_env_imgtripal = "hopt/floor2/rd/cart01/envi/imagetricpal";  // Topic dữ liệu môi trường ccu image tricam pal

const char *topic_elec_xenon300 = "hopt/floor2/rd/cart01/elec/xenon300"; // Topic dữ liệu điện nguồn sáng Xenon 300
const char *topic_env_xenon300 = "hopt/floor2/rd/cart01/envi/xenon300";  // Topic dữ liệu môi trường nguồn sáng Xenon 300

const char *topic_elec_co2ui400 = "hopt/floor2/rd/cart01/elec/co2ui400"; // Topic dữ liệu điện thiết bị bơm CO2 UI400
const char *topic_env_co2ui400 = "hopt/floor2/rd/cart01/envi/co2ui400";  // Topic dữ liệu môi trường thiết bị bơm CO2 UI400

// Hàm kết nối WiFi, xử lý ngoại lệ khi không kết nối được
void IOT_MQTT_setupWifi()
{
    delay(10); // Đợi một chút trước khi bắt đầu kết nối
    Serial.println("Starting WiFi connection...");

    // Thử kết nối WiFi chính
    Serial.print("Connecting to primary WiFi: ");
    Serial.println(WIFI_SSID);
    WiFi.mode(WIFI_STA);                  // Đặt chế độ WiFi là Station (client)
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // Kết nối WiFi chính

    int retry = 0; // Biến đếm số lần thử kết nối
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);        // Đợi 0.5s giữa các lần kiểm tra
        Serial.print("."); // In dấu chấm để báo tiến trình
        if (++retry > 40)  // Nếu quá 40 lần (20s) mà chưa kết nối được
        {
            Serial.println("\nPrimary WiFi connect failed. Trying backup WiFi...");
            break; // Thoát khỏi vòng lặp để thử WiFi phụ
        }
    }

    // Nếu không kết nối được WiFi chính, thử kết nối WiFi phụ
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.print("Connecting to backup WiFi: ");
        Serial.println(BK_WIFI_SSID);
        WiFi.begin(BK_WIFI_SSID, BK_WIFI_PASSWORD); // Kết nối WiFi phụ

        retry = 0; // Reset biến đếm số lần thử kết nối
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);        // Đợi 0.5s giữa các lần kiểm tra
            Serial.print("."); // In dấu chấm để báo tiến trình
            if (++retry > 40)  // Nếu quá 40 lần (20s) mà chưa kết nối được
            {
                Serial.println("\nBackup WiFi connect failed. Restarting...");
                ESP.restart(); // Khởi động lại ESP để thử lại từ đầu
            }
        }
    }

    // Nếu kết nối thành công, in địa chỉ IP
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

// Hàm đảm bảo WiFi luôn được kết nối, tự động reconnect nếu mất kết nối
void IOT_MQTT_ensureWifiConnected()
{
    if (WiFi.status() != WL_CONNECTED) // Nếu WiFi không kết nối
    {
        Serial.println("WiFi disconnected. Reconnecting...");
        WiFi.disconnect();                    // Ngắt kết nối hiện tại (nếu có)
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // Thử kết nối lại WiFi chính

        int retry = 0;
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500); // Đợi 0.5s giữa các lần kiểm tra
            Serial.print(".");
            if (++retry > 40) // Nếu quá 20 giây mà không kết nối được
            {
                Serial.println("\nPrimary WiFi failed. Trying backup WiFi...");
                WiFi.disconnect();                          // Ngắt kết nối hiện tại
                WiFi.begin(BK_WIFI_SSID, BK_WIFI_PASSWORD); // Thử kết nối WiFi phụ

                retry = 0;
                while (WiFi.status() != WL_CONNECTED)
                {
                    delay(500);
                    Serial.print(".");
                    if (++retry > 40) // Nếu quá 20 giây mà không kết nối được
                    {
                        Serial.println("\nBackup WiFi failed. Restarting...");
                        ESP.restart(); // Khởi động lại ESP
                    }
                }
                break;
            }
        }

        Serial.println("\nWiFi reconnected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }
}

// Hàm đồng bộ thời gian hệ thống với NTP server, xử lý ngoại lệ khi chưa lấy được thời gian
void IOT_MQTT_setupTime()
{
    // Thiết lập múi giờ GMT+7 và các NTP server
    // configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // GMT+7, đổi nếu cần
    configTime(7 * 3600, 0, "asia.pool.ntp.org", "time.google.com"); // GMT+7, đổi nếu cần

    Serial.print("Waiting for NTP time sync...");
    time_t now = time(nullptr);          // Lấy thời gian hiện tại (epoch)
    unsigned long start = millis();      // Lấy thời gian bắt đầu chờ
    const unsigned long timeout = 10000; // Thời gian chờ tối đa 10 giây
    // Nếu thời gian chưa được đồng bộ (giá trị nhỏ), tiếp tục chờ
    while (now < 8 * 3600 * 2)
    {
        delay(100);          // Đợi 0.1s giữa các lần kiểm tra
        Serial.print(".");   // In dấu chấm để báo tiến trình
        now = time(nullptr); // Lấy lại thời gian

        if (millis() - start > timeout) // Nếu vượt quá thời gian chờ
        {
            Serial.println("\nNTP time sync timeout. Using default time.");
            return;
        }
    }
    Serial.println(" done!"); // Đã đồng bộ thời gian thành công
}

// ========== Helper: Get timestamp ==========
// Hàm lấy timestamp hiện tại dưới dạng chuỗi, dùng để ghi nhận thời điểm gửi dữ liệu
String IOT_MQTT_getTimestamp()
{
    time_t now = time(nullptr);                                 // Lấy thời gian hiện tại (epoch)
    struct tm timeinfo;                                         // Khai báo cấu trúc lưu thông tin thời gian
    localtime_r(&now, &timeinfo);                               // Chuyển đổi epoch sang dạng cấu trúc thời gian cục bộ
    char buf[32];                                               // Bộ đệm lưu chuỗi thời gian
    strftime(buf, sizeof(buf), "%H:%M:%S %d/%m/%Y", &timeinfo); // Định dạng chuỗi thời gian
    return String(buf);                                         // Trả về chuỗi thời gian đã định dạng
}

// Hàm thiết lập thông số kết nối MQTT cho client
void IOT_MQTT_setupMQTT(PubSubClient &client)
{
    client.setServer(MQTT_SERVER, MQTT_PORT); // Thiết lập địa chỉ và cổng MQTT broker
    client.setBufferSize(1024);               // Thiết lập kích thước bộ đệm cho gói tin MQTT
}

// Hàm đảm bảo kết nối MQTT luôn được duy trì, tự động reconnect nếu mất kết nối
void IOT_MQTT_ensureConnected(PubSubClient &client)
{
    while (!client.connected()) // Nếu chưa kết nối tới broker
    {
        Serial.print("Connecting to MQTT...");                          // In thông báo đang kết nối
        String clientId = "ESP32Client-" + String(random(0xffff), HEX); // Tạo clientId ngẫu nhiên
        if (client.connect(clientId.c_str()))                           // Thử kết nối với clientId vừa tạo
        {
            Serial.println("connected"); // Kết nối thành công
        }
        else
        {
            Serial.print("failed, rc="); // Kết nối thất bại, in mã lỗi
            Serial.print(client.state());
            Serial.println(" try again in 5s"); // Thông báo sẽ thử lại sau 5 giây
            delay(5000);                        // Đợi 5 giây trước khi thử lại
        }
    }
    client.loop(); // Gọi vòng lặp MQTT để xử lý các sự kiện
}

// ========== Publish only changed fields ==========
// Hàm publish dữ liệu lên MQTT nếu có thay đổi, chỉ gửi khi doc có dữ liệu
static void publishIfChanged(PubSubClient &client, const char *topic, JsonDocument &doc)
{
    if (doc.size() == 0) // Nếu không có trường nào thay đổi thì không gửi
        return;
    char jsonBuffer[512];                                       // Bộ đệm lưu chuỗi JSON
    serializeJson(doc, jsonBuffer);                             // Chuyển đổi doc sang chuỗi JSON
    Serial.printf("Publishing to %s: %s\n", topic, jsonBuffer); // In ra dữ liệu sẽ gửi để debug
    client.publish(topic, jsonBuffer);                          // Gửi dữ liệu lên topic MQTT
}

// Biến lưu thời gian lần đầu phát hiện socketPowerLost[id] = true
static unsigned long socketPowerLostFirstDetected[NUM_DEVICES] = {0};
static bool Pzems_firstPublish[NUM_DEVICES] = {true, true, true, true, true, true}; // Biến đánh dấu lần đầu publish dữ liệu PZEMs

// Hàm publish dữ liệu thiết bị lên MQTT, chỉ gửi khi có trường thay đổi
static void publishDeviceData(PubSubClient &client, const char *elecTopic, const char *envTopic, int id, const PZEMChangedFlags &eleChanged, const teHuDecviceChangedFlags &envChanged)
{
    // Tăng kích thước doc để tránh thiếu bộ nhớ khi gửi nhiều trường
    ArduinoJson::StaticJsonDocument<512> elecDoc;
    ArduinoJson::StaticJsonDocument<256> envDoc;

    // Lần đầu publish: gửi toàn bộ trường
    if (Pzems_firstPublish[id])
    {
        elecDoc["voltage"] = sensorData[id].valid ? sensorData[id].voltage : pzemVoltageCalib[id];
        elecDoc["current"] = sensorData[id].current;
        elecDoc["power"] = sensorData[id].power;
        elecDoc["frequency"] = sensorData[id].frequency;
        elecDoc["power_factor"] = sensorData[id].pf;
        elecDoc["machine_state"] = sensorData[id].machineState;
        elecDoc["over_voltage"] = overVoltage[id];
        elecDoc["over_current"] = overCurrent[id];
        elecDoc["over_power"] = overPower[id];
        elecDoc["under_voltage"] = underVoltage[id];
        elecDoc["socket_state"] = socketState[id];

        // Thời gian hoạt động
        char buf[16];
        switch (id)
        {
        case AUO_DISPLAY:
            op_time_counter_get_formatted(&op_time_auo_display, buf, sizeof(buf));
            break;
        case CCU_IMAGE1_S:
            op_time_counter_get_formatted(&op_time_ccu_img1s, buf, sizeof(buf));
            break;
        case CCU_IMAGE_1_HUB:
            op_time_counter_get_formatted(&op_time_ccu_img1hub, buf, sizeof(buf));
            break;
        case CCU_TRICAM_PAL:
            op_time_counter_get_formatted(&op_time_ccu_imgtricpal, buf, sizeof(buf));
            break;
        case XENON_300:
            op_time_counter_get_formatted(&op_time_xenon_300, buf, sizeof(buf));
            break;
        case ENDOFLATOR_UI400:
            op_time_counter_get_formatted(&op_time_UI400, buf, sizeof(buf));
            break;
        default:
            buf[0] = 0;
            break;
        }
        elecDoc["operating_time"] = buf;
        elecDoc["timestamp"] = IOT_MQTT_getTimestamp();
        publishIfChanged(client, elecTopic, elecDoc);

        // Môi trường thiết bị
        envDoc["over_temp_max"] = es35swDevice[id].over_temp_max;
        envDoc["under_temp_min"] = es35swDevice[id].under_temp_min;
        envDoc["over_humi_max"] = es35swDevice[id].over_humi_max;
        envDoc["under_humi_min"] = es35swDevice[id].under_humi_min;
        envDoc["timestamp"] = IOT_MQTT_getTimestamp();
        publishIfChanged(client, envTopic, envDoc);

        Pzems_firstPublish[id] = false;
        return;
    }

    // Các lần sau: chỉ gửi trường changed
    if (eleChanged.voltage[id])
        elecDoc["voltage"] = pzemVoltageCalib[id];
    if (eleChanged.current[id])
        elecDoc["current"] = sensorData[id].current;
    if (eleChanged.power[id])
        elecDoc["power"] = sensorData[id].power;
    if (eleChanged.frequency[id])
        elecDoc["frequency"] = sensorData[id].frequency;
    if (eleChanged.pf[id])
        elecDoc["power_factor"] = sensorData[id].pf;
    if (eleChanged.machineState[id])
        elecDoc["machine_state"] = sensorData[id].machineState;
    if (eleChanged.overVoltage[id])
        elecDoc["over_voltage"] = overVoltage[id];
    if (eleChanged.overCurrent[id])
        elecDoc["over_current"] = overCurrent[id];
    if (eleChanged.overPower[id])
        elecDoc["over_power"] = overPower[id];
    if (eleChanged.underVoltage[id])
        elecDoc["under_voltage"] = underVoltage[id];
    if (eleChanged.socketState[id])
        elecDoc["socket_state"] = socketState[id];

    // Thời gian hoạt động
    if (eleChanged.operating_time[id])
    {
        char buf[16];
        switch (id)
        {
        case AUO_DISPLAY:
            op_time_counter_get_formatted(&op_time_auo_display, buf, sizeof(buf));
            break;
        case CCU_IMAGE1_S:
            op_time_counter_get_formatted(&op_time_ccu_img1s, buf, sizeof(buf));
            break;
        case CCU_IMAGE_1_HUB:
            op_time_counter_get_formatted(&op_time_ccu_img1hub, buf, sizeof(buf));
            break;
        case CCU_TRICAM_PAL:
            op_time_counter_get_formatted(&op_time_ccu_imgtricpal, buf, sizeof(buf));
            break;
        case XENON_300:
            op_time_counter_get_formatted(&op_time_xenon_300, buf, sizeof(buf));
            break;
        case ENDOFLATOR_UI400:
            op_time_counter_get_formatted(&op_time_UI400, buf, sizeof(buf));
            break;
        default:
            buf[0] = 0;
            break;
        }
        elecDoc["operating_time"] = buf;
    }

    if (elecDoc.size() > 0)
    {
        elecDoc["timestamp"] = IOT_MQTT_getTimestamp();
        publishIfChanged(client, elecTopic, elecDoc);
    }

    // Môi trường thiết bị
    if (envChanged.overDeviceTemp[id])
        envDoc["over_temp_max"] = es35swDevice[id].over_temp_max;
    if (envChanged.underDeviceTemp[id])
        envDoc["under_temp_min"] = es35swDevice[id].under_temp_min;
    if (envChanged.overDeviceHumi[id])
        envDoc["over_humi_max"] = es35swDevice[id].over_humi_max;
    if (envChanged.underDeviceHumi[id])
        envDoc["under_humi_min"] = es35swDevice[id].under_humi_min;

    if (envDoc.size() > 0)
    {
        envDoc["timestamp"] = IOT_MQTT_getTimestamp();
        publishIfChanged(client, envTopic, envDoc);
    }
}

static bool firstEnvPublish = true; // Biến đánh dấu lần đầu publish dữ liệu môi trường

// Hàm publish dữ liệu môi trường lên MQTT, chỉ gửi khi có trường thay đổi
static void publishOprCondition(PubSubClient &client, const acLeakChangedFlags &acLeakChanged,const teHuCartChangedFlags &envChanged)
{
    ArduinoJson::StaticJsonDocument<256> acLeakDoc; 
    ArduinoJson::StaticJsonDocument<256> envDoc;

    if (firstEnvPublish)
    {
        acLeakDoc["leak_current"] = leakSensorData.acCurrent;
        acLeakDoc["over_safe_threshold"] = leakSensorData.acSoftWarning;
        acLeakDoc["over_warning_threshold"] = leakSensorData.acStrongWarning;
        acLeakDoc["timestamp"] = IOT_MQTT_getTimestamp();
        publishIfChanged(client, topic_elec_cart, acLeakDoc);

        envDoc["temp"] = es35swCart.temperature;
        envDoc["humi"] = es35swCart.humidity;
        envDoc["over_room_temp_max"] = es35swCart.over_room_temp_max;
        envDoc["under_room_temp_min"] = es35swCart.under_room_temp_min;
        envDoc["over_room_humi_max"] = es35swCart.over_room_humi_max;
        envDoc["under_room_humi_min"] = es35swCart.under_room_humi_min;
        envDoc["over_com_device_temp_max"] = es35swCart.over_com_device_temp_max;
        envDoc["under_com_device_temp_min"] = es35swCart.under_com_device_temp_min;
        envDoc["over_com_device_humi_max"] = es35swCart.over_com_device_humi_max;
        envDoc["under_com_device_humi_min"] = es35swCart.under_com_device_humi_min;
        envDoc["timestamp"] = IOT_MQTT_getTimestamp();
        publishIfChanged(client, topic_env_cart, envDoc);

        // Đánh dấu đã gửi toàn bộ dữ liệu lần đầu
        firstEnvPublish = false;
        return;
    }

    if (acLeakChanged.changeLeakACCurrent)
        acLeakDoc["leak_current"] = leakSensorData.acCurrent;
    if (acLeakChanged.softWarning)
        acLeakDoc["over_safe_threshold"] = leakSensorData.acSoftWarning;
    if (acLeakChanged.strongWarning)
        acLeakDoc["over_warning_threshold"] = leakSensorData.acStrongWarning;
    if (acLeakDoc.size() > 0) 
    {
        acLeakDoc["timestamp"] = IOT_MQTT_getTimestamp();
        publishIfChanged(client, topic_elec_cart, acLeakDoc);
    }

    if (envChanged.temperature) 
        envDoc["temp"] = es35swCart.temperature;
    if (envChanged.humidity) 
        envDoc["humi"] = es35swCart.humidity;
    if (envChanged.overRoomTemp)
        envDoc["over_room_temp_max"] = es35swCart.over_room_temp_max;
    if (envChanged.underRoomTemp)
        envDoc["under_room_temp_min"] = es35swCart.under_room_temp_min;
    if (envChanged.overRoomHumi)
        envDoc["over_room_humi_max"] = es35swCart.over_room_humi_max;
    if (envChanged.underRoomHumi)
        envDoc["under_room_humi_min"] = es35swCart.under_room_humi_min;
    if (envChanged.overComDeviceTemp)    
        envDoc["over_com_device_temp_max"] = es35swCart.over_com_device_temp_max;   
    if (envChanged.underComDeviceTemp)
        envDoc["under_com_device_temp_min"] = es35swCart.under_com_device_temp_min;
    if (envChanged.overComDeviceHumi)
        envDoc["over_com_device_humi_max"] = es35swCart.over_com_device_humi_max;
    if (envChanged.underComDeviceHumi)
        envDoc["under_com_device_humi_min"] = es35swCart.under_com_device_humi_min;
    if (envDoc.size() > 0) 
    {
        envDoc["timestamp"] = IOT_MQTT_getTimestamp();
        publishIfChanged(client, topic_env_cart, envDoc);
    }
}

// Hàm publish toàn bộ dữ liệu lên MQTT, gọi lần lượt các hàm publish cho từng loại dữ liệu
void IOT_MQTT_publishAll(PubSubClient &client, const acLeakChangedFlags &acLeakChanged, const teHuCartChangedFlags &teHuCartChanged, const teHuDecviceChangedFlags &teHuDecviceChanged, const PZEMChangedFlags &pzemChanged)
{
    publishOprCondition(client, acLeakChanged, teHuCartChanged);

    // Bảng ánh xạ topic điện và môi trường cho từng thiết bị
    const struct
    {
        const char *elecTopic;
        const char *envTopic;
        int id;
    } deviceTopics[] = {
        {topic_elec_auo,      topic_env_auo,      AUO_DISPLAY},
        {topic_elec_img1s,    topic_env_img1s,    CCU_IMAGE1_S},
        {topic_elec_img1hub,  topic_env_img1hub,  CCU_IMAGE_1_HUB},
        {topic_elec_imgtripal,topic_env_imgtripal,CCU_TRICAM_PAL},
        {topic_elec_xenon300, topic_env_xenon300, XENON_300},
        {topic_elec_co2ui400, topic_env_co2ui400, ENDOFLATOR_UI400}
    };

    for (const auto &device : deviceTopics)
    {
        publishDeviceData(client, device.elecTopic, device.envTopic, device.id, pzemChanged, teHuDecviceChanged);
    }
}
