#include <SoftwareSerial.h>
#include <ServoTimer2.h>  
#include <DS1302.h>
#include <EEPROM.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "HX711.h"

#define HX_DOUT             A4
#define HX_CLK              A5

#define TFT_DC 9
#define TFT_CS 10

#define UP_BTN              (PINC&0x01)
#define DOWN_BTN            (PINC&0x02)
#define SELECT_BTN          (PINC&0x04)

#define OP_MODE_READY          0
#define OP_MODE_RUN            1
//#define MODE_MENU           2
#define SELECT_MODE_DISPLAY     0
#define SELECT_MODE_MENU        1

#define BTN_MAIN_MENU       0
#define BTN_TIME_MODE       1
#define BTN_WEIGHT_MODE     2
#define BTN_SAVE            3
#define BTN_BACK            4

ServoTimer2 opServo;
//SoftwareSerial AX_Serial(5, 6); 
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
DS1302 rtc(2, 3, 4);
HX711 scale(HX_DOUT, HX_CLK);                                       // Loadcell HX711 모듈 초기화            


Time now_Time;
volatile int cnt = 0;
volatile unsigned char OP_mode = OP_MODE_READY;                     // 메인 동작 디스플레이 변수
volatile unsigned char SELECT_BTN_flag = 0, SELECT_BTN_cnt = 0, SELECT_BTN_mode = SELECT_MODE_DISPLAY;      // 선택 버튼 디스플레이 혹은 메뉴
volatile unsigned char UP_BTN_flag = 0, UP_BTN_cnt = 0, UP_BTN_mode = 0;
volatile unsigned char DOWN_BTN_flag = 0, DOWN_BTN_cnt = 0;
volatile unsigned char BTN_mode = 0;

