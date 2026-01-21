/**
 * @file MD0630T01A_LeakSensor.h
 * @brief Library for MD0630T01A leakage current sensor.
 * @author Nguyen Minh Tan (Ryan)
 * @date 2025-06-24
 * @license MIT
 */

#ifndef MD0630T01A_LEAKSENSOR_H
#define MD0630T01A_LEAKSENSOR_H

#include <ModbusMaster.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Addresses for Modbus registers of MD0630T01A sensor
// These registers are used to read the current values and thresholds for AC and DC currents.
#define REG_DC_CURRENT 0x00     // DC leakage current register address
#define REG_AC_CURRENT 0x01     // AC leakage current register address
#define REG_DC_THRESHOLD 0x02   // DC leakage threshold register address
#define REG_AC_THRESHOLD 0x03   // AC leakage threshold register address

// RX/TX pin for MD0630T01A sensor UART 1 of ESP32
#define RX_LEAK_SENSOR 9    // RX pin for MD0630T01A sensor
#define TX_LEAK_SENSOR 10   // TX pin for MD0630T01A sensor

// Digital output pins for threshold detection
#define PIN_DO 25 // DC Overcurrent output
#define PIN_AO 33 // AC Overcurrent output
#define PIN_DA 32 // AC or DC Overcurrent output

// Define threshold values for overcurrent detection
#define AC_LEAK_THRESHOLD_SOFT 3.0f          // mA, soft warning
#define AC_LEAK_THRESHOLD_STRONG 5.0f        // mA, strong warning (start)

#define LEAK_AC_DELTA_MIN 0.02f // mA, define minimum delta for AC leakage current updates

extern float lastLeakACCurrent; // Last recorded AC leakage current for delta checking

    /**
     * @brief Struct representing the MD0630T01A leakage current sensor.
     *
     * This struct uses the ModbusMaster library to communicate with the sensor
     * via the Modbus RTU protocol.
     */
    typedef struct
    {
        ModbusMaster *modbus; ///< Pointer to ModbusMaster object for sensor communication
    } LeakSensor;

    /**
     * @brief Struct containing leakage current sensor data.
     */
    typedef struct
    {
        float dcCurrent;            ///< Current DC leakage (mA)
        float acCurrent;            ///< Current AC leakage (mA)
        float dcThreshold;          ///< DC leakage threshold (mA)
        float acThreshold;          ///< AC leakage threshold (mA)
        
        bool overDC;                ///< Flag indicating DC leakage over threshold
        bool overAC;                ///< Flag indicating AC leakage over threshold
        bool overDA;                ///< Flag indicating AC or DC leakage over threshold

        bool acSoftWarning;         ///< Flag indicating soft warning for AC leakage
        bool acStrongWarning;       ///< Flag indicating strong warning for AC leakage

        bool valid;                 ///< Flag indicating valid sensor data
    } LeakSensorData;

    // Global variable definitions
    extern LeakSensorData leakSensorData; // Global variable holding leakage sensor data
    extern HardwareSerial leakSerial;     // Serial port for sensor communication
    extern ModbusMaster leakSensorNode;   // ModbusMaster object for Modbus RTU communication with MD0630T01A sensor
    extern LeakSensor leakSensor;         // Struct managing the leakage sensor, contains pointer to leakSensorNode for sensor handling functions

    /**
     * @brief Initialize UART pins for MD0630T01A leakage current sensor.
     * DO, AO, DA pins are used for overcurrent detection.
     */
    extern void initLeakSensorUart(void);

    /**
     * @brief Initialize Modbus communication for MD0630T01A leakage current sensor.
     * This function sets the slave ID for the Modbus communication.
     */
    extern void initLeakSensorModbus(void);

    // /**
    //  * @brief Initialize GPIO pins for the MD0630T01A leakage current sensor.
    //  *
    //  * Configures the pin modes for the DC overcurrent output (DO), AC overcurrent output (AO),
    //  * and combined AC/DC overcurrent output (DA) pins used for overcurrent detection.
    //  */
    // extern void initLeakSensorThresholdPins(void);

    /**
     * @brief Initialize the MD0630T01A leakage current sensor.
     *
     * This function initializes all necessary components for the sensor to operate,
     * including UART, Modbus communication, and the warning output pins.
     */
    extern void MD0630T01A_init(void);

    /**
     * @brief Read current DC leakage from MD0630T01A sensor.
     * @param sensor Pointer to the LeakSensor object.
     * @return Current DC leakage in mA.
     */
    extern float MD0630T01A_getDCCurrent(LeakSensor *sensor);

    /**
     * @brief Read current AC leakage from MD0630T01A sensor.
     * @param sensor Pointer to the LeakSensor object.
     * @return Current AC leakage in mA.
     */
    extern float MD0630T01A_getACCurrent(LeakSensor *sensor);

    /**
     * @brief Read DC leakage threshold from MD0630T01A sensor.
     * @param sensor Pointer to the LeakSensor object.
     * @return DC leakage threshold in mA.
     */
    extern float MD0630T01A_getDCThreshold(LeakSensor *sensor);

    /**
     * @brief Read AC leakage threshold from MD0630T01A sensor.
     * @param sensor Pointer to the LeakSensor object.
     * @return AC leakage threshold in mA.
     */
    extern float MD0630T01A_getACThreshold(LeakSensor *sensor);

    /**
     * @brief Check if DC leakage is over threshold.
     * @param sensor Pointer to the LeakSensor object.
     * @return true if DC leakage is over threshold, false otherwise.
     */
    extern bool MD0630T01A_isOverDC(LeakSensor *sensor);

    /**
     * @brief Check if AC leakage is over threshold.
     * @param sensor Pointer to the LeakSensor object.
     * @return true if AC leakage is over threshold, false otherwise.
     */
    extern bool MD0630T01A_isOverAC(LeakSensor *sensor);

    /**
     * @brief Check if AC or DC leakage is over threshold.
     * @param sensor Pointer to the LeakSensor object.
     * @return true if AC or DC leakage is over threshold, false otherwise.
     */
    extern bool MD0630T01A_isOverDA(LeakSensor *sensor);

#ifdef __cplusplus
}
#endif

#endif // MD0630T01A_LEAKSENSOR_H