/**
 * @file MD0630T01A_LeakSensor.cpp
 * @brief Implementation for MD0630T01A leakage current sensor library.
 * @author Nguyen Minh Tan (Ryan)
 * @date 2025-06-24
 * @license MIT
 */

#include "MD0630T01A_LeakSensor.h"

// Define global variables
HardwareSerial leakSerial(1); // Define Serial1 for MD0630T01A sensor - RX: GPIO 9, TX: GPIO 10
ModbusMaster leakSensorNode;  // ModbusMaster object for MD0630T01A sensor communication
LeakSensorData leakSensorData; // Global variable holding leakage sensor data
LeakSensor leakSensor = {&leakSensorNode}; // Struct managing the leakage sensor, contains pointer to leakSensorNode for sensor handling functions

float lastLeakACCurrent = 0;

// /**
//  * @brief Initializes the GPIO pins used for threshold detection on the MD0630T01A leakage current sensor.
//  *
//  * Sets the pin modes for the DC overcurrent output (DO), AC overcurrent output (AO),
//  * and combined AC/DC overcurrent output (DA) pins to INPUT for monitoring overcurrent events.
//  */
// void initLeakSensorThresholdPins(void)
// {
//     pinMode(PIN_DO, INPUT);
//     pinMode(PIN_AO, INPUT);
//     pinMode(PIN_DA, INPUT);
// }

/**
 * @brief Initializes UART communication for the MD0630T01A leakage current sensor.
 *
 * Configures the HardwareSerial port with 9600 baud rate, 8 data bits, no parity, 1 stop bit,
 * and assigns the RX and TX pins for the sensor.
 */
void initLeakSensorUart(void)
{
    leakSerial.begin(9600, SERIAL_8N1, RX_LEAK_SENSOR, TX_LEAK_SENSOR);
}

/**
 * @brief Initializes Modbus communication for the MD0630T01A leakage current sensor.
 * This function sets the slave address for Modbus communication.
 */
void initLeakSensorModbus(void)
{
    leakSensorNode.begin(1, leakSerial); // Slave ID = 0x01 (default)
}

/**
 * @brief Initializes the MD0630T01A leakage current sensor hardware and communication interfaces.
 *
 * This function configures the UART interface for serial communication, initializes the Modbus protocol
 * for sensor data exchange, and sets up the GPIO pins used for threshold detection signals.
 * Call this function once during system startup before interacting with the sensor.
 */
void MD0630T01A_init(void)
{
    initLeakSensorUart();
    initLeakSensorModbus();
    // initLeakSensorThresholdPins();
}

/**
 * @brief Reads a single input register from the leak sensor via Modbus and returns its value as a float.
 *
 * This function communicates with the leak sensor using the Modbus protocol to read the value of a specific input register.
 * The raw register value is divided by 10.0 to convert it to a floating-point representation, as per sensor specification.
 * If the read operation fails, the function returns -1.0f to indicate an error.
 *
 * @param sensor Pointer to a LeakSensor object containing the Modbus interface.
 * @param reg    The address of the input register to read from the sensor.
 * @return float The value read from the register, converted to float. Returns -1.0f on failure.
 */
static float readRegister(LeakSensor *sensor, uint16_t reg)
{
    uint8_t result = sensor->modbus->readInputRegisters(reg, 1);
    if (result == sensor->modbus->ku8MBSuccess)
    {
        return sensor->modbus->getResponseBuffer(0) / 10.0f;
    }
    return -1.0f;
}

/**
 * @brief Function to read the DC current from the MD0630T01A leakage current sensor.
 * @param sensor Pointer to the LeakSensor object.
 * @return Current DC leakage in mA or -1.0 if there is an error
 */
float MD0630T01A_getDCCurrent(LeakSensor *sensor)
{
    return readRegister(sensor, REG_DC_CURRENT);
}

/**
 * @brief Function to read the AC current from the MD0630T01A leakage current sensor.
 * @param sensor Pointer to the LeakSensor object.
 * @return Current AC leakage in mA or -1.0 if there is an error
 */
float MD0630T01A_getACCurrent(LeakSensor *sensor)
{
    return readRegister(sensor, REG_AC_CURRENT);
}

/**
 * @brief Function to read the DC leakage threshold from the MD0630T01A leakage current sensor.
 * @param sensor Pointer to the LeakSensor object.
 * @return DC leakage threshold in mA or -1.0 if there is an error
 */
float MD0630T01A_getDCThreshold(LeakSensor *sensor)
{
    return readRegister(sensor, REG_DC_THRESHOLD);
}

/**
 * @brief Function to read the AC leakage threshold from the MD0630T01A leakage current sensor.
 * @param sensor Pointer to the LeakSensor object.
 * @return AC leakage threshold in mA or -1.0 if there is an error
 */
float MD0630T01A_getACThreshold(LeakSensor *sensor)
{
    return readRegister(sensor, REG_AC_THRESHOLD);
}

/**
 * @brief Function to check if the DC leakage current exceeds the threshold.
 * @param sensor Pointer to the LeakSensor object.
 * @return true if the DC leakage current exceeds the threshold, false otherwise.
 */
bool MD0630T01A_isOverDC(LeakSensor *sensor)
{
    return digitalRead(PIN_DO) == HIGH;
}

/**
 * @brief Function to check if the AC leakage current exceeds the threshold.
 * @param sensor Pointer to the LeakSensor object.
 * @return true if the AC leakage current exceeds the threshold, false otherwise.
 */
bool MD0630T01A_isOverAC(LeakSensor *sensor)
{
    return digitalRead(PIN_AO) == HIGH;
}


/**
 * @brief Function to check if either AC or DC leakage current exceeds the threshold.
 * @param sensor Pointer to the LeakSensor object.
 * @return true if either AC or DC leakage current exceeds the threshold, false otherwise.
 */
bool MD0630T01A_isOverDA(LeakSensor *sensor)
{
    return digitalRead(PIN_DA) == HIGH;
}

