#include "SensorHandlers.h" // Import các khai báo, struct, hàm xử lý cảm biến và trạng thái thiết bị

// Khai báo biến counter thời gian hoạt động cho từng máy, mỗi máy một vùng EEPROM khác nhau
OperatingTimeCounter op_time_auo_display;    // Thiết bị màn hình AUO
OperatingTimeCounter op_time_ccu_img1s;      // Thiết bị CCU image1s
OperatingTimeCounter op_time_ccu_img1hub;    // Thiết bị CCU image 1 hub
OperatingTimeCounter op_time_ccu_imgtricpal; // Thiết bị CCU image tricam pal
OperatingTimeCounter op_time_xenon_300;      // Thiết bị nguồn sáng xenon 300
OperatingTimeCounter op_time_UI400;          // Thiết bị bơm CO2 UI400

// Khai báo thời gian chờ beep còi tiếp theo sau khi phát hiện có thông số vượt ngưỡng
unsigned long lastWarningBeepTime = 0;
const unsigned long warningBeepInterval = 900000; // 15 phút = 900000 ms

// Hàm khởi tạo các biến, struct, trạng thái cảm biến, gọi khi khởi động hệ thống
void SensorHandlers_init()
{
    EEPROM.begin(128); // Khởi tạo vùng nhớ EEPROM, chỉ gọi một lần ở đây để lưu dữ liệu lâu dài

    // Khởi tạo từng bộ đếm thời gian hoạt động, mỗi thiết bị một địa chỉ EEPROM riêng
    op_time_counter_init(&op_time_auo_display, 0);     // Địa chỉ 0 cho thiết bị màn hình AUO
    op_time_counter_init(&op_time_ccu_img1s, 16);      // Địa chỉ 16 cho CCU image1s
    op_time_counter_init(&op_time_ccu_img1hub, 32);    // Địa chỉ 32 cho CCU image 1 hub
    op_time_counter_init(&op_time_ccu_imgtricpal, 48); // Địa chỉ 48 cho CCU image tricam pal
    op_time_counter_init(&op_time_xenon_300, 64);      // Địa chỉ 64 cho nguồn sáng xenon 300
    op_time_counter_init(&op_time_UI400, 80);          // Địa chỉ 80 cho bơm CO2 UI400
}

// Xử lý cảm biến rò điện, cập nhật trạng thái cảnh báo và flag thay đổi
void handleLeakSensor(bool &warning, acLeakChangedFlags &changed)
{
    // float newLeakACCurrent = MD0630T01A_getACCurrent(&leakSensor); // Đọc dòng rò điện mới
    float newLeakACCurrent = 0.0; // Kiểm tra trạng thái rò điện mới

    // Kiểm tra có thay đổi dòng rò điện so với lần trước không
    changed.changeLeakACCurrent = fabs(newLeakACCurrent - lastLeakACCurrent) > LEAK_AC_DELTA_MIN;

    // Nếu có thay đổi dòng rò điện thì cập nhật giá trị mới
    if (changed.changeLeakACCurrent)
    {
        leakSensorData.acCurrent = newLeakACCurrent;
        lastLeakACCurrent = newLeakACCurrent;
    }

    // Cập nhật trạng thái dòng rò điện mới nếu có thay đổi
    bool newSoftWarning = (leakSensorData.acCurrent >= AC_LEAK_THRESHOLD_SOFT && leakSensorData.acCurrent < AC_LEAK_THRESHOLD_STRONG);
    bool newStrongWarning = (leakSensorData.acCurrent >= AC_LEAK_THRESHOLD_STRONG);

    // Kiểm tra có thay đổi trạng thái cảnh báo dòng rò điện không
    changed.softWarning = (leakSensorData.acSoftWarning != newSoftWarning);
    changed.strongWarning = (leakSensorData.acStrongWarning != newStrongWarning);

    // Cập nhật các trạng thái cảnh báo theo ngưỡng dòng rò điện
    leakSensorData.acSoftWarning = newSoftWarning;
    leakSensorData.acStrongWarning = newStrongWarning;

    // Nếu có bất kỳ cảnh báo nào thì set warning = true để xử lý cảnh báo ngoài loop
    if (newSoftWarning || newStrongWarning)
    {
        warning = true;
    }
}

