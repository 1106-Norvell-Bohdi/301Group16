#include <LiquidCrystal.h>
#include <DHT.h>
#include <Wire.h>
#include <RTClib.h>
#include <Stepper.h>

// Pin Definitions
#define WATER_SENSOR 0
#define START_STOP_BUTTON 1 // I think start/stop button should be same button 
#define RESET 2

#define YELLOW 3 // DISABLE
#define GREEN 4 // IDLE
#define RED 5  // ERROR
#define BLUE 6 // RUNNNG

// pins for LCD
const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;

#define FAN  13
#define MOTOR_1 14
#define MOTOR_2 15
#define MOTOR_3 16
#define MOTOR_4 17
#define DHT_TYPE DHT11
#define DHT_PIN A1

// UART 
#define RDA 0x80
#define TBE 0x20

//Pin for Button Interupt And set up 
#define PinInterupt 21
//prot d 
volatile unsigned char* DDRD = (unsigned char*)0x0A;
volatile unsigned char* PinD = (unsigned char*)0x09;
volatile bool idle;
void startStop(){
    idle = ~idle;
}



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
    sprintf(time, "%02d-%02d-%04d %02d:%02s:%02s ", now.month(), now.day(), now.year(), now.hour(), now.minute(), now.second());

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

// Function Prototypes
void start_button();
void stop_button();
void reset_button();
void update_LCD(); // Needs to be written
void water_level_check(); // Needs to be written
void state_trans(CoolerState newState);
void store_event(const char* event); // Needs to be written
uint16_t get_water_level();

// State Definitions
enum CoolerState { DISABLED, IDLE, ERROR, RUNNING };
CoolerState currentState = DISABLED;

// Global Objects
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
RTC_DS1307 rtc;
Stepper stepperMotor(STEPS_PER_REV, MOTOR_1, MOTOR_2, MOTOR_3, MOTOR_4);
DHT dht(DHT_PIN, DHT_TYPE);

bool systemEnabled = false;
unsigned long lastUpdateTime = 0;

void setup(){
    DDRE |= (1 << DDE5);
    DDRE |= (1 << DDE3);
    DDRG |= (1 << DDG5);
    DDRH |= (1 << DDH5);
   //sets pin 21 (port d 0) to input
    DDRD &= 0xFE; 
    PinD &= 0xFE;
    //set up intrrupt function
    attatchInterrupt(digitalPinToInterrupt(PinInterupt), startStop, CHANGE));

    U0init(9600);
    rtc.begin();
    lcd.begin(16,2);
    dht.begin();
}

void loop(){
  if(idle){

  }
  else{
    unsigned int waterValue = adc_read(WATER_SENSOR);
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
            PORTG |= (1 << PG5);
            store_event("System DISABLED")
            break;
        
        case IDLE:
            // PORTH |= ~(1 << PH5);
            PORTE |= (1 << PE3);
            store_event("System IDLE");
            break;
      
        case ERROR:
            // PORTH |= (1<< PH4);
            // PORTH &= ~(1<< PH6);
            PORTE |= (1 << PE5);
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

uint16_t get_water_level(){
    ADMUX = (ADMUX & 0xF0) | WATER_SENSOR;
    ADCSRA |= (1 << ADSC);
    while((1 << ADSC) & ADCSRA);
    return ADC;
}

void start_stop_button(){
    if (currentState == DISABLED){
        state_trans(IDLE);
        store_event("Start System");
    }
}

void stop_button(){
    if (currentState != DISABLED){
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

void update_LCD(){
    //implement delay for updates every minute
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" Temp(°C): ");
  lcd.print(dht.readTemperature());
  lcd.print("   Humidity(%): ");
  lcd.print(dht.readHumidity());

  lcd.setCursor(0,1);
  lcd.print("State: ");
  switch(currentState){
    case DISABLED: 
      lcd.print("DISABLED");
      break;
    case IDLE: 
      lcd.print("IDLE");
      break;
    case RUNNING:
      lcd.print("RUNNING");
      break;
    case ERROR: 
      lcd.print("ERROR");
      break;
  }
}

void store_event(const char* event){
  UART_print(event);
}
