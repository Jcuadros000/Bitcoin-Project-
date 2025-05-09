//tracker.c

#include "tracker.h"            

// Global variable definitions:
float local_threshold = 0.0f;     // Initialize the threshold value used for comparisons to 0.0 (will be set later)
int alarmStopped = 0;             // Initialize the alarm flag to 0 (alarm not stopped)

// Delay routine: create a delay of 'ms' milliseconds.
void DelayMs(uint32_t ms) {       
    SysTick->LOAD = (SystemCoreClock / 1000U) - 1;  
    // Calculation: Each millisecond delay requires (SystemCoreClock / 1000) ticks.
    // For a 50 MHz clock: (50,000,000 / 1000) = 50,000 ticks per millisecond. Subtract 1 because the timer counts from LOAD down to 0.
    SysTick->VAL = 0;           // Clear the current value register to start counting from LOAD value.
    SysTick->CTRL = 5;          // Enable SysTick timer in processor clock mode (bit 0 = ENABLE, bit 2 = CLKSOURCE). No interrupt.
    while(ms--) {               // Loop for the number of milliseconds
        while((SysTick->CTRL & (1 << 16)) == 0) { } // Wait until the COUNTFLAG (bit 16) is set, meaning one millisecond has passed.
    }
    SysTick->CTRL = 0;          // Disable the SysTick timer after the delay.
}

// LCD initialization functions:

void LCD_Port_Init(void) {       
    SYSCTL->RCGCGPIO |= 0x01 | 0x04 | 0x10;  
    // Enable clock for GPIO Port A (0x01), Port C (0x04), and Port E (0x10)
    volatile unsigned long dummy = SYSCTL->RCGCGPIO;  
    // Dummy read to allow clock to stabilize.
    (void)dummy;                // Explicitly ignore the dummy variable.

    GPIOA->DIR |= 0x3C;         // Set PA2-PA5 as output for LCD data (bits 2,3,4,5 -> 0x3C)
    GPIOA->DEN |= 0x3C;         // Enable digital function on PA2-PA5.
    GPIOC->DIR |= 0x40;         // Set PC6 as output (used for LCD enable pulse).
    GPIOC->DEN |= 0x40;         // Enable digital function on PC6.
    GPIOE->DIR |= 0x01;         // Set PE0 as output (used for LCD RS, Register Select).
    GPIOE->DEN |= 0x01;         // Enable digital function on PE0.
}

void LCD_Pulse_Enable(void) {
    GPIOC->DATA = (GPIOC->DATA & ~0xFF) | 0x40;  // Set PC6 high to generate an enable pulse for the LCD.
    DelayMs(1);                  // Wait for 1 millisecond for the pulse to be recognized.
    GPIOC->DATA = (GPIOC->DATA & ~0xFF) & ~0x40;   // Set PC6 low to complete the pulse.
    DelayMs(1);                  // Wait 1 millisecond for settling.
}

void LCD_Write_4_Bits(unsigned char nibble) {
    // Clear bits corresponding to LCD data (PA2-PA5) and write the lower 4 bits of 'nibble' shifted into PA2-PA5.
    GPIOA->DATA = (GPIOA->DATA & ~0x3C) | ((nibble & 0x0F) << 2);
    LCD_Pulse_Enable();          // Pulse the enable signal to latch the data into the LCD.
}

void LCD_Send_Command(unsigned char cmd) {
    GPIOE->DATA &= ~0x01;       // Clear RS (Register Select) on PE0 to indicate a command.
    LCD_Write_4_Bits(cmd >> 4);   // Send the high nibble (upper 4 bits) of the command.
    LCD_Write_4_Bits(cmd & 0x0F); // Send the low nibble (lower 4 bits) of the command.
    DelayMs(2);                  // Wait for 2 milliseconds for the command to process.
}

void LCD_Send_Data(unsigned char data) {
    GPIOE->DATA |= 0x01;        // Set RS (Register Select) on PE0 to indicate data.
    LCD_Write_4_Bits(data >> 4);  // Send high nibble of data.
    LCD_Write_4_Bits(data & 0x0F); // Send low nibble of data.
    DelayMs(2);                  // Wait for 2 milliseconds to allow the data to be latched.
}

