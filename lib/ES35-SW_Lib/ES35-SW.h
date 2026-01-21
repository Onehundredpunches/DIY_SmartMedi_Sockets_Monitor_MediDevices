/**
 * @file ES35-SW.h
 * @brief Library for ES35-SW temperature/humidity sensor.
 * @author Nguyen Minh Tan (Ryan)
 * @date 2025-06-28
 * @license MIT
 */

#ifndef ES35_SW_H
#define ES35_SW_H

#include <ModbusMaster.h>
#include "PZEM016_Lib.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define REG_TEMPERATURE     0x00    // Temperature register address for ES35-SW sensor
#define REG_HUMIDITY        0x01    // Humidity register address for ES35-SW sensor
#define REG_SLAVE_ID        0x64    // Slave ID register address for ES35-SW sensor
#define REG_BAUD_RATE       0x65    // Baud rate register address for ES35-SW sensor

#define ES35SW_SLAVE_ID     0x09     // Slave ID for ES35-SW sensor
#define ES35_SW_BAUD_RATE   9600    // Baud rate for ES35-SW sensor

#define ES35_SW_RX_PIN 17 // RX pin for ES35-SW sensor
#define ES35_SW_TX_PIN 16 // TX pin for ES35-SW sensor

#define ES35_SW_SERIAL Serial2 // Serial port for ES35-SW sensors

#define TEMPERATURE_THHRESHOLD_warning  40.0f       // °C, Temperature threshold for warning
#define HUMIDITY_THRESHOLD_warning      80.0f       // %, Humidity threshold for warning

#define ES35_TEMP_MIN   -20.0f          // °C, Minimum measurable temperature of the sensor
#define ES35_TEMP_MAX   80.0f           // °C, Maximum measurable temperature of the sensor
#define ES35_HUMI_MIN   0.0f            // %, Minimum measurable humidity of the sensor
#define ES35_HUMI_MAX   100.0f          // %, Maximum measurable humidity of the sensor

#define TEMP_DELTA_MIN 0.2f // °C, Define minimum delta for temperature updates
#define HUMI_DELTA_MIN 1.0f // %, Define minimum delta for humidity updates

// Tham số delta cho nhiệt độ và độ ẩm (có thể chỉnh sửa)
#define ES35_DELTA_FU 1.7f
#define ES35_DELTA_N_LSB 3
#define ES35_DELTA_TEMP_MIN 0.2f // °C, ngưỡng tuyệt đối nhỏ nhất cho nhiệt độ
#define ES35_DELTA_HUMI_MIN 1.0f // %, ngưỡng tuyệt đối nhỏ nhất cho độ ẩm

#define ES35_ACCURACY_TEMP 0.005f // 0.5%
#define ES35_ACCURACY_HUMI 0.01f  // 1%
#define ES35_RESOLUTION_TEMP 0.1f
#define ES35_RESOLUTION_HUMI 0.1f
#define ES35_SIGMA_TEMP 0.05f
#define ES35_SIGMA_HUMI 0.2f

#define ROOM_TEMP_MIN 20.0f // °C, Minimum valid room temperature
#define ROOM_TEMP_MAX 25.0f // °C, Maximum valid room temperature
#define ROOM_HUMI_MIN 40.0f // %, Minimum valid room humidity
#define ROOM_HUMI_MAX 60.0f // %, Maximum valid room humidity

