/**
 * @file Buzzer.h
 * @brief Library for buzzer control.
 * @author Nguyen Minh Tan (Ryan)
 * @date 2025-07-01
 * @license MIT
 */

#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif


#define BUZZER_PIN 26 // Pin connected to the buzzer

#define TONE_MIN_FREQ 0  // Minimum tone frequency
#define TONE_MAX_FREQ 2000 // Maximum tone frequency
#define TONE_STEPS 100 // Frequency step between tones
#define TONE_DURATION 20 // Duration of each tone (ms)
#define ALARM_REPEAT 5 // Number of times to repeat the warning tone
#define PWM_CHANNEL 0 // PWM channel used for the buzzer
#define PWM_FREQUENCY 2000 // PWM frequency
#define PWM_RESOLUTION 8 // PWM resolution (bits)

extern uint16_t melody[]; // Array containing the frequencies of the notes
extern uint16_t duration[]; // Array containing the duration of each note

/**
 * @brief Initializes the buzzer.
 * This function sets up the PWM channel for the buzzer and plays a startup sound.
 */
extern void initBuzzer(void);

/**
 * @brief Turns off the buzzer sound.
 */
extern void noTone(void);

/**
 * @brief Plays the startup sound.
 * This function plays a short tone to indicate that the system has started successfully.
 */
extern void playStartupTone(void);

/**
 * @brief Plays the warning sound.
 * This function plays a sequence of tones with increasing frequency from TONE_MIN_FREQ to TONE_MAX_FREQ.
 */
extern void playWarningTone(void);


/**
 * @brief Plays the "Happy Birthday" melody.
 * This function plays the "Happy Birthday" song using the buzzer.
 */
extern void playHappyBirthdayTone(void);


#ifdef __cplusplus
}
#endif

#endif // BUZZER_H