// Xử lý cảm biến môi trường, cập nhật trạng thái cảnh báo và flag thay đổi
void handleES35SW(bool &warning, teHuCartChangedFlags &cartChanged, teHuDecviceChangedFlags &deviceChanged)
{
    if (ES35SW_update(&es35swCart)) // Đọc dữ liệu mới từ cảm biến môi trường
    {
        if (es35swCart.valid)
        {
            float temp = ES35SW_getTemperature(&es35swCart); // Đọc nhiệt độ mới
            float humi = ES35SW_getHumidity(&es35swCart);    // Đọc độ ẩm mới

            // Tính delta động cho nhiệt độ và độ ẩm
            float delta_temp = calculateDeltaES35(temp, ES35_ACCURACY_TEMP, ES35_RESOLUTION_TEMP, ES35_SIGMA_TEMP, ES35_DELTA_FU, ES35_DELTA_N_LSB, ES35_DELTA_TEMP_MIN);
            float delta_humi = calculateDeltaES35(humi, ES35_ACCURACY_HUMI, ES35_RESOLUTION_HUMI, ES35_SIGMA_HUMI, ES35_DELTA_FU, ES35_DELTA_N_LSB, ES35_DELTA_HUMI_MIN);

            // Kiểm tra có thay đổi nhiệt độ/độ ẩm so với lần trước không
            cartChanged.temperature = fabs(temp - lastESTemp) > delta_temp;
            cartChanged.humidity = fabs(humi - lastESHumi) > delta_humi;

            cartChanged.overRoomTemp = false;
            cartChanged.underRoomTemp = false;
            cartChanged.overRoomHumi = false;
            cartChanged.underRoomHumi = false;
            cartChanged.overComDeviceTemp = false;
            cartChanged.underComDeviceTemp = false;
            cartChanged.overComDeviceHumi = false;

            // Nếu có thay đổi nhiệt độ thì cập nhật giá trị mới
            if (cartChanged.temperature)
            {
                es35swCart.temperature = temp;
                lastESTemp = temp;
            }
            // Nếu có thay đổi độ ẩm thì cập nhật giá trị mới
            if (cartChanged.humidity)
            {
                es35swCart.humidity = humi;
                lastESHumi = humi;
            }

            bool preOverRoomTemp = es35swCart.over_room_temp_max;
            bool preUnderRoomTemp = es35swCart.under_room_temp_min;
            bool preOverRoomHumi = es35swCart.over_room_humi_max;
            bool preUnderRoomHumi = es35swCart.under_room_humi_min;
            bool preOverComDeviceTemp = es35swCart.over_com_device_temp_max;
            bool preUnderComDeviceTemp = es35swCart.under_com_device_temp_min;
            bool preOverComDeviceHumi = es35swCart.over_com_device_humi_max;
            bool preUnderComDeviceHumi = es35swCart.under_com_device_humi_min;

            es35swCart.over_room_temp_max = (es35swCart.temperature > ROOM_TEMP_MAX);
            es35swCart.under_room_temp_min = (es35swCart.temperature < ROOM_TEMP_MIN);
            es35swCart.over_room_humi_max = (es35swCart.humidity > ROOM_HUMI_MAX);
            es35swCart.under_room_humi_min = (es35swCart.humidity < ROOM_HUMI_MIN && es35swCart.humidity != 0);

            es35swCart.over_com_device_temp_max = (es35swCart.temperature > COMMON_DEVICE_TEMP_MAX);
            es35swCart.under_com_device_temp_min = (es35swCart.temperature < COMMON_DEVICE_TEMP_MIN);
            es35swCart.over_com_device_humi_max = (es35swCart.humidity > COMMON_DEVICE_HUMI_MAX);
            es35swCart.under_com_device_humi_min = (es35swCart.humidity < COMMON_DEVICE_HUMI_MIN && es35swCart.humidity != 0);

            // Kiểm tra có thay đổi trạng thái quá nhiệt/quá ẩm không
            cartChanged.overRoomTemp = (es35swCart.over_room_temp_max != preOverRoomTemp);
            cartChanged.underRoomTemp = (es35swCart.under_room_temp_min != preUnderRoomTemp);
            cartChanged.overRoomHumi = (es35swCart.over_room_humi_max != preOverRoomHumi);
            cartChanged.underRoomHumi = (es35swCart.under_room_humi_min != preUnderRoomHumi);

            cartChanged.overComDeviceTemp = (es35swCart.over_com_device_temp_max != preOverComDeviceTemp);
            cartChanged.underComDeviceTemp = (es35swCart.under_com_device_temp_min != preUnderComDeviceTemp);
            cartChanged.overComDeviceHumi = (es35swCart.over_com_device_humi_max != preOverComDeviceHumi);
            cartChanged.underComDeviceHumi = (es35swCart.under_com_device_humi_min != preUnderComDeviceHumi);

            for (SOCKET_ID id = AUO_DISPLAY; id < NUM_DEVICES; id = (SOCKET_ID)(id + 1))
            {
                bool prevOverTemp = es35swDevice[id].over_temp_max;
                bool prevUnderTemp = es35swDevice[id].under_temp_min;
                bool prevOverHumi = es35swDevice[id].over_humi_max;
                bool prevUnderHumi = es35swDevice[id].under_humi_min;

                es35swDevice[id].over_temp_max = (es35swCart.temperature > es35swThresholds[id].temperature_max);
                es35swDevice[id].under_temp_min = (es35swCart.temperature < es35swThresholds[id].temperature_min);
                es35swDevice[id].over_humi_max = (es35swCart.humidity > es35swThresholds[id].humidity_max);
                es35swDevice[id].under_humi_min = (es35swCart.humidity < es35swThresholds[id].humidity_min && es35swCart.humidity != 0);

                // Kiểm tra có thay đổi trạng thái quá nhiệt/quá ẩm không
                deviceChanged.overDeviceTemp[id] = (es35swDevice[id].over_temp_max != prevOverTemp);
                deviceChanged.underDeviceTemp[id] = (es35swDevice[id].under_temp_min != prevUnderTemp);
                deviceChanged.overDeviceHumi[id] = (es35swDevice[id].over_humi_max != prevOverHumi);
                deviceChanged.underDeviceHumi[id] = (es35swDevice[id].under_humi_min != prevUnderHumi);

                if (es35swDevice[id].over_temp_max || es35swDevice[id].under_temp_min ||
                    es35swDevice[id].over_humi_max || es35swDevice[id].under_humi_min)
                {
                    warning = true; // Nếu có thiết bị nào vượt ngưỡng thì cảnh báo
                }
            }

            // // Nếu có bất kỳ cảnh báo nào thì set warning = true để xử lý cảnh báo ngoài loop
            // if (es35swCart.over_room_temp_max || es35swCart.under_room_temp_min ||
            //     es35swCart.over_room_humi_max || es35swCart.under_room_humi_min ||
            //     es35swCart.over_com_device_temp_max || es35swCart.under_com_device_temp_min ||
            //     es35swCart.over_com_device_humi_max || es35swCart.under_com_device_humi_min)
            // {
            //     warning = true;
            // }
        }
        else
        {
            warning = true; // Nếu không đọc được dữ liệu cảm biến thì cảnh báo lỗi
        }
    }
}

// Biến lưu chuỗi thời gian hoạt động cuối cùng cho từng thiết bị để phát hiện thay đổi
static char last_operating_time[NUM_DEVICES][24] = {0};

// thời gian warm-up (ms)
static constexpr uint32_t PZEM_VALID_WARMUP_TIME = 3000;
static constexpr uint32_t PZEM_MACHINE_WARMUP_TIME = 3000;

// Xử lý cảm biến điện năng cho từng thiết bị, cập nhật trạng thái cảnh báo và flag thay đổi
// void handlePZEMSensors(bool &warning, PZEMChangedFlags &changed)
// {
//     // Debouce độc lập theo device
//     static uint32_t validRiseTime[NUM_DEVICES] = {0};
//     static bool validWarmup[NUM_DEVICES] = {false};

//     static uint32_t machineRiseTime[NUM_DEVICES] = {0};
//     static bool machineWarmup[NUM_DEVICES] = {false};

//     // Lưu trạng thái raw để bắt cạnh false->true chính xác (độc lập với debounce)
//     static bool     prevRawMachine[NUM_DEVICES]  = {false};

//     for (SOCKET_ID id = AUO_DISPLAY; id < NUM_DEVICES; id = (SOCKET_ID)(id + 1))
//     {
//         // Đọc dữ liệu từ cảm biến PZEM016T
//         readPZEM(id);