#define COMMON_DEVICE_TEMP_MIN 15.0f // °C, Minimum valid temperature for all devices
#define COMMON_DEVICE_TEMP_MAX 30.0f // °C, Maximum valid temperature for all devices
#define COMMON_DEVICE_HUMI_MIN 25.0f // %, Minimum valid humidity for all devices
#define COMMON_DEVICE_HUMI_MAX 75.0f // %, Maximum valid humidity for all devices

    extern float lastESTemp; // Last recorded temperature for delta checking
    extern float lastESHumi; // Last recorded humidity for delta checking

    typedef struct
    {
        float temperature_min; // Minimum valid temperature
        float temperature_max; // Maximum valid temperature
        float humidity_min;    // Minimum valid humidity
        float humidity_max;    // Maximum valid humidity

    } ES35SW_Thresholds;

    static const ES35SW_Thresholds es35swThresholds[NUM_DEVICES] = {
        [AUO_DISPLAY]       = {0.0f, 40.0f, 20.0f, 80.0f},
        [CCU_IMAGE1_S]      = {0.0f, 40.0f, 20.0f, 85.0f},
        [CCU_IMAGE_1_HUB]   = {10.0f, 40.0f, 10.0f, 100.0f},
        [CCU_TRICAM_PAL]    = {10.0f, 40.0f, 10.0f, 100.0f},
        [XENON_300]         = {10.0f, 40.0f, 5.0f, 95.0f},
        [ENDOFLATOR_UI400]  = {10.0f, 35.0f, 15.0f, 85.0f}};

    
    typedef struct
    {
        float temperature;                  // Temperature in degrees Celsius
        float humidity;                     // Humidity in percentage
        bool over_room_temp_max;            // Flag: temperature exceeds safe room max
        bool under_room_temp_min;            // Flag: temperature below safe room min
        bool over_room_humi_max;            // Flag: humidity exceeds safe room max
        bool under_room_humi_min;            // Flag: humidity below safe room min
        bool over_com_device_temp_max;      // Flag: temperature exceeds common device max
        bool under_com_device_temp_min;      // Flag: temperature below common device min
        bool over_com_device_humi_max;      // Flag: humidity exceeds common device max
        bool under_com_device_humi_min;      // Flag: humidity below common device min
        bool valid; // Validity flag to indicate if the data is valid

    } ES35SWData_Cart;
    

    typedef struct
    {
        bool over_temp_max;            // Flag: temperature exceeds opoerating max
        bool under_temp_min;            // Flag: temperature below operating min
        bool over_humi_max;            // Flag: humidity exceeds operating max
        bool under_humi_min;            // Flag: humidity below operating min

    } ES35SWData_Device;



    extern ModbusMaster es35swNode;     // ModbusMaster object for ES35-SW sensor communication
    extern HardwareSerial es35swSerial; // HardwareSerial object for ES35-SW sensor communication
    extern ES35SWData_Cart es35swCart;       // Struct to hold ES35-SW sensor data
    extern ES35SWData_Device es35swDevice[NUM_DEVICES]; // Array to hold Temp-Humi threshold device data
    
    /**
     * @brief Initialize the ES35-SW sensor.
     * This function sets up the Modbus communication for the ES35-SW sensor.
     * It initializes the serial port and prepares the ModbusMaster object.
     */

    extern void ES35SW_init();

    /**
     * @brief Update the ES35-SW sensor data.
     * This function reads the temperature and humidity from the ES35-SW sensor
     * and updates the ES35SWData struct.
     * @param sensor Pointer to the ES35SWData struct to store the sensor data.
     * @return true if the update was successful, false otherwise.
     */
    extern bool ES35SW_update(ES35SWData_Cart *sensor); // Function to update the sensor data

    /**
     * @brief Read temperature from the ES35-SW sensor.
     *  @param sensor Pointer to the ES35SWData struct to store the temperature data.
     *  @return Temperature in degrees Celsius, or -1.0 if there is an error.
     */
    extern float ES35SW_getTemperature(const ES35SWData_Cart *sensor);

    /**
     *  @brief Read humidity from the ES35-SW sensor.
     *  @param sensor Pointer to the ES35SWData struct to store the humidity data.
     *  @return Humidity in percentage, or -1.0 if there is an error.
     */
    extern float ES35SW_getHumidity(const ES35SWData_Cart *sensor);

    /**
     * @brief Calculate the delta for temperature/humidity.
     * This function calculates the delta value for temperature or humidity
     * based on the given parameters.
     * @param value The current value (temperature or humidity).
     * @param accuracy_pct The accuracy percentage.
     * @param resolution The resolution value.
     * @param sigma The sigma value.
     * @param F_u The F_u value.
     * @param N_lsb The N_lsb value.
     * @param delta_min The minimum delta value.
     * @return The calculated delta value.
     */
    extern float calculateDeltaES35(float value, float accuracy_pct, float resolution, float sigma, float F_u, int N_lsb, float delta_min);


#ifdef __cplusplus
}
#endif

#endif // ES35_SW_H