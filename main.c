#pragma config OSC = HSPLL, FCMEN = OFF, IESO = OFF, PWRT = OFF, BOREN = OFF, WDT = OFF, MCLRE = ON, LPT1OSC = OFF, LVP = OFF, XINST = OFF, DEBUG = OFF

#include <xc.h>
#include "breakpoints.h"


volatile int adc_value;
volatile char special;
char adc_flag;
char rb4_flag;
char end_game_flag;
char blinkShow;
char temp_adc_high;
char temp_adc_low;
char waitBlink;
char waitForBreakPoints;
int tmr1Counter;
int timer0_postscaler; // software implemented postscaler for timer0 : 50ms

int counter_10;     // 1ms interrupt counter, for 10ms debounce duration
char rb4_state;      // 0: Waiting for an input, 1: On debounce
char rb4_pressed;    // Will be set when rb4 press is validated after 10ms
char press_flag;     // Rb4 press in state 0 sets this flag
char debounce_flag;  /* Rb4 change in state 1 or 
                         Timer2 interrupt in state 1 sets this flag */
char correct_guess_flag;

void init(){
    tmr1Counter = 0; // initialize tmr1Counter to zero. 
    blinkShow = 0;   // initially set hide on blink.
    waitBlink = 1;   // initially set to wait blink.
    waitForBreakPoints = 1; // initally set to wait breakpoints.

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
    T1CON |= 0b10010000; // Set Timer1 to one 16-bit operation mode and prescaler to 1:2.
    TMR1H = 0x0B;
    TMR1L = 0xDB;      // Preload Timer1  to 3035.
    

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

    adc_value = 0;
    
    // start interrupts and timers
    RCONbits.IPEN = 1;       // enable low priority interrupts
    INTCONbits.GIEH = 1;     // high priorities enabled
    INTCONbits.GIEL = 1;     // low priorities enabled
    T0CONbits.TMR0ON = 1;    // start timer0
    T1CONbits.TMR1ON = 1;    // Timer1 is started.
    
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
    PORTHbits.RH0 = 0;
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
    else if(TMR1IF == 1 && end_game_flag && !waitForBreakPoints) { 
        tmr1Counter++;
        if(tmr1Counter == 160) {
            hs_passed();
            tmr1Counter = 0;    // Reset counter.
            waitBlink = 0;      // blink is over, continue to execute.
        }
        else if(tmr1Counter % 40 == 0) {
            hs_passed(); // breakpoint : should be called per 500ms using TIMER1
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
    TMR1H = 0x0B;
    TMR1L = 0xDB;      // Preload Timer1  to 3035.
    return;
}

void __interrupt(low_priority) low_isr(){
    if(INTCONbits.T0IF == 1 && !end_game_flag){
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
    return;
}

void adc_task(){ // get ADRESH & ADRESL values
    
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



void latj_update(void){ // 7 Segment Number Selection
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
    int special_no = 5;//special_number(); // give input for debug
    int current_no = adc_value;
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
        correct_guess();
        latcde_update_complete();
        end_game_flag = 1;
    }
    rb4_pressed = 0;
}

void main(void) {
    while(1){
        init();
        init_complete(); // breakpoint
        
        while( !end_game_flag ){ // 5 second loop

            if(adc_flag == 1){
                adc_task();
                adc_complete(); // breakpoint
                latj_update();
                latjh_update_complete(); // breakpoint
            }
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
        INTCONbits.GIEL = 0;
        
        if (correct_guess_flag == 0) {
            game_over();    // breakpoint
        }
        waitForBreakPoints = 0; // start timer1 for blink 2 sec.
        tmr1Counter = 0; // Reset Timer1 counter in case of game is ended earlier than 5 sec
                         // So tmr1Counter remained more than zero.
        while(waitBlink) {
            // Wait 2 seconds after game finished.
        }
        restart();  // breakpoint
    }
    return;
}
