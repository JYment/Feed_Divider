//#include "HX711.h"
#include <LiquidCrystal.h>
#include <avr/wdt.h>

#define SELECT_BTN                        (PIND&0x04)


LiquidCrystal lcd(7,8,9,10,11,12);//RS,E,DB4,DB5,DB6,DB7

unsigned char menu_mode = 0;
unsigned char normal_mode = 0;
unsigned char disp_mode = 0;

unsigned char ready_to_start = 0;
unsigned int loadcell_weigh = 50;
struct t{
    unsigned int week = 1;
    unsigned int hour = 12;
    unsigned int minute = 12;
    unsigned int sec = 12;
};

struct t now;

volatile unsigned char SELECT_BTN_flag = 0, SELECT_BTN_cnt = 0;


ISR(TIMER1_COMPA_vect) {
    //*************************************************************
    //* SELECT 버튼 동작 (길게 눌렀을 때, 짧게 눌렀을 때)     *
    //*************************************************************    
    if(SELECT_BTN) {
        if(SELECT_BTN_flag == 0) {
            SELECT_BTN_flag = 1;
            SELECT_BTN_cnt = 0;
        }
        else {
            if(SELECT_BTN_cnt < 60) {
                SELECT_BTN_cnt++;
            }
        }
    }
    else {
        SELECT_BTN_flag = 0;
        if(SELECT_BTN_cnt) {
            if(SELECT_BTN_cnt >= 15) {                  // 버튼 길게 누른 경우
                disp_mode = 0;
                normal_mode = 1;                        // RUN 동작
                ready_to_start = 1;
            }
            else {
                disp_mode = 1;
                menu_mode = 0;                        // 메뉴 Display
            }
            SELECT_BTN_cnt = 0;
        }
    }

    //*************************************************************
    //* UP 버튼 동작 (짧게 눌렀을 때)
    //*************************************************************    
        
}

void setup() {
    wdt_disable();                                      // 와치독 중지
    _delay_ms(200);
        
    lcd.begin(16,2);                                    // CLCD 초기화
//    scale.set_scale();                                  // Loadcell - HX711 초기화
//    scale.tare();    
    
    TCCR1A = 0x00;                                      // CTC Mode
    TCCR1B = 0x0B;                                      // 64 prescaling        16000000 / 64 65535
    TIMSK1 = 0x02;                                      // Comapre A Match Interrupt Enable
    OCR1A = 0x30D3;                                     // 50ms TIMER SET
    
//    wdt_enable(WDTO_2S);                                // 와치독 실행 (2초)
    sei();                                              // 전체 인터럽트 ENABLE
//    mode = INIT_MODE;
//    while(1) {                                          // 전압 인가 후 모드 
//        if(mode == INIT_MODE) {
//            init_display();                             // 초기화 화면 디스플레이
//        }
//        else {
//            break;
//        }
//    }
    lcd.setCursor(0, 0);
}

void loop() {
    switch(disp_mode) {
        case 0:
        break;
        case 1:
        menu_display();
        break;
        default:
        main_display();
        break;
            
    }
    
}


// 초기화면 
void init_display() {
    wdt_reset();
    lcd.setCursor(0, 0);        lcd.print("Feed Divider");
    lcd.setCursor(0, 1);        lcd.print("Initiallizing...");
}

// main 화면
void main_display() {
    lcd.setCursor(0,0);         lcd.print("Weigh : ");
    lcd.setCursor(8,0);         lcd.print(loadcell_weigh);
    lcd.setCursor(10,0);        lcd.print("g");    
    lcd.setCursor(13,0);
    if(ready_to_start == 0)     lcd.print("R/D");
    else                        lcd.print("RUN"); 

    lcd.setCursor(2,1);
    if(now.week == 1)           lcd.print("SUN");
    else if(now.week == 2)      lcd.print("MON");
    else if(now.week == 3)      lcd.print("TUE");
    else if(now.week == 4)      lcd.print("WED");
    else if(now.week == 5)      lcd.print("THU");
    else if(now.week == 6)      lcd.print("FRI");
    else if(now.week == 7)      lcd.print("SAT");
    lcd.setCursor(6,1);         lcd.print(now.hour);
    lcd.setCursor(8,1);         lcd.print(":");
    lcd.setCursor(9,1);         lcd.print(now.minute);
    lcd.setCursor(11,1);        lcd.print(":");
    lcd.setCursor(12,1);        lcd.print(now.sec);
}

// 메뉴 화면
void menu_display() {
    if(menu_mode == 0) {
        lcd.setCursor(2,0);         lcd.print("1. Weigh Set");
        lcd.setCursor(2,1);         lcd.print("2. Timer Set");
    }
    else {
        lcd.setCursor(2,0);         lcd.print("3. Run");
        lcd.setCursor(2,1);         lcd.print("4. Back");
    }
}
