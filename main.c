#pragma config OSC = HSPLL, FCMEN = OFF, IESO = OFF, PWRT = OFF, BOREN = OFF, WDT = OFF, MCLRE = ON, LPT1OSC = OFF, LVP = OFF, XINST = OFF, DEBUG = OFF

#include <xc.h>
#include "breakpoints.h"


volatile int adc_value;
volatile char special;
bit adc_flag;
bit rb4_flag;
bit correct_guess_flag;
bit end_game_flag;
bit blinkShow;
char temp_adc_high;
char temp_adc_low;
int tmr1Counter;

void init(){
    tmr1Counter = 0; // initialize tmr1Counter to zero. 
    blinkShow = 0;   // initially set hide on blink.
    INTCON.GIE=0;   // All interrupts are disabled.


    ADRESH = 0;
    ADRESL = 0;
    
    ADCON0 = 0x31;  // channel 12 (AN12) : enable ADON
    ADCON1 = 0;     // all pins analog : default Vrefs
    ADCON2 = 0x82;  // ADFM set : Tacq = 0.Tad : Fosc/32
    
    ADIE = 0;       // ADC interrupt disable : timer0 will check that
    TMR0IF = 0;     // disable for now
    GODONE = 1;     // start conversion
    adc_flag = 0;   // flag not set initially
    
    TRISH = 0;      // 
    RH0 = 1;        // enable digit 0
    
    TMR0ON = 1;     // start timer0


    /******** TIMER1 SETUP *******/
    TMR1IF = 1;
    PIE1.TMR1IE = 1;     // Timer1 interrupt is enabled.
    IPR1.TMR1IP = 1;     // Set Timer1 overflow interrupt priority to high.
    T1CON |= 0b10110000; // Set Timer1 to one 16-bit operation mode and prescaler to 1:8.
    TMR1L = 0x0BDB;      // Preload Timer1 to 3035.
    T1CON.TMR1ON = 1;    // Timer1 is started.


    INTCON.GIE=1;   // All interrupts are enabled.
    return;
}

void __interrupt(high_priority) high_isr() {
    if(TMR1IF == 1 && !end_game_flag) {
        tmr1Counter++;
        if(tmr1Counter == 400) {
            tmr1Counter = 0;    // Reset counter.
            end_game_flag = 1;  // Game is over.
        }
    }
    else if(TMR1IF == 1 && end_game_flag) {
        tmr1Counter++;
        if(tmr1Counter == 16) {
            tmr1Counter = 0;    // Reset counter.
            // TODO: Reset Game
        }
        else if(tmr1Counter % 4 == 0) {
            if (!blinkShow) {
                blinkShow = 1; 
                // TODO: Show the special number on 7-Segment display.
            }
            else {
                blinkShow = 0;
                // TODO: Hide the special number on 7-Segment display.
            }
        }
    }
    PIR1.TMR1IF = 0;     // Clear Timer1 register overflow bit.
    TMR1L = 0x0BDB;      // Preload Timer1  to 3035.
    return;
}

void __interrupt(low_priority) low_isr(){
    if(TMR0IF == 1){
        adc_flag      = 1;      // do adc_task in main
        temp_adc_high = ADRESH; // get high bits
        temp_adc_low  = ADRESL; // get low bits
        GODONE        = 1       // start the conversion again
        TMR0IF        = 0;      // clear timer0 interrupt flag
    }
    return;
}

void adc_task(){ // get ADRESH & ADRESL : 
    
    adc_flag = 0; // clear flag
    adc_value = temp_adc_high;
    adc_value << 8;
    adc_value += temp_adc_low;
    
    if(0 <= adc_value || adc_value <= 102){ adc_value = 0; }
    else if(adc_value <= 204) { adc_value = 1; }
    else if(adc_value <= 306) { adc_value = 2; }
    else if(adc_value <= 408) { adc_value = 3; }
    else if(adc_value <= 510) { adc_value = 4; }
    else if(adc_value <= 612) { adc_value = 5; }
    else if(adc_value <= 714) { adc_value = 6; }
    else if(adc_value <= 816) { adc_value = 7; }
    else if(adc_value <= 918) { adc_value = 8; }
    else if(adc_value <= 1023){ adc_value = 9; }
    
    return;
}



void check_RB4_button(){
    
    return;
}

char game_check(){
    
    return 0;
}

void blink_2_sec(){
    /* initially turned on
     * wait 500 ms then H=0, turn off
     * wait 500 ms then H=1, turn on
     * wait 500 ms then H=0, turn off
     * wait 500 ms then H=1, turn on -> tartisalim
     * */
    return;
}

void latj_update(void){ // 7 Segment Number
    switch(adc_value){
        case 0  : LATJ = 11111100;
        case 1  : LATJ = 01100000;
        case 2  : LATJ = 11011010;
        case 3  : LATJ = 11110010;
        case 4  : LATJ = 01100110;
        case 5  : LATJ = 10110110;
        case 6  : LATJ = 10111110;
        case 7  : LATJ = 11100000;
        case 8  : LATJ = 11111110;
        case 9  : LATJ = 11100110;
        default : LATJ = 11111100;
    }
    return;    
}


void main(void) {
    
    while(1){
    
        special = special_number();
        
        init();
        init_complete(); // breakpoint
        
        while( !end_game_flag ){ // 5 second loop

            if(adc_flag == 1){
                adc_task();
                adc_complete(); // breakpoint
                latj_update();
                latjh_update_complete(); // breakpoint
            }
            check_RB4_button();
            if(rb4_flag == 1){
                // other things
                correct_guess_flag == 1;
                rb4_handled();
            }
        }
        // timer1 500 loading
        if(correct_guess_flag == 0){ // initailly 0
            game_over(); // breakpoint
            latjh_update_complete(); // breakpoint
        }
        blink_2_sec();
        restart(); // breakpoint
    }
    
    return;
}
