
//  @file main.c

#include "tracker.h"          
#include <stdio.h>               

int main(void) {                 
    char uart_buffer[BUFFER_SIZE];  // Declare a buffer to store received UART characters.
    uint8_t index = 0;         // Initialize an index to track the current position in uart_buffer.
    float price = 0.0f, change = 0.0f;  // Variables to store the parsed BTC price and 24h change percentage.
    char line2[17] = {0};      // A string buffer for formatting the second line of LCD output (16 characters + null terminator).
    
    // Declare an array of threshold values for price alert (from 10,000 to 120,000).
    int thresholds[] = {10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000, 100000, 110000, 120000};
    int total_thresholds = sizeof(thresholds) / sizeof(thresholds[0]);  
    // Calculate the number of thresholds by dividing the total size of the array by the size of one element.
    int adjustable_index = 0;  // Index into the thresholds array; initially set to 0.
    char threshStr[17] = {0};  // Buffer for formatting threshold string for LCD display (16 characters max + null terminator).
    uint32_t elapsed = 0;      // Timer variable to count elapsed time in the threshold adjustment phase.

    // Initialize all peripherals:
    PushButton_Init();         // Initialize push button (GPIO configuration for PF4).
    RGB_LED_Init();            // Initialize the RGB LED (GPIO configuration for PD0 and PD1).
    Buzzer_Init();             // Initialize the buzzer (GPIO configuration for PF1).
    LCD_Init();                // Initialize the LCD (including port setup and command sequence).
    UART1_Init();              // Initialize UART1 (for receiving BTC price data).

    // Threshold adjustment phase: allow the user to select the minimum price value.
    LCD_Clear();               // Clear the LCD screen.
    LCD_Set_Cursor(0, 0);      // Set the cursor to the first column of the first row.
    LCD_Display_String("Set min val:");  // Display the prompt to set the minimum value.
    
    // Format and display the initial threshold value using a fixed field width.
    // "$%-7d" formats the number as left-justified in a field of 7 characters,
    // ensuring that previous characters are overwritten on the LCD.
    sprintf(threshStr, "$%-7d", thresholds[adjustable_index]);
    LCD_Set_Cursor(0, 1);      // Set the cursor to the first column of the second row.
    LCD_Display_String(threshStr);  // Display the threshold string.
    
    // Wait for 4 seconds while monitoring the push button to allow the user to adjust the threshold.
    while (elapsed < 4000) {   // 4000 milliseconds = 4 seconds.
        if (PushButton_Pressed()) {  
            // If the button is pressed, cycle to the next threshold.
            adjustable_index = (adjustable_index + 1) % total_thresholds;
            // Use modulo to wrap the index when reaching the end of the thresholds array.
            sprintf(threshStr, "$%-7d", thresholds[adjustable_index]);
            // Format the new threshold value.
            LCD_Set_Cursor(0, 1);      // Set the cursor to the second row.
            LCD_Display_String(threshStr);  // Update the LCD with the new threshold.
            elapsed = 0;           // Reset the elapsed time to allow further adjustments.
            DelayMs(300);          // Wait 300 ms to debounce and avoid rapid cycling.
        }
        DelayMs(100);             // Delay 100 ms between checks.
        elapsed += 100;           // Increment elapsed time by 100 ms.
    }

    // Save the selected threshold value into the global variable.
    local_threshold = (float)thresholds[adjustable_index];  
    // Convert the selected integer threshold to float.
    
    LCD_Clear();                // Clear the LCD.
    LCD_Set_Cursor(0, 0);       // Set cursor to the first row.
    LCD_Display_String("Threshold Saved");  // Inform the user that threshold is saved.
    DelayMs(3000);              // Wait 3 seconds for the user to read the message.
    LCD_Clear();                // Clear the LCD in preparation for the main loop.

    // Main loop: continuously read UART data, parse price, and update the display/alerts.
    while (1) {
        char c = UART1_Input_Character();  // Get a character from UART.
        // Check if we reached the end of a line (newline or carriage return) or the buffer is nearly full.
        if ((c == '\n') || (c == '\r') || (index >= BUFFER_SIZE - 1)) {
            uart_buffer[index] = '\0';    // Null-terminate the UART buffer to form a valid string.
            // Parse the UART buffer expecting a format: "BTC Price: $<price>, 24h Change: <change>%"
            if (sscanf(uart_buffer, "BTC Price: $%f, 24h Change: %f%%", &price, &change) == 2) {
                // Extract the price and change percentage from the string into variables.
                int intPrice = (int)price;  // Convert the float price to an integer for formatting.
                int thousands = intPrice / 1000;  // Calculate the thousands part (integer division).
                int remainder = intPrice % 1000;  // Calculate the remainder (modulo operation).
                // Format the price and change to show thousands separated by a comma and a 2-decimal change.
                sprintf(line2, "$%d,%03d  %+.2f%%", thousands, remainder, change);

                // Check if the current price is below the user-selected threshold.
                if (price < local_threshold) {
                    // Continue alerting while price is below threshold and the user hasn't pressed the button.
                    while (price < local_threshold && !PushButton_Pressed()) {
                        LCD_Clear();      // Clear the display.
                        char priceStr[17];  // Buffer for formatting the price string.
                        sprintf(priceStr, "$%d,%03d", thousands, remainder);  // Format the price.
                        LCD_Set_Cursor(0, 0);  // Set cursor on the first row.
                        LCD_Display_String(priceStr);  // Display the formatted price.
                        LCD_Set_Cursor(0, 1);  // Set cursor on the second row.
                        LCD_Display_String("BUY NOW");  // Display the alert message.
                        RGB_LED_Flash_Yellow();  // Flash the RGB LED to signal an alert.
                        Buzzer_Toggle();   // Toggle the buzzer to generate an audible alert.
                        DelayMs(150);      // Wait 150 ms between alert updates.
                    }
                    alarmStopped = 1;      // If the loop exits, assume the user has stopped the alarm.
                    Buzzer_Off();          // Make sure the buzzer is turned off.
                }
                if (price >= local_threshold) {  
                    // If the price is above the threshold, ensure the alarm flag is reset.
                    alarmStopped = 0;
                }

                LCD_Clear();            // Clear the LCD.
                LCD_Set_Cursor(0, 0);   // Set the cursor at the beginning of the first row.
                LCD_Display_String("BTC Price:");  // Display a static label.
                LCD_Set_Cursor(0, 1);   // Set the cursor at the beginning of the second row.
                LCD_Display_String(line2);  // Display the formatted price and change string.
                RGB_LED_Set_Normal(change); // Set the LED color according to the price change.
                Buzzer_Off();         // Ensure the buzzer is off when no alert is needed.
            } else {
                // If the UART data doesn't match the expected format, display a "Loading..." message.
                LCD_Clear();
                LCD_Set_Cursor(0, 0);
                LCD_Display_String("Loading...");
                GPIOD->DATA &= ~0x03;  // Turn off the RGB LED.
                Buzzer_Off();          // Turn off the buzzer.
            }
            index = 0;               // Reset the buffer index after processing a complete line.
        } else {
            uart_buffer[index++] = c;  // Append the received character to the buffer and increment the index.
        }
    }
    return 0;                    // End of main (in an embedded system, main usually never returns).
}
