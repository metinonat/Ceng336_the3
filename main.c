#pragma config OSC = HSPLL, FCMEN = OFF, IESO = OFF, PWRT = OFF, BOREN = OFF, WDT = OFF, MCLRE = ON, LPT1OSC = OFF, LVP = OFF, XINST = OFF, DEBUG = OFF

#include <xc.h>
#include "breakpoints.h"


volatile int adc_value;
volatile char special;
char adc_flag;
char rb4_flag;
char correct_guess_flag;
char end_game_flag;
char blinkShow;
char temp_adc_high;
char temp_adc_low;
int tmr1Counter;
int timer0_postscaler; // software implemented postscaler for timer0 : 50ms

void init(){
    tmr1Counter = 0; // initialize tmr1Counter to zero. 
    blinkShow = 0;   // initially set hide on blink.

    // flag inits ----
    end_game_flag = 0;
    adc_flag = 0;
    
    // ADC inits
    ADRESH = 0;
    ADRESL = 0;
    
    ADCON0 = 0x31;  // channel 12 (AN12) : enable ADON
    ADCON1 = 0x00;  // all pins analog : default Vrefs
    ADCON2 = 0x82;  // ADFM set : Tacq = 0.Tad : Fosc/32
    
    PIE1bits.ADIE = 0;       // ADC interrupt disable : timer0 will check that
    INTCONbits.GIEL = 1;
    ADCON0bits.GO = 1;       // start the conversion
    
    // 7 segment display
    TRISHbits.RH0 = 0;
    TRISHbits.RH4 = 1;
    TRISJ = 0;
    PORTHbits.RH0 = 1;       // enable digit 0
    LATJ = 0b11111100;       // set display as 0 initially
    
    // timer0
    T0CON = 0b01000101;      // 8 bit : Prescale = 1/64
    TMR0 = 131;              // for 50ms
    INTCON2bits.TMR0IP = 0;  // Timer0 low priority
    INTCONbits.T0IF = 0;     // disable for now
    INTCONbits.TMR0IE = 1;   // enable timer0 interrupts
    timer0_postscaler = 0;   // create an interrupt on counter == 25
    


    /******** TIMER1 SETUP *******/
    TMR1IF = 1;
    PIE1bits.TMR1IE = 1;     // Timer1 interrupt is enabled.
    IPR1bits.TMR1IP = 1;     // Set Timer1 overflow interrupt priority to high.
    T1CON |= 0b10110000; // Set Timer1 to one 16-bit operation mode and prescaler to 1:8.
    TMR1L = 0x0BDB;      // Preload Timer1 to 3035.
    T1CONbits.TMR1ON = 1;    // Timer1 is started.



    // start interrupts and timers
    RCONbits.IPEN = 1;       // enable low priority interrupts
    INTCONbits.GIEH = 1;     // high priorities enabled
    INTCONbits.GIEL = 1;     // low priorities enabled
    T0CONbits.TMR0ON = 1;    // start timer0
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
        if(tmr1Counter == 160) {
            tmr1Counter = 0;    // Reset counter.
            init(); // Check ????
        }
        else if(tmr1Counter % 40 == 0) {
            if (!blinkShow) {
                blinkShow = 1; 
                showSpecialNumber();
            }
            else {
                blinkShow = 0;
                hideSpecialNumber();
            }
        }
    }
    PIR1bits.TMR1IF = 0;     // Clear Timer1 register overflow bit.
    TMR1L = 0x0BDB;      // Preload Timer1  to 3035.
    return;
}

void __interrupt(low_priority) low_isr(){
    if(INTCONbits.T0IF == 1){
        if(timer0_postscaler == 25){
            INTCONbits.T0IF   = 0;  // clear timer0 interrupt flag
            adc_flag = 1;           // do adc_task in main
            temp_adc_high = ADRESH; // get high bits
            temp_adc_low  = ADRESL; // get low bits
            timer0_postscaler = 0;  // reset
            ADCON0bits.ADON = 1;
            ADCON0bits.GO = 1;  // start the conversion again
        }
        else{
            INTCONbits.T0IF   = 0;  // clear timer0 interrupt flag
            TMR0 = 131;             // reset timer0 load
            timer0_postscaler++;
        }
    }
    return;
}

void adc_task(){ // get ADRESH & ADRESL : 
    
    adc_flag = 0; // clear flag
    adc_value = (temp_adc_high << 8) | temp_adc_low; // adc_value = temp_adc_high:temp_adc_low
    
    if(0 <= adc_value && adc_value <= 102){ adc_value = 0; }
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



void showSpecialNumber(){
    
    switch(special){
        case 0  : LATJ = 0b11111100; break;
        case 1  : LATJ = 0b01100000; break;
        case 2  : LATJ = 0b11011010; break;
        case 3  : LATJ = 0b11110010; break;
        case 4  : LATJ = 0b01100110; break;
        case 5  : LATJ = 0b10110110; break;
        case 6  : LATJ = 0b10111110; break;
        case 7  : LATJ = 0b11100000; break;
        case 8  : LATJ = 0b11111110; break;
        case 9  : LATJ = 0b11100110; break;
        default : LATJ = 0b11111100; break;
    }
    return;
}
void hideSpecialNumber(){
    LATJ = 0;
    return;
}

void latj_update(void){ // 7 Segment Number
    switch(adc_value){
        case 0  : LATJ = 0b11111100; break;
        case 1  : LATJ = 0b01100000; break;
        case 2  : LATJ = 0b11011010; break;
        case 3  : LATJ = 0b11110010; break;
        case 4  : LATJ = 0b01100110; break;
        case 5  : LATJ = 0b10110110; break;
        case 6  : LATJ = 0b10111110; break;
        case 7  : LATJ = 0b11100000; break;
        case 8  : LATJ = 0b11111110; break;
        case 9  : LATJ = 0b11100110; break;
        default : LATJ = 0b11111100; break;
    }
    return;    
}


void main(void) {
    
    while(1){
    
        special = 5;//special_number();
        
        init();
        init_complete(); // breakpoint
        
        while( !end_game_flag ){ // 5 second loop

            if(adc_flag == 1){
                adc_task();
                adc_complete(); // breakpoint
                latj_update();
                latjh_update_complete(); // breakpoint
            }
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
       // blink_2_sec();
        restart(); // breakpoint
    }
    
    return;
}