//         // Lưu trạng thái trước đó
//         bool prevMachineState = sensorData[id].machineState;
//         bool prevOverVoltage = overVoltage[id];
//         bool prevOverCurrent = overCurrent[id];
//         bool prevOverPower = overPower[id];
//         bool prevUnderVoltage = underVoltage[id];
//         bool prevSocketState = socketState[id];

//         // Reset flag thay đổi
//         changed.voltage[id] = false;
//         changed.current[id] = false;
//         changed.power[id] = false;
//         changed.frequency[id] = false;
//         changed.pf[id] = false;
//         changed.machineState[id] = false;
//         changed.overVoltage[id] = false;
//         changed.overCurrent[id] = false;
//         changed.overPower[id] = false;
//         changed.underVoltage[id] = false;
//         changed.socketState[id] = false;
//         changed.operating_time[id] = false;
//         changed.socketState[id] = false;

//         if (sensorData[id].valid)
//         {
//             const uint32_t now = millis();

//             // Kiểm tra warm-up cho valid
//             if (!prevSocketState)
//             {
//                 validWarmup[id] = true;
//                 validRiseTime[id] = now;
//             }
//             if (validWarmup[id] && (uint32_t)(now - validRiseTime[id]) >= PZEM_VALID_WARMUP_TIME)
//             {
//                 validWarmup[id] = false; // Kết thúc giai đoạn warm-up

//                 lastPZEMVoltage[id] = sensorData[id].voltage;
//                 lastPZEMCurrent[id] = sensorData[id].current;
//                 lastPZEMPower[id] = sensorData[id].power;
//                 lastPZEMFreq[id] = sensorData[id].frequency;
//                 lastPZEMPF[id] = sensorData[id].pf;

//                 if (pzemVoltageCalib[id] == 0)
//                 {
//                     pzemVoltageCalib[id] = sensorData[id].voltage;
//                 }
//             }

//             socketState[id] = true; // Cảm biến valid
//             changed.socketState[id] = (socketState[id] != prevSocketState);

//             // Kiểm tra warm-up cho máy hoạt động
//             const bool rawMachineState = (sensorData[id].current > pzemThresholds[id].current_min);

//             if (rawMachineState && !prevMachineState)
//             {
//                 machineWarmup[id] = true;
//                 machineRiseTime[id] = now;
//             }
//             if (machineWarmup[id] && (uint32_t)(now - machineRiseTime[id]) >= PZEM_MACHINE_WARMUP_TIME)
//             {
//                 machineWarmup[id] = false; // Kết thúc giai đoạn warm-up

//                 lastPZEMVoltage[id] = sensorData[id].voltage;
//                 lastPZEMCurrent[id] = sensorData[id].current;
//                 lastPZEMPower[id] = sensorData[id].power;
//                 lastPZEMFreq[id] = sensorData[id].frequency;
//                 lastPZEMPF[id] = sensorData[id].pf;
//             }

//             sensorData[id].machineState = rawMachineState && !machineWarmup[id];

//             const bool allowUpdate = !validWarmup[id] && !machineWarmup[id];

//             // Phát hiện đổi machineState sau giai đoạn warm-up
//             if (allowUpdate)
//             {
//                 changed.machineState[id] = (sensorData[id].machineState != prevMachineState);
//             }

//             if (allowUpdate)
//             {
//                 // Tính delta động cho từng thông số
//                 float delta_voltage = calculateDelta(sensorData[id].voltage, PZEM_ACCURACY_VOLTAGE, PZEM_RESOLUTION_VOLTAGE, PZEM_SIGMA_VOLTAGE, PZEM_DELTA_FU, PZEM_DELTA_N_LSB, PZEM_DELTA_VOLTAGE_MIN);
//                 float delta_current = calculateDelta(sensorData[id].current, PZEM_ACCURACY_CURRENT, PZEM_RESOLUTION_CURRENT, PZEM_SIGMA_CURRENT, PZEM_DELTA_FU, PZEM_DELTA_N_LSB, PZEM_DELTA_CURRENT_MIN);
//                 float delta_power = calculateDelta(sensorData[id].power, PZEM_ACCURACY_POWER, PZEM_RESOLUTION_POWER, PZEM_SIGMA_POWER, PZEM_DELTA_FU, PZEM_DELTA_N_LSB, PZEM_DELTA_POWER_MIN);
//                 float delta_freq = calculateDelta(sensorData[id].frequency, PZEM_ACCURACY_FREQ, PZEM_RESOLUTION_FREQ, PZEM_SIGMA_FREQ, PZEM_DELTA_FU, PZEM_DELTA_N_LSB, PZEM_DELTA_FREQ_MIN);
//                 float delta_pf = calculateDelta(sensorData[id].pf, PZEM_ACCURACY_PF, PZEM_RESOLUTION_PF, PZEM_SIGMA_PF, PZEM_DELTA_FU, PZEM_DELTA_N_LSB, PZEM_DELTA_PF_MIN);

//                 // Kiểm tra thay đổi và cập nhật last*
//                 if (fabs(sensorData[id].voltage - lastPZEMVoltage[id]) > delta_voltage)
//                 {
//                     changed.voltage[id] = true;
//                     lastPZEMVoltage[id] = sensorData[id].voltage;

//                     // Hiệu chỉnh điện áp gửi lên dashboard
//                     float v_ref = sensorData[AUO_DISPLAY].voltage + PZEM0_VOLTAGE_OFFSET;
//                     float diff = fabs(sensorData[id].voltage - v_ref);

//                     // Tránh gửi 0V lần đầu
//                     if (lastPZEMVoltage[id] == 0 || pzemVoltageCalib[id] == 0)
//                     {
//                         pzemVoltageCalib[id] = v_ref;
//                     }
//                     else if (diff < PZEM_VOLTAGE_SYNC_THRESHOLD)
//                     {
//                         pzemVoltageCalib[id] = v_ref;
//                     }
//                     else
//                     {
//                         warning = true;
//                         pzemVoltageCalib[id] = sensorData[id].voltage;
//                         Serial.printf("[CẢNH BÁO] %s: Điện áp lệch ref quá lớn (%.2fV so với %.2fV)\n", SOCKET_NAMES[id], sensorData[id].voltage, v_ref);
//                     }
//                 }
//                 else if (pzemVoltageCalib[id] == 0)
//                 {
//                     // Đảm bảo không bị 0V ở lần đầu
//                     pzemVoltageCalib[id] = sensorData[id].voltage;
//                 }

