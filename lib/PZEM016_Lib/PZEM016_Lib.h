/**
 * @file PZEM016_Lib.h
 * @brief Library for handling PZEM016T sensors.
 * @author Nguyen Minh Tan (Ryan)
 * @date 2025-06-20
 * @license MIT
 */

#ifndef PZEM016_Lib_H
#define PZEM016_Lib_H

#include <PZEM004Tv30.h>
#include <algorithm> // For std::sort

#ifdef __cplusplus
extern "C"
{
#endif

// RX/TX pin for PZEM016T sensors
#define PZEM_RX_PIN 17
#define PZEM_TX_PIN 16

#define PZEM_SERIAL Serial2 // Serial port for PZEM016T sensors

#define PZEM_VOLTAGE_MIN 80.0f  // V, define minimum valid voltage
#define PZEM_VOLTAGE_MAX 260.0f // V, define maximum valid voltage
#define PZEM_CURRENT_MIN 0.0f   // A, define minimum valid current
#define PZEM_CURRENT_MAX 100.0f   // A, define maximum valid current
#define PZEM_POWER_MIN 0.0f     // W, define minimum valid power
#define PZEM_POWER_MAX 23000.0f  // W, define maximum valid power
#define PZEM_FREQUENCY_MIN 45.0f     // Hz, define minimum valid frequency
#define PZEM_FREQUENCY_MAX 65.0f     // Hz, define maximum valid frequency
#define PZEM_PF_MIN 0.0f        // Power Factor, define minimum valid power factor
#define PZEM_PF_MAX 1.0f        // Power Factor, define maximum valid power factor

// Delta calculation parameters (có thể chỉnh sửa cho từng loại cảm biến)
#define PZEM_DELTA_FU         1.2f   // Hệ số an toàn
#define PZEM_DELTA_N_LSB      1      // Số bậc LSB tối thiểu
#define PZEM_DELTA_VOLTAGE_MIN 0.2f  // Ngưỡng tuyệt đối nhỏ nhất cho điện áp
#define PZEM_DELTA_CURRENT_MIN 0.01f
#define PZEM_DELTA_POWER_MIN   0.2f
#define PZEM_DELTA_FREQ_MIN    0.2f
#define PZEM_DELTA_PF_MIN      0.03f

// Độ chính xác và bước đo của cảm biến PZEM016T (có thể chỉnh sửa nếu cần)
#define PZEM_ACCURACY_VOLTAGE 0.005f  // 0.5%
#define PZEM_ACCURACY_CURRENT 0.005f  // 0.5%
#define PZEM_ACCURACY_POWER   0.005f  // 0.5%
#define PZEM_ACCURACY_FREQ    0.005f  // 0.5%
#define PZEM_ACCURACY_PF      0.01f   // 1%

#define PZEM_RESOLUTION_VOLTAGE 0.1f
#define PZEM_RESOLUTION_CURRENT 0.001f
#define PZEM_RESOLUTION_POWER   0.1f
#define PZEM_RESOLUTION_FREQ    0.1f
#define PZEM_RESOLUTION_PF      0.01f

// Độ lệch chuẩn nhiễu thực nghiệm (có thể đo thực tế để chỉnh)
#define PZEM_SIGMA_VOLTAGE 0.1f
#define PZEM_SIGMA_CURRENT 0.01f
#define PZEM_SIGMA_POWER   0.1f
#define PZEM_SIGMA_FREQ    0.1f
#define PZEM_SIGMA_PF      0.01f

#define PZEM_VOLTAGE_SYNC_THRESHOLD 5.0f // Ngưỡng lệch tối đa cho phép để đồng bộ điện áp
#define PZEM0_VOLTAGE_OFFSET 0.0f               // Giá trị hiệu chỉnh thủ công cho cảm biến gốc (PZEM1) 


extern float pzemVoltageCalib[]; // Array to hold the calibration offsets for each PZEM016T sensor   

    // Define the PZEM016T sockets
    // These are the identifiers for each PZEM016T sensor socket.
    typedef enum
    {
        AUO_DISPLAY = 0,
        CCU_IMAGE1_S,
        CCU_IMAGE_1_HUB,
        CCU_TRICAM_PAL,
        XENON_300,
        ENDOFLATOR_UI400,
        NUM_DEVICES
    } SOCKET_ID;

    // Define name of devices for each PZEM016T sensor
    // These names are used for identification and debugging purposes.
    extern const char *SOCKET_NAMES[NUM_DEVICES];

    // Declear global variables for last readings
    // These variables are used to store the last readings from each PZEM016T sensor for delta checking.
    extern float lastPZEMVoltage[NUM_DEVICES];    // Store last voltage readings for each PZEM016T sensor
    extern float lastPZEMCurrent[NUM_DEVICES];    // Store last current readings for each PZEM016T sensor
    extern float lastPZEMPower[NUM_DEVICES];      // Store last power readings for each PZEM016T sensor
    extern float lastPZEMFreq[NUM_DEVICES];       // Store last frequency readings for each PZEM016T sensor
    extern float lastPZEMPF[NUM_DEVICES];         // Store last power factor readings for each PZEM016T sensor

    extern bool overVoltage[NUM_DEVICES]; // Array to hold over-voltage state for each PZEM016T sensor
    extern bool overCurrent[NUM_DEVICES]; // Array to hold over-current state for each PZEM016T sensor
    extern bool overPower[NUM_DEVICES];   // Array to hold over-power state for each PZEM016T sensor
    extern bool underVoltage[NUM_DEVICES]; // Array to hold under-voltage state for each PZEM016T sensor
    extern bool socketState[NUM_DEVICES]; // Array to hold power lost state for each PZEM016T sensor

    // Define the thresholds for each PZEM016T sensor
    // These thresholds are used to validate the readings from each PZEM016T sensor.
    typedef struct
    {
        float voltage_min;   // Minimum voltage (V)
        float voltage_max;   // Maximum voltage (V)
        float current_min;   // Minimum current (A)
        float current_max;   // Maximum current (A)
        float power_min;     // Minimum power (W)
        float power_max;     // Maximum power (W)
        float frequency_min; // Minimum frequency (Hz)
        float frequency_max; // Maximum frequency (Hz)
    } PZEM_Thresholds;

    // Set the thresholds for each PZEM016T sensor
    // These thresholds are used to validate the readings from each PZEM016T sensor.
    // [DEVICE_NAME] = {voltage_min, voltage_max, current_min, current_max, power_min, power_max, frequency_min, frequency_max}
    static const PZEM_Thresholds pzemThresholds[NUM_DEVICES] = {
        [AUO_DISPLAY]       = {218.0f, 240.0f, 0.1f, 0.65f, 0.0f, 142.5f, 49.5f, 50.5f}, //P (W)
        [CCU_IMAGE1_S]      = {218.0f, 240.0f, 0.01f, 0.534f, 0.0f, 128.25f, 49.5f, 50.5f},
        [CCU_IMAGE_1_HUB]   = {218.0f, 240.0f, 0.01f, 0.38f, 0.0f, 91.2f, 49.5f, 50.5f}, 
        [CCU_TRICAM_PAL]    = {218.0f, 240.0f, 0.01f, 0.237f, 0.0f, 57.0f, 49.5f, 50.5f},
        [XENON_300]         = {218.0f, 240.0f, 0.01f, 1.78f, 0.0f, 427.5f, 49.5f, 50.5f},
        [ENDOFLATOR_UI400]  = {218.0f, 240.0f, 0.01f, 1.52f, 0.0f, 364.8f, 49.5f, 50.5f}};

    /**
     * @brief Struct to hold PZEM data.
     * This struct contains the electrical parameters read from each PZEM016T sensor.
     * It includes voltage, current, power, energy, frequency, power factor (pf), state of device and a validity flag.
     */
    typedef struct
{
    float voltage;      ///< Voltage in V
    float current;      ///< Current in A
    float power;        ///< Power in W
    float energy;       ///< Energy in Wh
    float frequency;    ///< Frequency in Hz
    float pf;           ///< Power factor
    bool machineState;  ///< true = ON, false = OFF
    bool valid;         ///< true = data valid, false = invalid
} PZEMData;
    
    extern PZEM004Tv30 pzems[NUM_DEVICES];     // Array of PZEM004Tv30 objects for each sensor
    extern PZEMData sensorData[NUM_DEVICES];   // Array to hold data for each PZEM016T sensor
    extern uint8_t readFailCount[NUM_DEVICES]; // Array to count read failures for each sensor

    /**
     * @brief Initialize UART for PZEM016T sensors.
     * This function sets up the serial communication for PZEM016T sensors.
     */
    extern void PZEM016_init(void);

    /**
     * @brief Read data from the i-th PZEM016T sensor.
     * @param i Index of the sensor to read (0 to NUM_DEVICES-1).
     */
    extern void readPZEM(uint8_t i);
   
    /**
     * @brief Reset the energy readings of all PZEM016T sensors to zero.
     * This function sends a reset command to each PZEM016T sensor to clear its energy counter.
     */
    extern bool areAllSocketsPowerLost(); // Kiểm tra xem tất cả các thiết bị có đang mất nguồn không

    /**
     * @brief Apply a median filter to the data array.
     * @param data Pointer to the array of float data to be filtered.
     * @param size Size of the data array.
     * @return The median value from the filtered data.
     */
    extern float applyMedianFilter(float *data, int size); // Hàm lọc trung vị

    /**
     * @brief Calculate the delta value for a given parameter.
     * @param value Current value of the parameter.
     * @param accuracy_pct Accuracy percentage of the sensor.
     * @param resolution Resolution of the sensor.
     * @param sigma Standard deviation of noise for the parameter.
     * @param F_u Safety factor.
     * @param N_lsb Minimum number of LSB steps.
     * @param delta_min Minimum absolute delta threshold.
     * @return Calculated delta value.
     */
    // Hàm tính delta cho từng thông số
    float calculateDelta(float value, float accuracy_pct, float resolution, float sigma, float F_u, int N_lsb, float delta_min);

#ifdef __cplusplus
}
#endif

#endif // PZEM016_Lib_H
