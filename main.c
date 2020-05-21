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
char rb4_flag;
char temp_adc_high;
char temp_adc_low;
char correct_guess_flag;
char end_game_flag;

void init_complete(void){}
void restart(void){}

void init(){
    
}

void get_adc_value(){
    
}

void check_RB4_button(){
    
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
}

void latj_update(void){ // 7 Segment Number
    switch(adc_value){
        case 0x00 : PORTJ = 11111100;
        case 0x01 : PORTJ = 01100000;
        case 0x02 : PORTJ = 11011010;
        case 0x03 : PORTJ = 11110010;
        case 0x04 : PORTJ = 01100110;
        case 0x05 : PORTJ = 10110110;
        case 0x06 : PORTJ = 10111110;
        case 0x07 : PORTJ = 11100000;
        case 0x08 : PORTJ = 11111110;
        case 0x09 : PORTJ = 11100110;
        default   : PORTJ = 11111100;
    }
    return;    
}

void get_and_conv_adc(){
    
}

void low_isr(){
    temp_adc_high = adch;
    temp_adc_low = adcl;
    timer_value loading;
    adc_flag = 1;
}

void main(void) {
    
    while(1){
    
        special = special_number();
        
        init();
        init_complete(); // breakpoint
        
        while( !end_game_flag /* some flag maybe */){ // 5 second loop

            if(adc_flag == 1){
                get_and_conv_adc();
                latj_update();
                latjh_update_complete(); // breakpoint
            }
            check_RB4_button(); // <- rb4_handled();
            if(rb4_pressed == 1){
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