//                 if (fabs(sensorData[id].current - lastPZEMCurrent[id]) > delta_current)
//                 {
//                     changed.current[id] = true;
//                     lastPZEMCurrent[id] = sensorData[id].current;
//                 }
//                 if (fabs(sensorData[id].power - lastPZEMPower[id]) > delta_power)
//                 {
//                     changed.power[id] = true;
//                     lastPZEMPower[id] = sensorData[id].power;
//                 }
//                 if (fabs(sensorData[id].frequency - lastPZEMFreq[id]) > delta_freq)
//                 {
//                     changed.frequency[id] = true;
//                     lastPZEMFreq[id] = sensorData[id].frequency;
//                 }
//                 if (fabs(sensorData[id].pf - lastPZEMPF[id]) > delta_pf)
//                 {
//                     changed.pf[id] = true;
//                     lastPZEMPF[id] = sensorData[id].pf;
//                 }

//                 // Cập nhật trạng thái cảnh báo dựa vào ngưỡng
//                 overVoltage[id] = (sensorData[id].voltage > pzemThresholds[id].voltage_max);
//                 underVoltage[id] = (sensorData[id].voltage < pzemThresholds[id].voltage_min);
//                 overCurrent[id] = (sensorData[id].current > pzemThresholds[id].current_max);
//                 overPower[id] = (sensorData[id].power > pzemThresholds[id].power_max);

//                 // // Xác định trạng thái hoạt động dựa vào dòng điện
//                 // sensorData[id].machineState = (sensorData[id].current > pzemThresholds[id].current_min);

//                 // // Cập nhật trạng thái mất nguồn ổ cắm or cảm biến lỗi
//                 // socketState[id] = sensorData[id].valid;

//                 // Phát hiện thay đổi trạng thái cảnh báo so với lần trước
//                 changed.machineState[id] = (sensorData[id].machineState != prevMachineState);
//                 changed.overVoltage[id] = (overVoltage[id] != prevOverVoltage);
//                 changed.overCurrent[id] = (overCurrent[id] != prevOverCurrent);
//                 changed.overPower[id] = (overPower[id] != prevOverPower);
//                 changed.underVoltage[id] = (underVoltage[id] != prevUnderVoltage);
//                 changed.socketState[id] = (socketState[id] != prevSocketState);

//                 // Cập nhật thời gian hoạt động cho từng máy, đồng thời phát hiện thay đổi để publish lên MQTT
//                 char buf[16];
//                 switch (id)
//                 {
//                 case AUO_DISPLAY:
//                     op_time_counter_update(&op_time_auo_display, sensorData[id].machineState);
//                     op_time_counter_get_formatted(&op_time_auo_display, buf, sizeof(buf));
//                     break;
//                 case CCU_IMAGE1_S:
//                     op_time_counter_update(&op_time_ccu_img1s, sensorData[id].machineState);
//                     op_time_counter_get_formatted(&op_time_ccu_img1s, buf, sizeof(buf));
//                     break;
//                 case CCU_IMAGE_1_HUB:
//                     op_time_counter_update(&op_time_ccu_img1hub, sensorData[id].machineState);
//                     op_time_counter_get_formatted(&op_time_ccu_img1hub, buf, sizeof(buf));
//                     break;
//                 case CCU_TRICAM_PAL:
//                     op_time_counter_update(&op_time_ccu_imgtricpal, sensorData[id].machineState);
//                     op_time_counter_get_formatted(&op_time_ccu_imgtricpal, buf, sizeof(buf));
//                     break;
//                 case XENON_300:
//                     op_time_counter_update(&op_time_xenon_300, sensorData[id].machineState);
//                     op_time_counter_get_formatted(&op_time_xenon_300, buf, sizeof(buf));
//                     break;
//                 case ENDOFLATOR_UI400:
//                     op_time_counter_update(&op_time_UI400, sensorData[id].machineState);
//                     op_time_counter_get_formatted(&op_time_UI400, buf, sizeof(buf));
//                     break;
//                 default:
//                     buf[0] = 0;
//                     break;
//                 }

//                 if (strcmp(buf, last_operating_time[id]) != 0)
//                 {
//                     changed.operating_time[id] = true;
//                     strcpy(last_operating_time[id], buf);
//                 }
//                 else
//                 {
//                     changed.operating_time[id] = false;
//                 }

//                 // Nếu có bất kỳ cảnh báo nào thì set warning = true để xử lý cảnh báo ngoài loop
//                 if (overVoltage[id] || underVoltage[id] || overCurrent[id] || overPower[id])
//                 {
//                     warning = true;
//                 }
//             }
//         }
//         else
//         {
//             if (!sensorData[id].valid)
//             {
//                 sensorData[id].voltage = 0.0;
//                 sensorData[id].current = 0.0;
//                 sensorData[id].power = 0.0;
//                 sensorData[id].frequency = 0.0;
//                 sensorData[id].pf = 0.0;
//                 sensorData[id].machineState = false;
//                 socketState[id] = false;
//                 changed.socketState[id] = (socketState[id] != prevSocketState);

//                 // reset warm-up khi mat valid
//                 validWarmup[id] = false;
//                 machineWarmup[id] = false;

//                 // Đếm số lần đọc lỗi, log nếu nhiều lần
//                 if (!areAllSocketsPowerLost())
//                 {
//                     readFailCount[id]++;
//                     Serial.printf("Failed to read PZEM #%d (%d times)\n", id + 1, readFailCount[id]);
//                     if (readFailCount[id] >= 40)
//                     {
//                         Serial.printf("Warning: PZEM #%d read failed 40 times!\n", id + 1);
//                         // ESP.restart(); // Nếu muốn tự động restart khi lỗi liên tục
//                     }
//                 }
//                 warning = true;
//             }
//         }
//         delay(150); // Delay ngắn tránh nhiễu khi đọc cảm biến
//     }

//     if (areAllSocketsPowerLost())
//     {
//         warning = false; // Nếu tất cả thiết bị đều mất nguồn thì không cảnh báo
//         return;
//     }
// }