void LCD_Set_Cursor(unsigned char col, unsigned char row) {
    // Calculate the address based on the row (0 or 1) and column
    unsigned char addr = (row == 0 ? 0x00 : 0x40) + (col & 0x0F);
    // For row 0, start at 0x00; for row 1, start at 0x40. (col & 0x0F ensures column is within 0-15.)
    LCD_Send_Command(0x80 | addr);  // Set DDRAM address command: bit 7 high, then address.
}

void LCD_Init(void) {
    LCD_Port_Init();            // Initialize the LCD GPIO ports.
    DelayMs(40);                // Wait 40 ms for LCD power up.
    LCD_Write_4_Bits(0x03);     // Send "0x03" to initialize in 8-bit mode (first step in initialization).
    DelayMs(5);                 // Wait 5 ms.
    LCD_Write_4_Bits(0x03);     // Repeat the 0x03 command.
    DelayMs(1);                 // Wait 1 ms.
    LCD_Write_4_Bits(0x03);     // Send 0x03 a third time.
    DelayMs(1);                 // Wait 1 ms.
    LCD_Write_4_Bits(0x02);     // Send 0x02 to set the LCD to 4-bit mode.
    LCD_Send_Command(0x28);     // Function set: 4-bit, 2-line, 5x8 dots (0x28 = 0010 1000).
    LCD_Send_Command(0x08);     // Display off command.
    LCD_Send_Command(0x01);     // Clear display command.
    DelayMs(2);                 // Wait 2 ms for the clear command to complete.
    LCD_Send_Command(0x06);     // Entry mode set: increment automatically, no display shift.
    LCD_Send_Command(0x0C);     // Display on, cursor off command.
}

void LCD_Clear(void) {
    LCD_Send_Command(0x01);     // Send the clear display command.
    DelayMs(2);                 // Wait for 2 ms so that the display can clear.
}

void LCD_Display_String(const char *str) {
    // Loop through each character in the string until the null terminator is reached.
    while (*str)
        LCD_Send_Data(*str++); // Send each character to the LCD and advance to the next.
}

// UART functions:

void UART1_Init(void) {
    SYSCTL->RCGCUART |= 0x02;   // Enable the UART1 module clock (bit 1 corresponds to UART1).
    SYSCTL->RCGCGPIO |= 0x02;   // Enable the GPIO port clock for Port B (for UART1 pins).
    while ((SYSCTL->PRGPIO & 0x02) == 0) { }  // Wait until Port B is ready.
    
    UART1->CTL &= ~0x0001;      // Disable UART1 during configuration (clear UARTEN, bit 0).
    // Baud rate setup:
    // Equation: Baud Rate Divider = SystemClock / (16 * Baud Rate)
    // For a desired baud of 115200 and SystemClock = 50,000,000:
    // Divider = 50,000,000 / (16 * 115200) ˜ 27.1267; Integer part = 27, Fraction = 0.1267 * 64 ˜ 8.11 -> FBRD = 8.
    UART1->IBRD = 27;           // Set the integer baud rate divisor to 27.
    UART1->FBRD = 8;            // Set the fractional baud rate divisor to 8.
    UART1->LCRH = (0x3 << 5) | (1 << 4);  
    // Set word length to 8 bits (0x3 << 5) and enable FIFOs (bit 4).
    UART1->CTL |= 0x0301;       // Enable UART1: set UARTEN (bit 0), TXE (bit 8), and RXE (bit 9).

    GPIOB->AFSEL |= 0x03;       // Enable alternate functions on PB0 and PB1 for UART.
    GPIOB->PCTL = (GPIOB->PCTL & ~0xFF) | 0x11;  
    // Configure PB0 and PB1 for UART (PCTL value 0x1 for each pin), preserving other bits.
    GPIOB->DEN |= 0x03;         // Enable digital functionality on PB0 and PB1.
}

