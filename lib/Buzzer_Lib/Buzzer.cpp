/**
 * @file Buzzer.cpp
 * @brief Implementation for buzzer control library.
 * @author Nguyen Minh Tan (Ryan)
 * @date 2025-07-01
 * @license MIT
 */

#include "Buzzer.h"

// Declare array for the melody and duration of the startup tone
uint16_t melody[] = {1047, 1319, 1568, 1319, 1047, 1568, 2093}; 
                //    C6,   E6,   G6,   E6,   C6,   G6,   C7
uint16_t duration[] = {120, 120, 120, 120, 120, 180, 350}; // milliseconds for each note

/**
 * @brief Initializes the buzzer.
 * This function sets up the PWM channel for the buzzer and plays the startup tone.
 */
void initBuzzer(void)
{
    ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(BUZZER_PIN, PWM_CHANNEL);
    
    playStartupTone();
}

/**
 * @brief Turns off the buzzer sound.
 * This function disables the sound by setting the PWM channel frequency to 0.
 */
void noTone(void)
{
    ledcWriteTone(PWM_CHANNEL, 0); // Turn off sound
}

/**
 * @brief Plays the startup tone.
 * This function plays a short sequence of tones to indicate successful system startup.
 * The sequence consists of 3 short beeps and 1 long beep.
 */
void playStartupTone(void)
{
    for (int i = 0; i < 7; i++)
    {
        ledcWriteTone(PWM_CHANNEL, melody[i]);
        delay(duration[i]);
        ledcWriteTone(PWM_CHANNEL, TONE_MIN_FREQ);
        delay(40);
    }
}

/**
 * @brief Plays a warning tone.
 * This function plays a sequence of high-frequency tones as an alert.
 * The sequence consists of 3 high and clear beeps, repeated multiple times.
 */
void playWarningTone(void)
{
    for (int repeat = 0; repeat < ALARM_REPEAT; repeat++)
    {
        for (int i = 0; i < 3; i++)
        {
            ledcWriteTone(PWM_CHANNEL, TONE_MAX_FREQ); 
            delay(120);
            ledcWriteTone(PWM_CHANNEL, TONE_MIN_FREQ);
            delay(80);
        }
        delay(250); 
    }
    ledcWriteTone(PWM_CHANNEL, TONE_MIN_FREQ); 
}


/**
 * @brief Plays the "Happy Birthday" melody.
 * This function plays the "Happy Birthday" song using the buzzer.
 */
void playHappyBirthdayTone(void)
{
    // Nốt nhạc của bài "Happy Birthday"
    uint16_t happyBirthdayMelody[] = {
        262, 262, 294, 262, 349, 330, // Happy Birthday to You
        262, 262, 294, 262, 392, 349, // Happy Birthday to You
        262, 262, 523, 440, 349, 330, 294, // Happy Birthday Dear [Name]
        466, 466, 440, 349, 392, 349       // Happy Birthday to You
    };

    // Độ dài của từng nốt nhạc (ms)
    uint16_t happyBirthdayDuration[] = {
        250, 250, 500, 500, 500, 1000, // Happy Birthday to You
        250, 250, 500, 500, 500, 1000, // Happy Birthday to You
        250, 250, 500, 500, 500, 500, 500, // Happy Birthday Dear [Name]
        250, 250, 500, 500, 500, 1000       // Happy Birthday to You
    };

    // Phát bài hát
    for (int i = 0; i < sizeof(happyBirthdayMelody) / sizeof(happyBirthdayMelody[0]); i++)
    {
        ledcWriteTone(PWM_CHANNEL, happyBirthdayMelody[i]); // Phát nốt nhạc
        delay(happyBirthdayDuration[i]);                   // Đợi thời gian của nốt nhạc
        ledcWriteTone(PWM_CHANNEL, TONE_MIN_FREQ);         // Tắt âm giữa các nốt
        delay(50);                                         // Đợi một chút giữa các nốt
    }
}