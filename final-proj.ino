// CPE 301 - Group 16 Final Swamp Cooler
// Keegan Evans, Bohdi Norvell, Aiden Coss, Connor James
// All partners contributed to the completion of the code.


#include <LiquidCrystal.h>
#include <DHT.h>
#include <Wire.h>
#include <RTClib.h>
#include <Stepper.h>


// Pin Definitions
#define WATER_LEVEL_THRESHOLD 200
#define TEMP_HIGH_THRESHOLD 30 
#define TEMP_LOW_THRESHOLD 24

#define WATER_SENSOR A0 //PF0
#define START_STOP_BUTTON 1 // PE1
#define RESET 2    //PE4

#define YELLOW 3 // PE5 - DISABLE
#define GREEN 4 // PG5 - IDLE
#define RED 5  // PE3 - ERROR
#define BLUE 6 // PH3 - RUNNNG

#define DHT_PIN 13    // PB7 
#define DHT_TYPE 11    //DHT type 11
#define MOTOR_1 14    // PJ1
#define MOTOR_2 15    // PJ0
#define MOTOR_3 16    // Ph1
#define MOTOR_4 17    // PH0
#define FAN_PIN 30     // PC7

// State Definitions
enum CoolerState { DISABLED, IDLE, ERROR, RUNNING };
CoolerState currentState = DISABLED;

// pins for LCD
const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;
// Global Objects
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
RTC_DS1307 rtc;
//Stepper stepperMotor(STEPS_PER_REV, MOTOR_1, MOTOR_2, MOTOR_3, MOTOR_4);
DHT dht(DHT_PIN, DHT_TYPE);

bool systemEnabled = false;
unsigned long lastUpdateTime = 0;

// Function Prototypes
void start_stop_button();
void reset_button();
void update_LCD(); 
void water_level_check(); 
void state_trans(CoolerState newState);
void store_event(const char* event); 
uint16_t get_water_level();
void control_fan();
void control_stepper();
void check_system_timers();

// UART 
#define RDA 0x80
#define TBE 0x20

//Pin for Button Interupt And set up 
#define PinInterupt 21
#define PinRestart 22
//prot d
volatile unsigned char* my_DDRD = (unsigned char*)0x0A;
volatile unsigned char* my_PinD = (unsigned char*)0x09;

const int stepsPerCycle = 100;
Stepper Mystepper(stepsPerCycle, 14, 15, 16 ,17);

volatile unsigned char* myUCSR0A = (unsigned char*)0x00C0;
volatile unsigned char* myUCSR0B = (unsigned char*)0x00C1;
volatile unsigned char* myUCSR0C = (unsigned char*)0x00C2;
volatile unsigned int*  myUBRR0  = (unsigned int*) 0x00C4;
volatile unsigned char* myUDR0   = (unsigned char*)0x00C6;

void U0init(unsigned long U0baud){
    unsigned long FCPU = 16000000;
    unsigned int tbaud = (FCPU / 16 / U0baud) - 1;
    *myUCSR0A = 0x20;
    *myUCSR0B = 0x18;
    *myUCSR0C = 0x06;
    *myUBRR0 = tbaud;
}

void U0putchar(unsigned char U0data){
    while(!(*myUCSR0A & TBE));
    *myUDR0 = U0data;
}

void UART_print(const char* msg){
  DateTime now = rtc.now();
    char time[20];
    sprintf(time, "%02d-%02d-%04d %02d:%02d:%02d ", now.month(), now.day(), now.year(), now.hour(), now.minute(), now.second());

const char* ptr = time;
while(*ptr != '\0'){
    U0putchar(*ptr++);
}
  while(*msg != '\0'){
    U0putchar(*msg++);
  }
    U0putchar('\n');
}
// ADC
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

void adc_init(){
    *my_ADCSRA |= 0x80;
    *my_ADCSRA &= ~(0x40);
    *my_ADCSRA &= ~(0x20);
    *my_ADCSRA |= 0x07;

    *my_ADCSRB &= ~(0x08);
    *my_ADCSRB &= ~(0x07);

    *my_ADMUX &= ~(0x80);
    *my_ADMUX |= 0x40;
    *my_ADMUX &= ~(0x20);
    *my_ADMUX &= ~(0x1F);
}

unsigned int adc_read(unsigned char adc_channel_num){
    *my_ADMUX &= ~(0x1F);
    *my_ADCSRB &= ~(0x08);
    *my_ADCSRA |= 0x40;
    while((*my_ADCSRA & 0x40) != 0);
    return *my_ADC_DATA;
}

void setup(){
    DDRE |= (1 << DDE5);
    DDRE |= (1 << DDE3);
    DDRG |= (1 << DDG5);
    DDRH |= (1 << DDH5);
   //sets pin 21 (port d 0) to input
    *my_DDRD &= 0xFE; 
    *my_PinD &= 0xFE;
    *my_DDRD &= 0xFD;
    *my_PinD &= 0xFD;

    pinMode(PinInterupt, INPUT_PULLUP);
    pinMode(PinRestart, INPUT_PULLUP);
    
    //set up intrrupt function
    attachInterrupt(digitalPinToInterrupt(PinInterupt), start_stop_button, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PinRestart), reset_button, CHANGE);

    Mystepper.setSpeed(60);

    U0init(9600);
    rtc.begin();
    lcd.begin(16,2);
    dht.begin();
}

unsigned long currentMillis;
const unsigned long updateInterval = 60000; // 1 minute in milliseconds

