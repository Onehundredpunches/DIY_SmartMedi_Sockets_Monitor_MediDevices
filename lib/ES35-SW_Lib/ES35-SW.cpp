/**
 * @file ES35-SW.cpp
 * @brief Implementation for ES35-SW temperature/humidity sensor library.
 * @author Nguyen Minh Tan (Ryan)
 * @date 2025-06-28
 * @license MIT
 */

#include "ES35-SW.h"

// Define global variables
ModbusMaster es35swNode;        // Modbus master object for ES35-SW sensor
HardwareSerial es35swSerial(2); // Serial port for ES35-SW sensors
ES35SWData_Cart es35swCart;          // Data structure to hold ES35-SW sensor data
ES35SWData_Device es35swDevice[NUM_DEVICES]; // Array to hold ES35-SW device data

float lastESTemp = 0; // Last recorded temperature for delta checking
float lastESHumi = 0; // Last recorded humidity for delta checking

static float tempHistory[5] = {0};
static float humiHistory[5] = {0};
static bool isHistoryInitialized = false;

/**
 *  @brief Initialize  the ES35-SW sensor.
 *  This function sets up the Modbus communication for the ES35-SW sensor.
 */
void ES35SW_init()
{
    es35swSerial.begin(9600, SERIAL_8N1, ES35_SW_RX_PIN, ES35_SW_TX_PIN); // Initialize serial communication
    es35swNode.begin(ES35SW_SLAVE_ID, es35swSerial);                      // Set slave ID for Modbus communication
}

// Hàm lọc median (có thể dùng lại từ PZEM hoặc viết lại)
float applyMedianFilterES35(float *data, int size)
{
    float sortedData[size];
    memcpy(sortedData, data, size * sizeof(float));
    std::sort(sortedData, sortedData + size);
    return sortedData[size / 2];
}

/**
 *  @brief Update the ES35-SW sensor data.
 *  This function reads temperature and humidity data from the ES35-SW sensor.
 *  @param sensor Pointer to the ES35SWData struct to store the sensor data.
 *  @return true if the read was successful and data is valid, false otherwise.
 */
bool ES35SW_update(ES35SWData_Cart *sensor)
{
    uint8_t result = es35swNode.readHoldingRegisters(REG_TEMPERATURE, 2); // FC 0x03
    if (result == es35swNode.ku8MBSuccess)
    {
        float temp = es35swNode.getResponseBuffer(0) / 10.0f; // Temperature in degrees Celsius
        float humi = es35swNode.getResponseBuffer(1) / 10.0f; // Humidity in percentage

        // Kiểm tra giá trị hợp lệ trong dải đo cảm biến
        if (isnan(temp) || temp < ES35_TEMP_MIN || temp > ES35_TEMP_MAX ||
            isnan(humi) || humi < ES35_HUMI_MIN || humi > ES35_HUMI_MAX)
        {
            sensor->valid = false;
            return false;
        }

        // Khởi tạo lịch sử nếu lần đầu
        if (!isHistoryInitialized)
        {
            for (int j = 0; j < 5; j++)
            {
                tempHistory[j] = temp;
                humiHistory[j] = humi;
            }
            isHistoryInitialized = true;
        }

        // Cập nhật lịch sử
        for (int j = 4; j > 0; j--)
        {
            tempHistory[j] = tempHistory[j - 1];
            humiHistory[j] = humiHistory[j - 1];
        }
        tempHistory[0] = temp;
        humiHistory[0] = humi;

        // Lọc median
        sensor->temperature = applyMedianFilterES35(tempHistory, 5);
        sensor->humidity    = applyMedianFilterES35(humiHistory, 5);
        sensor->valid = true; // Mark data as valid
        return true;          // Successful read
    }
    else
    {
        sensor->valid = false; // Mark data as invalid
        return false;          // Read failed
    }
}

/**
 *  @brief Read temperature from the ES35-SW sensor.
 *  @param sensor Pointer to the ES35SWData struct to store the temperature data.
 *  @return Temperature in degrees Celsius, or -1.0 if there is an error.
 */
float ES35SW_getTemperature(const ES35SWData_Cart *sensor)
{
    if (sensor && sensor->valid)
    {
        return sensor->temperature;
    }
    else
    {
        return -1.0f; // Lỗi hoặc ngoài dải đo
    }
}

/**
 *  @brief Read humidity from the ES35-SW sensor.
 *  @param sensor Pointer to the ES35SWData struct to store the humidity data.
 *  @return Humidity in percentage, or -1.0 if there is an error.
 */
float ES35SW_getHumidity(const ES35SWData_Cart *sensor)
{
    if (sensor && sensor->valid)
    {
        return sensor->humidity;
    }
    else
    {
        return -1.0f; // Lỗi hoặc ngoài dải đo
    }
}


/**
 * @brief Calculate the delta value for ES35-SW sensor readings.
 */
float calculateDeltaES35(float value, float accuracy_pct, float resolution, float sigma, float F_u, int N_lsb, float delta_min)
{
    float delta = F_u * (fabs(value) * accuracy_pct + sigma + N_lsb * resolution);
    return (delta > delta_min) ? delta : delta_min;
}
