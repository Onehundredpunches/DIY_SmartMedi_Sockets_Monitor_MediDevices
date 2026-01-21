/**
 * @file PZEM016_Lib.cpp
 * @brief Implementation for PZEM016T sensor library.
 * @author Nguyen Minh Tan (Ryan)
 * @date 2025-06-20
 * @license MIT
 */

#include <PZEM016_Lib.h>

float lastPZEMVoltage[NUM_DEVICES] = {0}; // Store last voltage readings for each PZEM016T sensor
float lastPZEMCurrent[NUM_DEVICES] = {0}; // Store last current readings for each PZEM016T sensor
float lastPZEMPower[NUM_DEVICES] = {0};   // Store last power readings for each PZEM016T sensor
float lastPZEMFreq[NUM_DEVICES] = {0};    // Store last frequency readings for each PZEM016T sensor
float lastPZEMPF[NUM_DEVICES] = {0};      // Store last power factor readings for each PZEM016T sensor

float pzemVoltageCalib[NUM_DEVICES] = {0}; // Calibration offsets for each PZEM016T sensor

bool overVoltage[NUM_DEVICES] = {false};     // Array to hold over-voltage state for each PZEM016T sensor
bool overCurrent[NUM_DEVICES] = {false};     // Array to hold over-current state for each PZEM016T sensor
bool overPower[NUM_DEVICES] = {false};       // Array to hold over-power state for each PZEM016T sensor
bool underVoltage[NUM_DEVICES] = {false};    // Array to hold under-voltage state for each PZEM016T sensor
bool socketState[NUM_DEVICES] = {false}; // Array to hold power lost state for each PZEM016T sensor

// Initialize PZEM016T sensors with different addresses
PZEM004Tv30 pzems[NUM_DEVICES] = {
    PZEM004Tv30(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x01),
    PZEM004Tv30(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x02),
    PZEM004Tv30(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x03),
    PZEM004Tv30(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x04),
    PZEM004Tv30(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x05),
    PZEM004Tv30(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x06)};

// Array to store PZEM016T sensor data
PZEMData sensorData[NUM_DEVICES];

// Array to count read failures for each PZEM016T sensor
uint8_t readFailCount[NUM_DEVICES] = {0};

// Define name of devices for each PZEM016T sensor
// These names are used for identification and debugging purposes.
const char *SOCKET_NAMES[NUM_DEVICES] = {
    "AUO_DISPLAY",
    "CCU_IMAGE1_S",
    "CCU_IMAGE_1_HUB",
    "CCU_TRICAM_PAL",
    "XENON_300",
    "ENDOFLATOR_UI400"};

/**
 * @brief Initialize UART for PZEM016T sensors.
 *
 * @details Configures the UART interface for communication with PZEM016T sensors.
 */
void PZEM016_init(void)
{
    PZEM_SERIAL.begin(9600, SERIAL_8N1, PZEM_RX_PIN, PZEM_TX_PIN);
}

/**
 * @brief Kiểm tra xem tất cả các thiết bị có đang mất nguồn không.
 * @return true nếu tất cả các thiết bị đều mất nguồn, false nếu còn thiết bị hoạt động.
 */
bool areAllSocketsPowerLost()
{
    for (int i = 0; i < NUM_DEVICES; ++i)
    {
        if (socketState[i] || sensorData[i].valid) // Nếu có bất kỳ thiết bị nào không mất nguồn
        {
            return false;
        }
    }
    return true; // Tất cả thiết bị đều mất nguồn - true
}

/**
 * @brief Apply a median filter to an array of float data.
 */
float applyMedianFilter(float *data, int size)
{
    float sortedData[size];
    memcpy(sortedData, data, size * sizeof(float)); // Copy data to avoid modifying the original array
    std::sort(sortedData, sortedData + size);       // Sort the copied array
    return sortedData[size / 2];                    // Return the median value
}

/**
 * @brief Read data from the i-th PZEM016T sensor.
 *
 * @param i Index of the sensor to read (0 to NUM_DEVICES-1).
 * @details Reads voltage and current. If successful, updates all sensor data fields and resets the failure counter.
 *          If reading fails, increments the failure counter and prints a warning after 5 consecutive failures.
 */