char UART1_Input_Character(void) {
    while (UART1->FR & 0x10) { }  // Wait while the Receive FIFO is empty (check bit 4 in FR register).
    return (char)(UART1->DR & 0xFF);  // Read the received data (mask with 0xFF to get only the lower 8 bits) and return it.
}

// Push Button functions:

void PushButton_Init(void) {
    SYSCTL->RCGCGPIO |= 0x20;   // Enable the clock for GPIO Port F (0x20 corresponds to Port F).
    while ((SYSCTL->PRGPIO & 0x20) == 0) { }  // Wait until Port F is ready.
    GPIOF->DIR &= ~0x10;        // Set PF4 (push button) as input (clear bit 4).
    GPIOF->DEN |= 0x10;         // Enable digital functionality on PF4.
    GPIOF->PUR |= 0x10;         // Enable internal pull-up resistor on PF4.
}

int PushButton_Pressed(void) {
    // Returns true (non-zero) if the push button is pressed (active low), else false (0).
    return ((GPIOF->DATA & 0x10) == 0);
}

// RGB LED functions:

void RGB_LED_Init(void) {
    SYSCTL->RCGCGPIO |= 0x08;   // Enable the clock for GPIO Port D (0x08 corresponds to Port D).
    while ((SYSCTL->PRGPIO & 0x08) == 0) { }  // Wait until Port D is ready.
    GPIOD->DIR |= 0x03;         // Set PD0 and PD1 as outputs (for two channels of an RGB LED).
    GPIOD->DEN |= 0x03;         // Enable digital functionality on PD0 and PD1.
    GPIOD->DATA &= ~0x03;       // Initialize PD0 and PD1 to low (LEDs off).
}

void RGB_LED_Set_Normal(float change) {
    // Change > 0.001: indicate positive change (e.g., green or blue)
    if (change > 0.001)
        GPIOD->DATA = (GPIOD->DATA & ~0x03) | 0x02;  // Turn on one LED channel (e.g., PD1)
    // Change < -0.001: indicate negative change (e.g., red)
    else if (change < -0.001)
        GPIOD->DATA = (GPIOD->DATA & ~0x03) | 0x01;  // Turn on the other LED channel (e.g., PD0)
    else
        GPIOD->DATA &= ~0x03;   // Otherwise, turn all off if change is near zero.
}

void RGB_LED_Flash_Yellow(void) {
    static int led_state = 0;   // Static variable to retain state between function calls.
    if (led_state) {
        GPIOD->DATA &= ~0x03;   // Turn off both LED channels.
        led_state = 0;          // Update state to indicate LEDs are off.
    } else {
        GPIOD->DATA = (GPIOD->DATA & ~0x03) | 0x03;  // Turn on both LED channels to create a yellow color.
        led_state = 1;          // Update state to indicate LEDs are on.
    }
}

// Buzzer functions:

void Buzzer_Init(void) {
    SYSCTL->RCGCGPIO |= 0x20;   // Enable the clock for GPIO Port F (0x20 corresponds to Port F).
    while ((SYSCTL->PRGPIO & 0x20) == 0) { }  // Wait until Port F is ready.
    GPIOF->DIR |= 0x02;         // Set PF1 as output for the buzzer.
    GPIOF->DEN |= 0x02;         // Enable digital functionality on PF1.
    GPIOF->DATA &= ~0x02;       // Initialize the buzzer to be off (PF1 low).
}

void Buzzer_Toggle(void) {
    static int buzzer_state = 0;  // Retain buzzer state between calls.
    if (buzzer_state) {
        GPIOF->DATA &= ~0x02;   // Turn off the buzzer (clear PF1).
        buzzer_state = 0;       // Update state to off.
    } else {
        GPIOF->DATA |= 0x02;    // Turn on the buzzer (set PF1).
        buzzer_state = 1;       // Update state to on.
    }
}

void Buzzer_Off(void) {
    GPIOF->DATA &= ~0x02;       // Turn off the buzzer by clearing PF1.
}