volatile unsigned char set_time_mode = 0, set_time_cnt = 0;
volatile char set_hour = 0, set_min = 0;
unsigned int con_hour[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned int con_min[8] = {0, 0, 0, 0, 0, 0, 0, 0};
unsigned int op_hour = 0, op_min = 0;
unsigned char con_weigh = 0;
unsigned char open_flag = 0;


long sum = 0;
int avg = 0, before_avg = 0;
char hour_flag, min_flag, week_flag;
char before_week_flag;

float calibration_factor = -10000;                                  // Loadcell calibration factor value
int readIndex = 0;
float total = 0.0;
const int cycles = 10;
float weight_average = 0.0;
float readings[cycles];
int temp_i;
int addr = 0;


ISR(TIMER1_COMPA_vect)              // 50ms TIMER1
{
    cnt++;
    if(cnt == 20)
//    if(cnt == 250)
    {
        cnt = 0;
        if(SELECT_BTN_mode == SELECT_MODE_DISPLAY)
        {
            tft.fillRect(250, 165, 55, 30, ILI9341_BLACK);
            time_display();
        }
    }

    // ******************************************************************** //
	// ************************ SELECT BTN PROCESS ************************ //
    if(SELECT_BTN)
    {
        if(SELECT_BTN_flag == 0)
        {
            SELECT_BTN_flag = 1;
            SELECT_BTN_cnt = 0;
        }
        else
        {
            if(SELECT_BTN_cnt < 60)     SELECT_BTN_cnt++;
        }
    }
    else
    {
        SELECT_BTN_flag = 0;
        if(SELECT_BTN_cnt)
        {
            if(SELECT_BTN_cnt >= 15)								// 길게 눌렀을 때
            {
                //Serial.println("SELECT BTN long Clicked");  Serial.print(SELECT_BTN_mode);    Serial.println(BTN_mode);
                if(SELECT_BTN_mode == SELECT_MODE_DISPLAY)
                {
                    if(OP_mode == OP_MODE_READY)        OP_mode = OP_MODE_RUN;
                    else                                OP_mode = OP_MODE_READY;
                    main_display();                
                }
                else
                {
                    if(BTN_mode == BTN_TIME_MODE || BTN_mode == BTN_WEIGHT_MODE)
                    {
                        menu_Func();
                    }
                }
            }
            else													// 짧게 눌렀을 때
            {
                //Serial.println("SELECT BTN short Clicked!");
                if(SELECT_BTN_mode == SELECT_MODE_DISPLAY)
                {
					//Serial.print("SELECT_BTN_mode:");  Serial.println("MAIN Display!!!!");  					
                    menu_Func();
                }
                else if(SELECT_BTN_mode == SELECT_MODE_MENU)
                {
					//Serial.print("SELECT_BTN_mode:");  Serial.println("MENU Display!");
                    if(BTN_mode == BTN_MAIN_MENU)
                    {
                        if(UP_BTN_mode == 0)					    // TIME mode
                        {
                            set_hour = 0;
                            set_time_cnt = 0;
                            //Serial.println("SELECT:TIME MODE");
                            tft.fillScreen(ILI9341_BLACK);          // ERASE
                            menu_frame_display();                   // MENU FRAME DISPLAY
							timer_frame_display();					// TIME FRAME DISPLAY
                            timer_set_display();                    // TIME SET DISPLAY
                            BTN_mode = BTN_TIME_MODE;
                        }
                        else if(UP_BTN_mode == 1)					// WEIGH mode
                        {
                            //Serial.println("SELECT:WEIGH MODE");
                            tft.fillScreen(ILI9341_BLACK);			// ERASE
                            menu_frame_display();					// MENU FRAME DISPLAY
                            weigh_frame_display();					// WEIGH FRAME DISPLAY
							weigh_set_display();					// WEIGH SET DISPLAY
                            BTN_mode = BTN_WEIGHT_MODE;
                        }
                        else if(UP_BTN_mode == 2)					// SAVE mode
                        {
                            //Serial.println("SELECT:SAVE MODE");
							for(int i=0; i<8; i++)					// HOUR, MIN SAVE
							{
								EEPROM.write(addr+i, con_hour[i]);
								EEPROM.write(addr+10+i, con_min[i]);
							}
							EEPROM.write(20, con_weigh);			// WEIGH SAVE
                        }
                        else if(UP_BTN_mode == 3)					// BACK mode
                        {
                            //Serial.println("SELECT:BACK MODE");
                            UP_BTN_mode = 0;
                            tft.fillScreen(ILI9341_BLACK);
                            SELECT_BTN_mode = SELECT_MODE_DISPLAY;
                            main_display();
                            weigh_display();
                            time_display();
                            BTN_mode = BTN_MAIN_MENU;				// BTN_mode init : MAIN MENU
                        }
                    }
					else if(BTN_mode == BTN_TIME_MODE)
					{
						set_time_cnt++;
						if(set_time_cnt == 2)
						{
							set_hour++;
							set_min++;
							set_time_cnt = 0;
						}
						timer_set_display();						// TIME SET DISPLAY
					}
					else if(BTN_mode == BTN_WEIGHT_MODE)
					{
						// 메뉴 중 무게 설정 모드
					}
                }
            }
            SELECT_BTN_cnt = 0;
        }
    }

    // ******************************************************************** //
    // ************************** UP BTN PROCESS ************************** //
    if(UP_BTN)
    {
        if(UP_BTN_flag == 0)
        {
            UP_BTN_flag = 1;
            UP_BTN_cnt = 0;
        }
        else
        {
            if(UP_BTN_cnt < 60)         UP_BTN_cnt++;
        }
    }
    else
    {
        UP_BTN_flag = 0;
        if(UP_BTN_cnt)
        {
            //Serial.print("UP BTN Clicked:");      Serial.print(SELECT_BTN_mode);    Serial.println(BTN_mode);
            //Serial.print("UP BTN Clicked:");      Serial.print(SELECT_BTN_mode);    Serial.print(set_time_cnt);  Serial.println(con_min[set_hour]);            
            if(SELECT_BTN_mode == SELECT_MODE_MENU)
            {
                if(BTN_mode == BTN_MAIN_MENU)
                {
                    if(UP_BTN_mode++ == 3)  UP_BTN_mode = 0;
                    menu_select_display(UP_BTN_mode);
                }
                else if(BTN_mode == BTN_TIME_MODE)
                {                   
                    if(set_time_cnt == 0)				con_hour[set_hour]++;
                    else								con_min[set_min]++;

					if(con_hour[set_hour] > 23)			con_hour[set_hour] = 23;
					if(con_min[set_min] > 59)			con_min[set_min] = 59;
					
                    tft.fillRect(70, 55, 150, 160, ILI9341_BLACK);
                    timer_set_display();
                }
                else if(BTN_mode == BTN_WEIGHT_MODE)
                {
                    con_weigh++;
                    if(con_weigh > 100)     con_weigh = 100;
					tft.fillRect(70, 55, 180, 160, ILI9341_BLACK);
					weigh_set_display();
                }
            }
            UP_BTN_cnt = 0;
        }
    }

    // ******************************************************************** //
    // ************************* DOWN BTN PROCESS ************************* //
    if(DOWN_BTN)
    {
        if(DOWN_BTN_flag == 0)
        {
            DOWN_BTN_flag = 1;
            DOWN_BTN_cnt = 0;
        }
        else
        {
            if(DOWN_BTN_cnt < 60)         DOWN_BTN_cnt++;
        }
    }
    else
    {
        DOWN_BTN_flag = 0;
        if(DOWN_BTN_cnt)
        {
            //Serial.print("DOWN BTN Clicked:");      Serial.print(SELECT_BTN_mode);    Serial.print(set_time_cnt);  Serial.println(con_min[0]);
            DOWN_BTN_cnt = 0;
            if(SELECT_BTN_mode == SELECT_MODE_MENU)
            {
                if(BTN_mode == BTN_MAIN_MENU)
                {
                    
                }
                else if(BTN_mode == BTN_TIME_MODE)
                {
					if(set_time_cnt == 0)				con_hour[set_hour]--;
                    else                                con_min[set_min]--;
					
					if(con_hour[set_hour] < 0 || con_hour[set_hour] > 25)			con_hour[set_hour] = 0;
					if(con_min[set_min] < 0 || con_min[set_min] > 59)			    con_min[set_min] = 0;
					
                    tft.fillRect(70, 55, 150, 160, ILI9341_BLACK);
					timer_set_display();
                }
                else if(BTN_mode == BTN_WEIGHT_MODE)
                {
                    con_weigh--;
                    if(con_weigh < 0)     con_weigh = 0;
                    tft.fillRect(70, 55, 180, 160, ILI9341_BLACK);
                    weigh_set_display();
                }
            }
        }
    }    
}


void menu_Func(void)
{
    SELECT_BTN_mode = SELECT_MODE_MENU;
    menu_frame_display();                   // mainmenu_display
    mainmenu_display();                     // mainmenu_display
    menu_select_display(UP_BTN_mode);       // mainmenu_display -> CHECK 1 WHITE
    BTN_mode = BTN_MAIN_MENU;               // BTN_mode = MAIN_MENU
}


void eeprom_read_data(void)
{
    for(int i=0; i<8; i++)
    {
        con_hour[i] = EEPROM.read(addr+i);
        con_min[i] = EEPROM.read(addr+10+i);
    }
    con_weigh = EEPROM.read(20);    
}


void setup() 
{   
	eeprom_read_data();             // eeprom open
    
    rtc.halt(false);
    rtc.writeProtect(false);

//    rtc.setDOW(SATURDAY);         // Set Day-of-Week to FRIDAY
//    rtc.setTime(23, 45, 00);      // Set the time to 12:00:00 (24hr format)
//    rtc.setDate(7, 9, 2019);      // Set the date to August 6th, 2010
   
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(ILI9341_BLACK);
    
    SELECT_BTN_mode = SELECT_MODE_DISPLAY;
    OP_mode = OP_MODE_READY;
    
    main_display();
    weigh_display();
    time_display();

    Serial.begin(9600);
////    AX_Serial.begin(9600);    
////    AX_Serial.begin(1000000);

//    opServo.attach(6);

    TCCR1A = 0x00;                  // CTC Mode
    TCCR1B = 0x0B;                  // 64 prescaling        16000000 / 64 65535
    TIMSK1 = 0x02;                  // Comapre A Match Interrupt Enable
    OCR1A = 0x30D3;                 // 50ms TIMER SET
    sei();

    scale.tare();                   // Weigh initiallize
    
//    Packet_Baudrate();
    Serial_Packet(0xFE, 200);
    delay(300);
    Serial_Packet(0xFE, 500);
    delay(300);
//    pinMode(12, OUTPUT);
//    opServo.write(700);
//    delay(1000);
//    opServo.write(1800);
//    delay(1000);
}


void loop() 
{
    if(SELECT_BTN_mode == SELECT_MODE_DISPLAY)
    {
        weigh_measure();                // weigh measurement
        time_display_init();            // time display init
    }
    if(OP_mode == OP_MODE_RUN)
    {
        for(int i=0; i<8; i++)
        {
            if((con_hour[i] == op_hour) && (con_min[i] == op_min))                  // 지정된 시간이 되었을 때
            {
                temp_i = i;
                if(temp_i == i)                                                     // i == temp_i 일때 
                {
                    if(weight_average >= con_weigh)                                 // 사료가 충분 - 오픈안함
                    {
                        open_flag = 1;
                        Serial_Packet(0xFE, 500);
                    }
                    else                                                            // 사료가 충분하지 않을때 오픈
                    {
                        if(open_flag == 0)
                        {
                            Serial_Packet(0xFE, 200);
                        }
                        else
                        {
                            Serial_Packet(0xFE, 500);
                        }
                    }
                }
            }
            if((con_hour[temp_i] != op_hour) || (con_min[temp_i] != op_min))
            {
                open_flag = 0;
            }
        }
    }    
}


void main_display() 
{
    tft.drawRect(0, 0, 320, 240, ILI9341_WHITE);
    tft.drawRect(1, 1, 318, 238, ILI9341_WHITE);
    tft.drawRect(2, 2, 316, 236, ILI9341_WHITE);
    tft.fillRect(220, 15, 96, 40, ILI9341_BLACK);

    if(OP_mode == OP_MODE_READY)
    {
        tft.drawRect(220, 15, 316, 40, ILI9341_YELLOW);
        tft.setCursor(227, 25);
        tft.setTextColor(ILI9341_YELLOW);
        tft.setTextSize(3);
        tft.print("READY");
    }
    else if(OP_mode == OP_MODE_RUN)
    {
        tft.drawRect(220, 15, 316, 40, ILI9341_GREEN);
        tft.setCursor(245, 25);
        tft.setTextColor(ILI9341_GREEN);
        tft.setTextSize(3);
        tft.print("RUN");
    }
}


void weigh_display()
{
    tft.fillRect(40, 60, 150, 50, ILI9341_BLACK);
    tft.setCursor(40, 30);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("WEIGHT");
    tft.setCursor(40, 60);    
    tft.setTextColor(ILI9341_GREEN);
    tft.setTextSize(6);
    tft.print((int)(weight_average));
    tft.print("g");
}


void time_display()
{
    tft.setCursor(110, 135);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("NOW TIME");
    tft.setCursor(20, 165);
    tft.setTextColor(ILI9341_GREEN);
    tft.setTextSize(4);
    if(rtc.getDOWStr() == "Monday") 
    {
        week_flag = 1;
        tft.print("MON");
    }
    else if(rtc.getDOWStr() == "Tuesday")
    {
        week_flag = 2;
        tft.print("TUE");
    }
    else if(rtc.getDOWStr() == "Wednesday")
    {
        week_flag = 3;
        tft.print("WED");
    }
    else if(rtc.getDOWStr() == "Thursday")
    {
        week_flag = 4;
        tft.print("THU");
    }
    else if(rtc.getDOWStr() == "Friday") 
    {
        week_flag = 5;
        tft.print("FRI");
    }
    else if(rtc.getDOWStr() == "Saturday")
    {
        week_flag = 6;
        tft.print("SAT");
    }
    else if(rtc.getDOWStr() == "Sunday") 
    {
        week_flag = 7;
        tft.print("SUN");
    }
    
    tft.print(" ");
    tft.print(getTime());
}

char *getTime()
{
    char *output= "xxxxxxxx";
    now_Time = rtc.getTime();

    op_hour = now_Time.hour;
    op_min = now_Time.min;
    
    if(now_Time.min == 0 && hour_flag == 0)     hour_flag = 1;
    if(now_Time.sec == 0 && min_flag == 0)      min_flag = 1;

    if (now_Time.hour<10)       output[0]=48;
    else                        output[0]=char((now_Time.hour / 10)+48);
    output[1]=char((now_Time.hour % 10)+48);
    output[2]=58;
    if (now_Time.min<10)        output[3]=48;
    else                        output[3]=char((now_Time.min / 10)+48);
    output[4]=char((now_Time.min % 10)+48);
    output[5]=58;

    if (now_Time.sec<10)
        output[6]=48;
    else
        output[6]=char((now_Time.sec / 10)+48);
    output[7]=char((now_Time.sec % 10)+48);
    output[8]=0;

    return output;
}

void weigh_measure()
{
    loadcell_measure();
    if(before_avg > weight_average+3 || before_avg < weight_average-3)
    {
        before_avg = weight_average;
        if(weight_average < 5 || weight_average > 110)
        {
            weight_average = 0;
        }
        weigh_display();
        delay(10);
    }
}

void time_display_init()
{
    if(hour_flag == 1)
    {
        hour_flag = 2;
        tft.fillRect(110, 165, 55, 30, ILI9341_BLACK);      // hour clear
        time_display();
    }
    else if(hour_flag == 2)
    {
       if(now_Time.min != 0)        hour_flag = 0; 
    }

    if(min_flag == 1)
    {
        min_flag = 2;
        tft.fillRect(180, 165, 55, 30, ILI9341_BLACK);      // min clear
        time_display();
    }
    else if(min_flag == 2)
    {
        if(now_Time.sec != 0)       min_flag = 0; 
    }

    if(before_week_flag != week_flag)
    {
        tft.fillRect(20, 165, 75, 30, ILI9341_BLACK);       // weak
        before_week_flag = week_flag;
        time_display();
    }
    tft.fillRect(3, 195, 100, 40, ILI9341_BLACK);           // clear
}

void menu_frame_display()
{
    tft.fillScreen(ILI9341_BLACK);
    tft.drawRect(0, 0, 320, 240, ILI9341_WHITE);
    tft.drawRect(1, 1, 318, 238, ILI9341_WHITE);
    tft.drawRect(2, 2, 316, 236, ILI9341_WHITE);
}

void mainmenu_display()
{
    tft.drawRect(30, 20, 260, 200, ILI9341_WHITE);
    tft.setCursor(90, 40);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(3);
    tft.print("> MENU <");
    
    menu_select_display(0);
}

void menu_select_display(char select_num)
{
    if(select_num == 0)         tft.setTextColor(ILI9341_WHITE);
    else                        tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(70, 90);
    tft.setTextSize(2);
    tft.print("> 1. SET TIME");
    tft.setCursor(70, 120);
    if(select_num == 1)         tft.setTextColor(ILI9341_WHITE);
    else                        tft.setTextColor(ILI9341_YELLOW);
    tft.setTextSize(2);
    tft.print("> 2. SET WEIGHT");
    tft.setCursor(70, 150);
    if(select_num == 2)         tft.setTextColor(ILI9341_WHITE);
    else                        tft.setTextColor(ILI9341_YELLOW);
    tft.setTextSize(2);
    tft.print("> 3. SAVE");    
    tft.setCursor(70, 180);
    if(select_num == 3)         tft.setTextColor(ILI9341_WHITE);
    else                        tft.setTextColor(ILI9341_YELLOW);
    tft.setTextSize(2);
    tft.print("> 4. BACK");   
}


void timer_frame_display()
{
    tft.drawRect(30, 20, 260, 200, ILI9341_WHITE);
    tft.setCursor(85, 30);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("> TIME SET <");	
}


void timer_set_display()
{
    for(int i=0; i<8; i++)
    {
        if(con_hour[i] > 23)    con_hour[i] = 0;
        if(con_min[i] > 59)     con_min[i] = 0;
    }


    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(90, 55);
    tft.setTextSize(2);
	tft.print("> 1. ");
	if(set_hour == 0)
	{
		if(set_time_cnt == 0)			tft.setTextColor(ILI9341_WHITE);
	}
    else
    {
        tft.setTextColor(ILI9341_YELLOW);
    }
	tft.print(con_hour[0]);	
	tft.setTextColor(ILI9341_YELLOW);	tft.print(":");	
	if(set_hour == 0)
	{
        if(set_time_cnt == 1)			tft.setTextColor(ILI9341_WHITE);
	}	
    else
    {
        tft.setTextColor(ILI9341_YELLOW);
    }
	tft.print(con_min[0]);

    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(90, 75);
    tft.setTextSize(2);
	tft.print("> 2. ");
	if(set_hour == 1)
	{
		if(set_time_cnt == 0)			tft.setTextColor(ILI9341_WHITE);
	}
	else
	{
		tft.setTextColor(ILI9341_YELLOW);
	}
	tft.print(con_hour[1]);	
	tft.setTextColor(ILI9341_YELLOW);	tft.print(":");
	if(set_hour == 1)
	{
		if(set_time_cnt == 1)			tft.setTextColor(ILI9341_WHITE);
	}
	else
	{
		tft.setTextColor(ILI9341_YELLOW);
	}
	tft.print(con_min[1]);
	
	tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(90, 95);
    tft.setTextSize(2);
	tft.print("> 3. ");
	if(set_hour == 2)
	{
		if(set_time_cnt == 0)			tft.setTextColor(ILI9341_WHITE);
	}
	else
	{
		tft.setTextColor(ILI9341_YELLOW);
	}
	tft.print(con_hour[2]);	
	tft.setTextColor(ILI9341_YELLOW);	tft.print(":");	
	if(set_hour == 2)
	{
		if(set_time_cnt == 1)			tft.setTextColor(ILI9341_WHITE);
	}
	else
	{
		tft.setTextColor(ILI9341_YELLOW);
	}
	tft.print(con_min[2]);
		
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(90, 115);
    tft.setTextSize(2);
	tft.print("> 4. ");	
	if(set_hour == 3)
	{
		if(set_time_cnt == 0)			tft.setTextColor(ILI9341_WHITE);
	}
	else
	{
		tft.setTextColor(ILI9341_YELLOW);
	}
	tft.print(con_hour[3]);	
	tft.setTextColor(ILI9341_YELLOW);	tft.print(":");	
	if(set_hour == 3)
	{
		if(set_time_cnt == 1)			tft.setTextColor(ILI9341_WHITE);
	}
	else
	{
		tft.setTextColor(ILI9341_YELLOW);
	}	
	tft.print(con_min[3]);	

    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(90, 135);
    tft.setTextSize(2);
	tft.print("> 5. ");	
	if(set_hour == 4)
	{
		if(set_time_cnt == 0)			tft.setTextColor(ILI9341_WHITE);
	}
	else
	{
		tft.setTextColor(ILI9341_YELLOW);
	}
	tft.print(con_hour[4]);	
	tft.setTextColor(ILI9341_YELLOW);	tft.print(":");	
	if(set_hour == 4)
	{
		if(set_time_cnt == 1)			tft.setTextColor(ILI9341_WHITE);
	}
	else
	{
		tft.setTextColor(ILI9341_YELLOW);
	}
	tft.print(con_min[4]);
	
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(90, 155);
    tft.setTextSize(2);
	tft.print("> 6. ");	
	if(set_hour == 5)
	{
		if(set_time_cnt == 0)			tft.setTextColor(ILI9341_WHITE);
	}
	else
	{
		tft.setTextColor(ILI9341_YELLOW);
	}
	tft.print(con_hour[5]);	
	tft.setTextColor(ILI9341_YELLOW);	tft.print(":");	
	if(set_hour == 5)
	{
		if(set_time_cnt == 1)			tft.setTextColor(ILI9341_WHITE);
	}
	else
	{
		tft.setTextColor(ILI9341_YELLOW);
	}	
	tft.print(con_min[5]);
	
    tft.setTextColor(ILI9341_YELLOW);
    tft.setCursor(90, 175);
    tft.setTextSize(2);
	tft.print("> 7. ");	
	if(set_hour == 6)
	{
		if(set_time_cnt == 0)			tft.setTextColor(ILI9341_WHITE);
	}
	else
	{
		tft.setTextColor(ILI9341_YELLOW);
	}
	tft.print(con_hour[6]);	
	tft.setTextColor(ILI9341_YELLOW);	tft.print(":");	
	if(set_hour == 6)
	{
		if(set_time_cnt == 1)			tft.setTextColor(ILI9341_WHITE);
	}
	else
	{
		tft.setTextColor(ILI9341_YELLOW);
	}	
	tft.print(con_min[6]);
	
    tft.setTextColor(ILI9341_YELLOW);   
    tft.setCursor(90, 195);
    tft.setTextSize(2);
	tft.print("> 8. ");	
	if(set_hour == 7)
	{
		if(set_time_cnt == 0)			tft.setTextColor(ILI9341_WHITE);
	}
	else
	{
		tft.setTextColor(ILI9341_YELLOW);
	}
	tft.print(con_hour[7]);	
	tft.setTextColor(ILI9341_YELLOW);	tft.print(":");	
	if(set_hour == 7)
	{
		if(set_time_cnt == 1)			tft.setTextColor(ILI9341_WHITE);
	}
	else
	{
		tft.setTextColor(ILI9341_YELLOW);
	}	
	tft.print(con_min[7]);	
}


void weigh_frame_display()
{
    tft.drawRect(30, 20, 260, 200, ILI9341_WHITE);
    tft.setCursor(75, 30);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.print("> WEIGHT SET <");
}


void weigh_set_display()
{
    if(con_weigh > 150)     con_weigh = 0;
	tft.setTextColor(ILI9341_YELLOW);
	tft.setCursor(110, 75);
	tft.setTextSize(5);
	tft.print(con_weigh);
    tft.setCursor(190, 75);
	tft.print(" g");
}


void loadcell_measure() 
{
    scale.set_scale(calibration_factor);                            // 로드셀 CALIBRATION 진행
    total = total - readings[readIndex];                            
    readings[readIndex] = scale.get_units(), 1;                     // 로드셀 데이터 측정
    total = total + readings[readIndex];                            // 기존데이터 + 최신데이터 = 데이터의 합
    readIndex = readIndex + 1;                                      // index 카운트
        
    if(readIndex >= cycles) 										// index와 cycles의 값이 같으면
    {                                       
        readIndex = 0;                                              // index 초기화
    }
    weight_average = total / cycles;                                // 데이터의 총합 / cycles = 평균데이터
}


void Serial_Packet(unsigned char ID, unsigned int Angle)
{
    unsigned char Check_Sum, Header = 0xFF;
    Serial.write(Header);
    Serial.write(Header);
    Serial.write(ID);
    Serial.write(0x05);
    Serial.write(0x03);
    Serial.write(0x1E);
    Serial.write(Angle);
    Serial.write(Angle>>8);
    Check_Sum = ~(ID + 0x05 + 0x03 + 0x1E + Angle + (Angle>>8));
    Serial.write(Check_Sum);
}
//
//void Packet(unsigned char ID, unsigned int Angle)
//{
//    unsigned char Check_Sum, Header = 0xFF;
//    AX_Serial.write(Header);
//    AX_Serial.write(Header);
//    AX_Serial.write(ID);
//    AX_Serial.write(0x05);
//    AX_Serial.write(0x03);
//    AX_Serial.write(0x1E);
//    AX_Serial.write(Angle);
//    AX_Serial.write(Angle>>8);
//    Check_Sum = ~(ID + 0x05 + 0x03 + 0x1E + Angle + (Angle>>8));
//    AX_Serial.write(Check_Sum);
//}
//
//void Packet_Baudrate()
//{
//    unsigned char Check_Sum;
//// Address = 0x04
//// Data = 1 -> 1Mbps
//// Data = 16 -> 115200bps
//// Data = 34 -> 57600bps
//// Data = 103 -> 19200bps
//// Data = 207 -> 9600bps
//
//
//    AX_Serial.write(0xFF);
//    AX_Serial.write(0xFF);
//    AX_Serial.write(0xFE);
//    AX_Serial.write(0x04);
//    AX_Serial.write(0x03);
//    AX_Serial.write(0x04);
//    AX_Serial.write(207);
//    Check_Sum = ~(0xFE + 0x04 + 0x03 + 0x04 + 207);
//    AX_Serial.write(Check_Sum);
//}
