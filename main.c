#pragma config OSC = HSPLL, FCMEN = OFF, IESO = OFF, PWRT = OFF, BOREN = OFF, WDT = OFF, MCLRE = ON, LPT1OSC = OFF, LVP = OFF, XINST = OFF, DEBUG = OFF

#include <xc.h>
#include "breakpoints.h"

/* 7 segment numbers:
 * 0 : 11111100
 * 1 : 01100000
 * 2 : 11011010
 * 3 : 11110010
 * 4 : 01100110
 * 5 : 10110110
 * 6 : 10111110
 * 7 : 11100000
 * 8 : 11111110
 * 9 : 11100110
 */

volatile int adc_value;
volatile char special;
char adc_flag;
char temp_adc_high;
char temp_adc_low;
char end_game_flag;

int counter_10;     // 1ms interrupt counter, for 10ms debounce duration
char rb4_state;      // 0: Waiting for an input, 1: On debounce
char rb4_pressed;    // Will be set when rb4 press is validated after 10ms
char press_flag;     // Rb4 press in state 0 sets this flag
char debounce_flag;  /* Rb4 change in state 1 or 
                         Timer2 interrupt in state 1 sets this flag */
char correct_guess_flag;

void __interrupt(high_priority) high_isr() {
    
}

void __interrupt(low_priority) low_isr() {
    if (TMR2IF == 1) {
        if (rb4_state == 1) {
            debounce_flag = 1;
        }
        TMR2IF = 0;
    }
    if (RBIF == 1) {
        if (rb4_state == 0) {
            if (PORTBbits.RB4 == 1) {
                press_flag = 1;
            }
        }
        else {
            debounce_flag = 1;
        }
        int read = PORTB;
        RBIF = 0;
    }
}

void rb4_init() {
    // Timer2 Configuration
    TMR2IP = 0;          // Low priority
    TMR2IF = 0;          // Clear flag
    TMR2IE = 1;          // Enable interrupt
    T2CON  = 0b01111011; // Set scalers
    PR2    = 39;         // For 1ms
    TMR2   = 0;          // Reset value
    
    // Rb4 Configuration
    int read = PORTB;     
    RBIF  = 0;            // Clear flag 
    RBIP  = 0;            // Low priority
    RBIE  = 1;            // Enable PORTB interrupts
    TRISB = 0b00010000;   // Set RB4 as input
       
    // Reset flags
    rb4_pressed        = 0;
    rb4_state          = 0;
    correct_guess_flag = 0;
    press_flag         = 0;
    debounce_flag      = 0;
    
    // Reset arrow leds
    TRISC = 0;
    TRISD = 0;
    TRISE = 0;
    
    // Enable interrupts
    IPEN = 1;
    GIEH = 1;
    GIEL = 1;
}

/** Call right after the press 
    Set counter, set state and enable timer2 **/
void on_rb4_press() {
    counter_10 = 10;
    rb4_state = 1;
    TMR2ON = 1;
}

/** This function will be called every 1ms for 10ms
 *  RB4: 1 -> Decrement counter until it hits 0, then validate the press
 *       0 -> Noise detected, abort  **/
void rb4_debounce() {
    if (PORTBbits.RB4 == 1) {
        if (counter_10 == 0) {
            // validate press
            // reset & disable
            rb4_pressed = 1;
            TMR2ON = 0;
            TMR2   = 0;
            rb4_state = 0;
            rb4_handled();  // debug function
        }
        else {
            // decrement counter
            counter_10--;
        }
    }
    else {
        // reset & disable
        TMR2ON = 0;
        TMR2   = 0;
        rb4_state = 0;
    }
}

/** Turn on arrow leds or set correct guess flag **/
void make_guess() {
    int special_no = 8;
    int current_no = 7;
    if (current_no < special_no) {
        // Up arrow
        LATC = 0b00000010;
        LATD = 0b00001111;
        LATE = 0b00000010;
        latcde_update_complete();   // debug function
    }
    else if (current_no > special_no) {
        // Down arrow
        LATC = 0b00000100;
        LATD = 0b00001111;
        LATE = 0b00000100;
        latcde_update_complete();   // debug function
    }
    else {
        // Correct guess
        LATC = 0;
        LATD = 0;
        LATE = 0;
        correct_guess_flag = 1;
    }
    rb4_pressed = 0;
}

void main(void) {
    rb4_init();
    while(1){
        if (press_flag == 1) {
            on_rb4_press();
            press_flag = 0;
        }
        if (debounce_flag == 1) {
            rb4_debounce();
            debounce_flag = 0;
        }
        if (rb4_pressed == 1) {
            make_guess();
        }
    }
    
    return;
}