void readPZEM(uint8_t i)
{
    if (i >= NUM_DEVICES)
    {
        Serial.printf("Error: Tried to read sensor index %d, but only %d sensors are available (0-%d).\n", i, NUM_DEVICES, NUM_DEVICES - 1);
        return;
    }

    // Static variables for Median Filter
    static float voltageHistory[NUM_DEVICES][5] = {0};
    static float currentHistory[NUM_DEVICES][5] = {0};
    static float powerHistory[NUM_DEVICES][5] = {0};
    static float frequencyHistory[NUM_DEVICES][5] = {0};
    static float pfHistory[NUM_DEVICES][5] = {0};

    // Read data from the sensor
    float U = pzems[i].voltage();
    float I = pzems[i].current();
    float P = (i == AUO_DISPLAY) ? pzems[i].power() : U * I;
    float F = pzems[i].frequency();
    float PF = pzems[i].pf();

    // Check if the values are NaN (not a number)
    if (isnan(U) || isnan(I) || isnan(P) || isnan(F) || isnan(PF))
    {
        Serial.printf("Error: NaN detected for device %d. Skipping update.\n", i);
        sensorData[i].valid = false; // Mark data as invalid
        readFailCount[i]++;
        if (readFailCount[i] >= 5)
        {
            Serial.printf("Warning: Device %d failed to read 5 times consecutively due to NaN sensor values.\n", i);
        }
        return;
    }

    // Check if the values are within the valid range
    if (U < PZEM_VOLTAGE_MIN || U > PZEM_VOLTAGE_MAX ||
        I < PZEM_CURRENT_MIN || I > PZEM_CURRENT_MAX ||
        P < PZEM_POWER_MIN || P > PZEM_POWER_MAX ||
        F < PZEM_FREQUENCY_MIN || F > PZEM_FREQUENCY_MAX ||
        PF < PZEM_PF_MIN || PF > PZEM_PF_MAX)
    {
        Serial.printf("Out-of-range data from sensor (device %d): Voltage: %.2f (%.2f~%.2f), Current: %.2f (%.2f~%.2f), Power: %.2f (%.2f~%.2f), Frequency: %.2f (%.2f~%.2f), PF: %.2f (%.2f~%.2f)\n",
                      i, U, PZEM_VOLTAGE_MIN, PZEM_VOLTAGE_MAX,
                      I, PZEM_CURRENT_MIN, PZEM_CURRENT_MAX,
                      P, PZEM_POWER_MIN, PZEM_POWER_MAX,
                      F, PZEM_FREQUENCY_MIN, PZEM_FREQUENCY_MAX,
                      PF, PZEM_PF_MIN, PZEM_PF_MAX);
        sensorData[i].valid = false; // Mark data as invalid
        readFailCount[i]++;
        if (readFailCount[i] >= 5)
        {
            Serial.printf("Warning: Device %d failed to read 5 times consecutively due to out-of-range sensor values.\n", i);
        }
        return;
    }

    // Nếu dòng điện về 0, cập nhật ngay lập tức
    if (I < pzemThresholds[i].current_min) // Dòng điện < ngưỡng dòng điện tối thiểu
    {
        sensorData[i].voltage = U;
        sensorData[i].current = I;
        sensorData[i].power = P;
        sensorData[i].frequency = F;
        sensorData[i].pf = PF;
        sensorData[i].valid = true;

        // Reset failure count if data is valid
        readFailCount[i] = 0;

        return;
    }

    // Initialize history arrays with the first valid reading
    static bool isHistoryInitialized[NUM_DEVICES] = {false};
    if (!isHistoryInitialized[i])
    {
        for (int j = 0; j < 5; j++)
        {
            voltageHistory[i][j] = U;
            currentHistory[i][j] = I;
            powerHistory[i][j] = P;
            frequencyHistory[i][j] = F;
            pfHistory[i][j] = PF;
        }
        isHistoryInitialized[i] = true;
    }

    // Update history for Median Filter
    for (int j = 4; j > 0; j--)
    {
        voltageHistory[i][j] = voltageHistory[i][j - 1];
        currentHistory[i][j] = currentHistory[i][j - 1];
        powerHistory[i][j] = powerHistory[i][j - 1];
        frequencyHistory[i][j] = frequencyHistory[i][j - 1];
        pfHistory[i][j] = pfHistory[i][j - 1];
    }
    voltageHistory[i][0] = U;
    currentHistory[i][0] = I;
    powerHistory[i][0] = P;
    frequencyHistory[i][0] = F;
    pfHistory[i][0] = PF;

    // Apply Median Filter
    float voltageMedian = applyMedianFilter(voltageHistory[i], 5);
    float currentMedian = applyMedianFilter(currentHistory[i], 5);
    float powerMedian = applyMedianFilter(powerHistory[i], 5);
    float frequencyMedian = applyMedianFilter(frequencyHistory[i], 5);
    float pfMedian = applyMedianFilter(pfHistory[i], 5);

    // Update sensor data after filtering
    sensorData[i].voltage = voltageMedian;
    sensorData[i].current = currentMedian;
    sensorData[i].power = powerMedian;
    sensorData[i].frequency = frequencyMedian;
    sensorData[i].pf = pfMedian;
    sensorData[i].valid = true;

    // Reset failure count if data is valid
    readFailCount[i] = 0;

}

/**
 * @brief Calculate the delta value for a given measurement.
 *
 * @param value The measured value.
 * @param accuracy_pct The desired accuracy as a percentage (e.g., 0.01 for 1%).
 * @param resolution The resolution of the measurement system.
 * @param sigma The process noise standard deviation.
 * @param F_u The sensitivity of the measurement system.
 * @param N_lsb The number of least significant bits.
 * @param delta_min The minimum delta value.
 * @return The calculated delta value.
 */
float calculateDelta(float value, float accuracy_pct, float resolution, float sigma, float F_u, int N_lsb, float delta_min)
{
    float delta = F_u * (fabs(value) * accuracy_pct + sigma + N_lsb * resolution);
    return (delta > delta_min) ? delta : delta_min;
}