void loop() {
  currentMillis = millis();

  // Update LCD every minute if system is active
  if (currentState != DISABLED && currentMillis - lastUpdateTime >= updateInterval) {
    update_LCD();
    lastUpdateTime = currentMillis;
  }

  // Continuously monitor water level
  if (currentState != DISABLED) {
    water_level_check();
  }

  // Fan control based on temperature (IDLE/RUNNING only)
  if (currentState == IDLE || currentState == RUNNING) {
    control_fan();
  }

  // Allow vent position control in all states except DISABLED
  if (currentState != DISABLED) {
    control_stepper();
  }
}

// ISR FOR BUTTONS START/STOP & REST
void start_stop_button(){
    if (currentState == DISABLED){
        state_trans(IDLE);
        store_event("Start System");
    }
    else{
        state_trans(DISABLED);
        store_event("Stopped System");
    }
}

void reset_button(){
    if(currentState == ERROR){
        uint16_t waterLevel = get_water_level();
        if (waterLevel > WATER_LEVEL_THRESHOLD){
            state_trans(IDLE);
            store_event("System Reset");
        }
    }
}

void update_LCD() {
    lcd.clear();

    // Read sensor values
    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();
    uint16_t waterLevel = adc_read(WATER_SENSOR);
    DateTime now = rtc.now();

    // Line 1: Temp and Humidity
    lcd.setCursor(0, 0);
    if (isnan(temp) || isnan(humidity)) {
        lcd.print("Sensor error");
    } else {
        lcd.print("T:");
        lcd.print(temp, 1);
        lcd.print("C H:");
        lcd.print(humidity, 0);
        lcd.print("%");
    }

    // Line 2: State, Water Level, Time (HH:MM)
    lcd.setCursor(0, 1);
    lcd.print("S:");
    switch(currentState){
        case DISABLED: lcd.print("DIS "); break;
        case IDLE:     lcd.print("IDL "); break;
        case RUNNING:  lcd.print("RUN "); break;
        case ERROR:    lcd.print("ERR "); break;
    }

    lcd.print("W:");
    lcd.print(waterLevel);

    lcd.print(" ");
    if (now.hour() < 10) lcd.print("0");
    lcd.print(now.hour());
    lcd.print(":");
    if (now.minute() < 10) lcd.print("0");
    lcd.print(now.minute());
}

void water_level_check(){
    uint16_t waterLevel = get_water_level();
    if (waterLevel < WATER_LEVEL_THRESHOLD) {
    if (currentState != DISABLED && currentState != ERROR) {
      state_trans(ERROR);
    }
  } else if (currentState == ERROR) {
    // Water level is now good, but need reset button to return to IDLE
  }
}

void state_trans(CoolerState newState){
    PORTE &= ~(1 << PE5); // This is for yellow which is pin 3 AKA disabled
    PORTG &= ~(1 << PG5); // This is for green which is pin 4 AKA IDLE
    PORTE &= ~(1 << PE3); // THis is for RED which is pin 5 AKA ERROR
    PORTH &= ~(1 << PH3); // This  is for Blue which is pin 6 AKA RUNNING

    switch(newState){
        case DISABLED:
            // PORTE |= ~(1 << PE5);
            // PORTH &= ~(1 << PH6);
            PORTE |= (1 << PE5);
            store_event("System DISABLED");
            break;
        
        case IDLE:
            // PORTH |= ~(1 << PH5);
            PORTG |= (1 << PG5);
            store_event("System IDLE");
            break;
      
        case ERROR:
            // PORTH |= (1<< PH4);
            // PORTH &= ~(1<< PH6);
            PORTE |= (1 << PE3);
            lcd.clear();
            lcd.print("System Error: Low Water");
            store_event("System ERROR");
            break;
      
        case RUNNING:
            // PORTH |= (1 << PH5);
            // PORTH &= ~(1 << PH6);
            PORTH |= (1 << PH3);
            store_event("System RUNNING");
            break;
    }
    currentState = newState;
}


void store_event(const char* event) {
    DateTime now = rtc.now();
    char logLine[100];

    // Format: [MM-DD-YYYY HH:MM:SS] 
    sprintf(logLine, "[%02d-%02d-%04d %02d:%02d:%02d] %s",
            now.month(), now.day(), now.year(),
            now.hour(), now.minute(), now.second(),
            event);

    UART_print(logLine);
}
uint16_t get_water_level(){
    ADMUX = (ADMUX & 0xF0) | WATER_SENSOR;
    ADCSRA |= (1 << ADSC);
    while((1 << ADSC) & ADCSRA);
    return ADC;
}

void control_fan() {
    float temp = dht.readTemperature();

    if (isnan(temp)) return;

    if (temp >= TEMP_HIGH_THRESHOLD && currentState == IDLE) {
         PORTC |= (1 << PC7);
        state_trans(RUNNING);
    } else if (temp <= TEMP_LOW_THRESHOLD && currentState == RUNNING) {
        PORTC &= ~(1 << PC7);
        state_trans(IDLE);
    }
}

void control_stepper() {
    if(currentState == RUNNING){
        Mystepper.step(stepsPerCycle);
    }
    else if(currentState == DISABLED || currentState == IDLE){
        Mystepper.step(-stepsPerCycle);
    }
    else{
    }
    // Log movement event
    store_event("Stepper position changed");
}

void check_system_timers() {
    if (millis() - lastUpdateTime >= 60000) {
        update_LCD();
        water_level_check();
        control_fan();
        lastUpdateTime = millis();
    }
}