/////////////////////////////
void handlePZEMSensors(bool &warning, PZEMChangedFlags &changed)
{
    static uint32_t validRiseTime[NUM_DEVICES]   = {0};
    static bool     validWarmup[NUM_DEVICES]     = {false};

    static uint32_t machineRiseTime[NUM_DEVICES] = {0};
    static bool     machineWarmup[NUM_DEVICES]   = {false};

    static bool bootSnapshotSent = false;

    for (SOCKET_ID id = AUO_DISPLAY; id < NUM_DEVICES; id = (SOCKET_ID)(id + 1))
    {
        readPZEM(id);
        delay(150);
    }

    const uint32_t now = millis();

    // ========================= BOOT SNAPSHOT =========================
    if (!bootSnapshotSent)
    {
        const bool  ref_ok = sensorData[AUO_DISPLAY].valid;
        const float v_ref  = sensorData[AUO_DISPLAY].voltage + PZEM0_VOLTAGE_OFFSET;

        for (SOCKET_ID id = AUO_DISPLAY; id < NUM_DEVICES; id = (SOCKET_ID)(id + 1))
        {
            changed.voltage[id]        = false;
            changed.current[id]        = false;
            changed.power[id]          = false;
            changed.frequency[id]      = false;
            changed.pf[id]             = false;
            changed.machineState[id]   = false;
            changed.overVoltage[id]    = false;
            changed.underVoltage[id]   = false;
            changed.overCurrent[id]    = false;
            changed.overPower[id]      = false;
            changed.socketState[id]    = false;
            changed.operating_time[id] = false;

            validWarmup[id]  = false;
            machineWarmup[id]= false;

            if (!sensorData[id].valid)
            {
                socketState[id] = false;
                changed.socketState[id] = true;

                sensorData[id].voltage = 0.0f;
                sensorData[id].current = 0.0f;
                sensorData[id].power   = 0.0f;
                sensorData[id].frequency = 0.0f;
                sensorData[id].pf = 0.0f;
                sensorData[id].machineState = false;

                overVoltage[id] = underVoltage[id] = overCurrent[id] = overPower[id] = false;

                changed.voltage[id]   = true;
                changed.current[id]   = true;
                changed.power[id]     = true;
                changed.frequency[id] = true;
                changed.pf[id]        = true;
                changed.machineState[id] = true;
                changed.overVoltage[id]  = true;
                changed.underVoltage[id] = true;
                changed.overCurrent[id]  = true;
                changed.overPower[id]    = true;
                changed.operating_time[id] = true;

                lastPZEMVoltage[id] = 0.0f;
                lastPZEMCurrent[id] = 0.0f;
                lastPZEMPower[id]   = 0.0f;
                lastPZEMFreq[id]    = 0.0f;
                lastPZEMPF[id]      = 0.0f;
                pzemVoltageCalib[id]= 0.0f;

                strcpy(last_operating_time[id], "00:00:00");
                continue;
            }

            socketState[id] = true;
            changed.socketState[id] = true;

            const bool rawMachine = (sensorData[id].current > pzemThresholds[id].current_min);
            sensorData[id].machineState = rawMachine;

            float v_send;
            if (id == AUO_DISPLAY) v_send = sensorData[id].voltage + PZEM0_VOLTAGE_OFFSET;
            else if (ref_ok)
            {
                const float diff = fabs(sensorData[id].voltage - v_ref);
                v_send = (diff < PZEM_VOLTAGE_SYNC_THRESHOLD) ? v_ref : sensorData[id].voltage;
                if (diff >= PZEM_VOLTAGE_SYNC_THRESHOLD)
                {
                    Serial.printf("[CẢNH BÁO] %s: Điện áp lệch ref quá lớn (%.2fV so với %.2fV)\n",
                                  SOCKET_NAMES[id], sensorData[id].voltage, v_ref);
                }
            }
            else v_send = sensorData[id].voltage;

            pzemVoltageCalib[id] = v_send;

            changed.voltage[id]   = true;
            changed.current[id]   = true;
            changed.power[id]     = true;
            changed.frequency[id] = true;
            changed.pf[id]        = true;
            changed.machineState[id] = true;

            overVoltage[id]  = (sensorData[id].voltage > pzemThresholds[id].voltage_max);
            underVoltage[id] = (sensorData[id].voltage < pzemThresholds[id].voltage_min);
            overCurrent[id]  = (sensorData[id].current > pzemThresholds[id].current_max);
            overPower[id]    = (sensorData[id].power   > pzemThresholds[id].power_max);

            changed.overVoltage[id]  = true;
            changed.underVoltage[id] = true;
            changed.overCurrent[id]  = true;
            changed.overPower[id]    = true;

            if (overVoltage[id] || underVoltage[id] || overCurrent[id] || overPower[id])
                warning = true;

            char buf[16] = {0};
            switch (id)
            {
            case AUO_DISPLAY:
                op_time_counter_update(&op_time_auo_display, sensorData[id].machineState);
                op_time_counter_get_formatted(&op_time_auo_display, buf, sizeof(buf));
                break;
            case CCU_IMAGE1_S:
                op_time_counter_update(&op_time_ccu_img1s, sensorData[id].machineState);
                op_time_counter_get_formatted(&op_time_ccu_img1s, buf, sizeof(buf));
                break;
            case CCU_IMAGE_1_HUB:
                op_time_counter_update(&op_time_ccu_img1hub, sensorData[id].machineState);
                op_time_counter_get_formatted(&op_time_ccu_img1hub, buf, sizeof(buf));
                break;
            case CCU_TRICAM_PAL:
                op_time_counter_update(&op_time_ccu_imgtricpal, sensorData[id].machineState);
                op_time_counter_get_formatted(&op_time_ccu_imgtricpal, buf, sizeof(buf));
                break;
            case XENON_300:
                op_time_counter_update(&op_time_xenon_300, sensorData[id].machineState);
                op_time_counter_get_formatted(&op_time_xenon_300, buf, sizeof(buf));
                break;
            case ENDOFLATOR_UI400:
                op_time_counter_update(&op_time_UI400, sensorData[id].machineState);
                op_time_counter_get_formatted(&op_time_UI400, buf, sizeof(buf));
                break;
            default:
                buf[0] = 0;
                break;
            }
            changed.operating_time[id] = true;
            strcpy(last_operating_time[id], buf);

            lastPZEMVoltage[id] = v_send;
            lastPZEMCurrent[id] = sensorData[id].current;
            lastPZEMPower[id]   = sensorData[id].power;
            lastPZEMFreq[id]    = sensorData[id].frequency;
            lastPZEMPF[id]      = sensorData[id].pf;
        }

        bootSnapshotSent = true;

        if (areAllSocketsPowerLost())
        {
            warning = false;
            return;
        }
        return;
    }

    // ========================= NORMAL OPERATION =========================
    const bool  refReady = sensorData[AUO_DISPLAY].valid && !validWarmup[AUO_DISPLAY];
    const float v_ref    = sensorData[AUO_DISPLAY].voltage + PZEM0_VOLTAGE_OFFSET;

    float v_send[NUM_DEVICES] = {0};
    bool  allowLine[NUM_DEVICES] = {false};
    bool  allowLoad[NUM_DEVICES] = {false};

    // pass A
    for (SOCKET_ID id = AUO_DISPLAY; id < NUM_DEVICES; id = (SOCKET_ID)(id + 1))
    {
        const bool prevSocketState  = socketState[id];
        const bool prevMachineState = sensorData[id].machineState;  // <-- QUAN TRỌNG cho change detect

        changed.voltage[id]        = false;
        changed.current[id]        = false;
        changed.power[id]          = false;
        changed.frequency[id]      = false;
        changed.pf[id]             = false;
        changed.machineState[id]   = false;
        changed.overVoltage[id]    = false;
        changed.underVoltage[id]   = false;
        changed.overCurrent[id]    = false;
        changed.overPower[id]      = false;
        changed.socketState[id]    = false;
        changed.operating_time[id] = false;

        if (!sensorData[id].valid)
        {
            socketState[id] = false;
            changed.socketState[id] = (prevSocketState != false);

            validWarmup[id]   = false;
            machineWarmup[id] = false;

            sensorData[id].voltage = 0.0f;
            sensorData[id].current = 0.0f;
            sensorData[id].power   = 0.0f;
            sensorData[id].frequency = 0.0f;
            sensorData[id].pf = 0.0f;

            // machine_state: OFFLINE thì bắt buộc về false và publish nếu trước đó true
            sensorData[id].machineState = false;
            if (prevMachineState) changed.machineState[id] = true;   // <-- FIX

            if (lastPZEMVoltage[id] != 0.0f) { lastPZEMVoltage[id] = 0.0f; changed.voltage[id] = true; }
            if (lastPZEMCurrent[id] != 0.0f) { lastPZEMCurrent[id] = 0.0f; changed.current[id] = true; }
            if (lastPZEMPower[id]   != 0.0f) { lastPZEMPower[id]   = 0.0f; changed.power[id] = true; }
            if (lastPZEMFreq[id]    != 0.0f) { lastPZEMFreq[id]    = 0.0f; changed.frequency[id] = true; }
            if (lastPZEMPF[id]      != 0.0f) { lastPZEMPF[id]      = 0.0f; changed.pf[id] = true; }

            pzemVoltageCalib[id] = 0.0f;
            continue;
        }

        socketState[id] = true;
        changed.socketState[id] = (prevSocketState != true);

        if (!prevSocketState)
        {
            validWarmup[id] = true;
            validRiseTime[id] = now;
        }
        if (validWarmup[id] && (uint32_t)(now - validRiseTime[id]) >= PZEM_VALID_WARMUP_TIME)
            validWarmup[id] = false;

        const bool rawMachine = (sensorData[id].current > pzemThresholds[id].current_min);

        if (rawMachine && !prevMachineState && !machineWarmup[id])
        {
            machineWarmup[id] = true;
            machineRiseTime[id] = now;
        }
        if (!rawMachine) machineWarmup[id] = false;
        else if (machineWarmup[id] && (uint32_t)(now - machineRiseTime[id]) >= PZEM_MACHINE_WARMUP_TIME)
            machineWarmup[id] = false;

        sensorData[id].machineState = (rawMachine && !machineWarmup[id]);

        allowLine[id] = (!validWarmup[id]);
        allowLoad[id] = (!validWarmup[id] && !machineWarmup[id]);

        // machine_state: chỉ publish khi allowLoad để tránh nhiễu lúc mới bật máy
        if (allowLoad[id] && (sensorData[id].machineState != prevMachineState))
        {
            changed.machineState[id] = true;   // <-- FIX
        }

        if (!refReady || !allowLine[id]) v_send[id] = lastPZEMVoltage[id];
        else if (id == AUO_DISPLAY)      v_send[id] = sensorData[id].voltage + PZEM0_VOLTAGE_OFFSET;
        else
        {
            const float diff = fabs(sensorData[id].voltage - v_ref);
            v_send[id] = (diff < PZEM_VOLTAGE_SYNC_THRESHOLD) ? v_ref : sensorData[id].voltage;
            if (diff >= PZEM_VOLTAGE_SYNC_THRESHOLD)
            {
                Serial.printf("[CẢNH BÁO] %s: Điện áp lệch ref quá lớn (%.2fV so với %.2fV)\n",
                              SOCKET_NAMES[id], sensorData[id].voltage, v_ref);
            }
        }
        pzemVoltageCalib[id] = v_send[id];
    }

    bool broadcastVoltage = false;
    if (refReady)
    {
        for (SOCKET_ID id = AUO_DISPLAY; id < NUM_DEVICES; id = (SOCKET_ID)(id + 1))
        {
            if (!sensorData[id].valid || !allowLine[id]) continue;

            const float delta_v = calculateDelta(v_send[id],
                                                PZEM_ACCURACY_VOLTAGE, PZEM_RESOLUTION_VOLTAGE, PZEM_SIGMA_VOLTAGE,
                                                PZEM_DELTA_FU, PZEM_DELTA_N_LSB, PZEM_DELTA_VOLTAGE_MIN);

            if (fabs(v_send[id] - lastPZEMVoltage[id]) > delta_v)
            {
                broadcastVoltage = true;
                break;
            }
        }
    }

    // pass B
    for (SOCKET_ID id = AUO_DISPLAY; id < NUM_DEVICES; id = (SOCKET_ID)(id + 1))
    {
        if (!sensorData[id].valid) continue;

        if (refReady && allowLine[id] && broadcastVoltage)
        {
            changed.voltage[id] = true;
            lastPZEMVoltage[id] = v_send[id];
        }
        else if (refReady && allowLine[id])
        {
            const float delta_v = calculateDelta(v_send[id],
                                                PZEM_ACCURACY_VOLTAGE, PZEM_RESOLUTION_VOLTAGE, PZEM_SIGMA_VOLTAGE,
                                                PZEM_DELTA_FU, PZEM_DELTA_N_LSB, PZEM_DELTA_VOLTAGE_MIN);
            if (fabs(v_send[id] - lastPZEMVoltage[id]) > delta_v)
            {
                changed.voltage[id] = true;
                lastPZEMVoltage[id] = v_send[id];
            }
        }

        if (allowLine[id])
        {
            const float delta_f = calculateDelta(sensorData[id].frequency,
                                                PZEM_ACCURACY_FREQ, PZEM_RESOLUTION_FREQ, PZEM_SIGMA_FREQ,
                                                PZEM_DELTA_FU, PZEM_DELTA_N_LSB, PZEM_DELTA_FREQ_MIN);
            if (fabs(sensorData[id].frequency - lastPZEMFreq[id]) > delta_f)
            {
                changed.frequency[id] = true;
                lastPZEMFreq[id] = sensorData[id].frequency;
            }
        }

        if (allowLoad[id])
        {
            const float delta_i = calculateDelta(sensorData[id].current,
                                                PZEM_ACCURACY_CURRENT, PZEM_RESOLUTION_CURRENT, PZEM_SIGMA_CURRENT,
                                                PZEM_DELTA_FU, PZEM_DELTA_N_LSB, PZEM_DELTA_CURRENT_MIN);
            const float delta_p = calculateDelta(sensorData[id].power,
                                                PZEM_ACCURACY_POWER, PZEM_RESOLUTION_POWER, PZEM_SIGMA_POWER,
                                                PZEM_DELTA_FU, PZEM_DELTA_N_LSB, PZEM_DELTA_POWER_MIN);
            const float delta_pf = calculateDelta(sensorData[id].pf,
                                                 PZEM_ACCURACY_PF, PZEM_RESOLUTION_PF, PZEM_SIGMA_PF,
                                                 PZEM_DELTA_FU, PZEM_DELTA_N_LSB, PZEM_DELTA_PF_MIN);

            if (fabs(sensorData[id].current - lastPZEMCurrent[id]) > delta_i)
            {
                changed.current[id] = true;
                lastPZEMCurrent[id] = sensorData[id].current;
            }
            if (fabs(sensorData[id].power - lastPZEMPower[id]) > delta_p)
            {
                changed.power[id] = true;
                lastPZEMPower[id] = sensorData[id].power;
            }
            if (fabs(sensorData[id].pf - lastPZEMPF[id]) > delta_pf)
            {
                changed.pf[id] = true;
                lastPZEMPF[id] = sensorData[id].pf;
            }

            char buf[16] = {0};
            switch (id)
            {
            case AUO_DISPLAY:
                op_time_counter_update(&op_time_auo_display, sensorData[id].machineState);
                op_time_counter_get_formatted(&op_time_auo_display, buf, sizeof(buf));
                break;
            case CCU_IMAGE1_S:
                op_time_counter_update(&op_time_ccu_img1s, sensorData[id].machineState);
                op_time_counter_get_formatted(&op_time_ccu_img1s, buf, sizeof(buf));
                break;
            case CCU_IMAGE_1_HUB:
                op_time_counter_update(&op_time_ccu_img1hub, sensorData[id].machineState);
                op_time_counter_get_formatted(&op_time_ccu_img1hub, buf, sizeof(buf));
                break;
            case CCU_TRICAM_PAL:
                op_time_counter_update(&op_time_ccu_imgtricpal, sensorData[id].machineState);
                op_time_counter_get_formatted(&op_time_ccu_imgtricpal, buf, sizeof(buf));
                break;
            case XENON_300:
                op_time_counter_update(&op_time_xenon_300, sensorData[id].machineState);
                op_time_counter_get_formatted(&op_time_xenon_300, buf, sizeof(buf));
                break;
            case ENDOFLATOR_UI400:
                op_time_counter_update(&op_time_UI400, sensorData[id].machineState);
                op_time_counter_get_formatted(&op_time_UI400, buf, sizeof(buf));
                break;
            default:
                buf[0] = 0;
                break;
            }

            if (strcmp(buf, last_operating_time[id]) != 0)
            {
                changed.operating_time[id] = true;
                strcpy(last_operating_time[id], buf);
            }
        }
    }

    if (areAllSocketsPowerLost())
    {
        warning = false;
        return;
    }
}






