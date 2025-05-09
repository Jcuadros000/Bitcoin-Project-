//tracker.h
#ifndef TRACKER_H                 // Prevent multiple inclusions: if TRACKER_H is not defined...
#define TRACKER_H                 // ...define TRACKER_H to signal this header has been included

#include "TM4C123GH6PM.h"         // Include the microcontroller-specific header containing register definitions
#include <stdio.h>                // Include the standard I/O library (needed for sprintf, etc.)

#define SystemCoreClock 50000000U  // Define the system core clock as 50,000,000 cycles per second (50 MHz)
// Explanation: The system clock is set in hardware. Here, 50e6 cycles/second is used for timing functions.
#define BUFFER_SIZE 128           // Define the size of the UART input buffer as 128 bytes

// Declaration of global variables used across modules:
extern float local_threshold;     // 'local_threshold' holds the selected threshold value for price comparison
extern int alarmStopped;          // 'alarmStopped' is a flag indicating if the alarm has been stopped

// Function prototype declarations:

// Delay routine: creates a blocking delay in milliseconds.
// 'ms' is the number of milliseconds to delay.
void DelayMs(uint32_t ms);      

// LCD (Liquid Crystal Display) related function prototypes:
void LCD->Port->Init(void);         // Initialize the GPIO ports used by the LCD
void LCD_Pulse_Enable(void);      // Generate an enable pulse to latch data into the LCD
void LCD_Write_4_Bits(unsigned char nibble);  // Write a 4-bit nibble to the LCD data bus
void LCD_Send_Command(unsigned char cmd);     // Send a command byte to the LCD
void LCD_Send_Data(unsigned char data);       // Send a data byte (character) to the LCD
void LCD_Set_Cursor(unsigned char col, unsigned char row);  // Set the cursor to a specific column and row on the LCD
void LCD_Init(void);              // Initialize the LCD (4-bit mode, clear display, etc.)
void LCD_Clear(void);             // Clear the LCD display
void LCD_Display_String(const char *str);  // Display a null-terminated string on the LCD

// UART (Universal Asynchronous Receiver/Transmitter) function prototypes:
void UART1_Init(void);            // Initialize UART1 for serial communication
char UART1_Input_Character(void); // Retrieve a single character from the UART1 receive buffer

// Push Button function prototypes:
void PushButton_Init(void);       // Initialize the push button (set direction, enable pull-up resistor)
int PushButton_Pressed(void);     // Check and return whether the push button is currently pressed

// RGB LED (Red, Green, Blue Light Emitting Diode) function prototypes:
void RGB_LED_Init(void);          // Initialize the GPIO ports for the RGB LED
void RGB_LED_Set_Normal(float change);  // Set the LED color based on a given change value
void RGB_LED_Flash_Yellow(void);  // Flash the RGB LED yellow (used as an alert indication)

// Buzzer function prototypes:
void Buzzer_Init(void);           // Initialize the GPIO port for the buzzer
void Buzzer_Toggle(void);         // Toggle the buzzer on and off
void Buzzer_Off(void);            // Turn the buzzer off

#endif // TRACKER_H            // End of inclusion guard for tracker.h