// In toàn bộ snapshot dữ liệu cảm biến lên Serial để debug, kiểm tra hệ thống
void printSensorSnapshot()
{
    Serial.println();
    Serial.println("=============== SENSOR SNAPSHOT ===============");

    // Leak sensor
    Serial.println("[LEAK SENSOR]");
    Serial.printf("  AC Current: %.2f mA (Threshold: %.2f mA)\n", leakSensorData.acCurrent, leakSensorData.acThreshold);
    Serial.printf("  Soft Warning: %s | Strong Warning: %s\n",
                  leakSensorData.acSoftWarning ? "YES" : "NO",
                  leakSensorData.acStrongWarning ? "YES" : "NO");

    // ES35-SW environment sensor
    Serial.println("\n[ES35-SW ENVIRONMENT]");
    Serial.printf("  Temp: %.2f°C | Humi: %.2f%% | Valid: %s\n",
                  es35swCart.temperature, es35swCart.humidity, es35swCart.valid ? "YES" : "NO");
    Serial.printf("  Over Room Temp: %s | Under Room Temp: %s | Over Room Humi: %s | Under Room Humi: %s\n",
                  es35swCart.over_room_temp_max ? "YES" : "NO",
                  es35swCart.under_room_temp_min ? "YES" : "NO",
                  es35swCart.over_room_humi_max ? "YES" : "NO",
                  es35swCart.under_room_humi_min ? "YES" : "NO");
    Serial.printf("  Over Device Com Temp: %s | Under Device Com Temp: %s | Over Device Com Humi: %s | Under Device Com Humi: %s\n",
                  es35swCart.over_com_device_temp_max ? "YES" : "NO",
                  es35swCart.under_com_device_temp_min ? "YES" : "NO",
                  es35swCart.over_com_device_humi_max ? "YES" : "NO",
                  es35swCart.under_com_device_humi_min ? "YES" : "NO");

    // Power sockets/devices
    Serial.println("\n[POWER SOCKETS]");
    if (areAllSocketsPowerLost())
    {
        Serial.println("  All sockets are power lost. No data to print.");
    }
    else
    {
        for (SOCKET_ID id = AUO_DISPLAY; id < NUM_DEVICES; id = (SOCKET_ID)(id + 1))
        {
            Serial.printf("  [%s]\n", SOCKET_NAMES[id]);
            Serial.printf("    Machine State: %s | Valid: %s | Socket State: %s\n",
                          sensorData[id].machineState ? "ON" : "OFF",
                          sensorData[id].valid ? "YES" : "NO",
                          socketState[id] ? "YES" : "NO");
            Serial.printf("    [MQTT] U=%.2fV I=%.3fA P=%.1fW F=%.2fHz PF=%.2f\n",
                          pzemVoltageCalib[id], sensorData[id].current, sensorData[id].power,
                          sensorData[id].frequency, sensorData[id].pf);
            Serial.printf("    [RAW ] U=%.2fV I=%.3fA P=%.1fW F=%.2fHz PF=%.2f\n",
                          sensorData[id].voltage, sensorData[id].current, sensorData[id].power,
                          sensorData[id].frequency, sensorData[id].pf);
            Serial.printf("    [LAST] U=%.2fV I=%.3fA P=%.1fW F=%.2fHz PF=%.2f\n",
                          lastPZEMVoltage[id], lastPZEMCurrent[id], lastPZEMPower[id],
                          lastPZEMFreq[id], lastPZEMPF[id]);
            Serial.printf("    OverV: %s | UnderV: %s | OverI: %s | OverP: %s\n",
                          overVoltage[id] ? "YES" : "NO",
                          underVoltage[id] ? "YES" : "NO",
                          overCurrent[id] ? "YES" : "NO",
                          overPower[id] ? "YES" : "NO");
        }
    }

    // Operating time
    Serial.println("\n[OPERATING TIME]");
    char buf[24];
    op_time_counter_get_formatted(&op_time_auo_display, buf, sizeof(buf));
    Serial.printf("  AUO_DISPLAY:      %s\n", buf);
    op_time_counter_get_formatted(&op_time_ccu_img1s, buf, sizeof(buf));
    Serial.printf("  CCU_IMAGE1_S:     %s\n", buf);
    op_time_counter_get_formatted(&op_time_ccu_img1hub, buf, sizeof(buf));
    Serial.printf("  CCU_IMAGE_1_HUB:  %s\n", buf);
    op_time_counter_get_formatted(&op_time_ccu_imgtricpal, buf, sizeof(buf));
    Serial.printf("  CCU_TRICAM_PAL:   %s\n", buf);
    op_time_counter_get_formatted(&op_time_xenon_300, buf, sizeof(buf));
    Serial.printf("  XENON_300:        %s\n", buf);
    op_time_counter_get_formatted(&op_time_UI400, buf, sizeof(buf));
    Serial.printf("  ENDOFLATOR_UI400: %s\n", buf);

    // Delta snapshot
    Serial.println("\n[DELTA SNAPSHOT]");
    for (SOCKET_ID id = AUO_DISPLAY; id < NUM_DEVICES; id = (SOCKET_ID)(id + 1))
    {
        float delta_voltage = calculateDelta(sensorData[id].voltage, PZEM_ACCURACY_VOLTAGE, PZEM_RESOLUTION_VOLTAGE, PZEM_SIGMA_VOLTAGE, PZEM_DELTA_FU, PZEM_DELTA_N_LSB, PZEM_DELTA_VOLTAGE_MIN);
        float delta_current = calculateDelta(sensorData[id].current, PZEM_ACCURACY_CURRENT, PZEM_RESOLUTION_CURRENT, PZEM_SIGMA_CURRENT, PZEM_DELTA_FU, PZEM_DELTA_N_LSB, PZEM_DELTA_CURRENT_MIN);
        float delta_power = calculateDelta(sensorData[id].power, PZEM_ACCURACY_POWER, PZEM_RESOLUTION_POWER, PZEM_SIGMA_POWER, PZEM_DELTA_FU, PZEM_DELTA_N_LSB, PZEM_DELTA_POWER_MIN);
        float delta_freq = calculateDelta(sensorData[id].frequency, PZEM_ACCURACY_FREQ, PZEM_RESOLUTION_FREQ, PZEM_SIGMA_FREQ, PZEM_DELTA_FU, PZEM_DELTA_N_LSB, PZEM_DELTA_FREQ_MIN);
        float delta_pf = calculateDelta(sensorData[id].pf, PZEM_ACCURACY_PF, PZEM_RESOLUTION_PF, PZEM_SIGMA_PF, PZEM_DELTA_FU, PZEM_DELTA_N_LSB, PZEM_DELTA_PF_MIN);

        Serial.printf("  [%s] ΔU=%.3fV ΔI=%.4fA ΔP=%.2fW ΔF=%.3fHz ΔPF=%.3f\n",
                      SOCKET_NAMES[id], delta_voltage, delta_current, delta_power, delta_freq, delta_pf);
    }
    float delta_temp = calculateDeltaES35(es35swCart.temperature, ES35_ACCURACY_TEMP, ES35_RESOLUTION_TEMP, ES35_SIGMA_TEMP, ES35_DELTA_FU, ES35_DELTA_N_LSB, ES35_DELTA_TEMP_MIN);
    float delta_humi = calculateDeltaES35(es35swCart.humidity, ES35_ACCURACY_HUMI, ES35_RESOLUTION_HUMI, ES35_SIGMA_HUMI, ES35_DELTA_FU, ES35_DELTA_N_LSB, ES35_DELTA_HUMI_MIN);
    Serial.printf("  [ES35-SW] ΔTemp=%.3f°C ΔHumi=%.3f%%\n", delta_temp, delta_humi);

    Serial.println("=============== END SNAPSHOT ==================");
    Serial.println();
}

void handleWarningBeep(bool warning)
{
    static bool warningActive = false;

    if (warning)
    {
        unsigned long now = millis();
        if (!warningActive || now - lastWarningBeepTime >= warningBeepInterval)
        {
            playWarningTone(); // Phát còi cảnh báo
            lastWarningBeepTime = now;
            warningActive = true;
        }
    }
    else
    {
        warningActive = false; // Reset trạng thái nếu hết warning
    }
}