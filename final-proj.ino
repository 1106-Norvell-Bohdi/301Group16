#include <LiquidCrystal.h>
#include <DHT.h>
#include <Wire.h>
#include <RTClib.h>
#include <Stepper.h>

// Pin Definitions
#define WATER_SENSOR A0
#define START 2 // I think start/stop button should be same button 
#define STOP 3
#define RESET 4

#define YELLOW 5 // DISABLE
#define GREEN 6 // IDLE
#define RED 7  // ERROR
#define BLUE 8 // RUNNNG

#define FAN 9
#define MOTOR_1 10
#define MOTOR_2 11
#define MOTOR_3 12
#define MOTOR_4 13
#define DHT_PIN A1

// UART REGISTER (CANT USE serial.print etc)
#define RDA 0x80
#define TBE 0x20

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
  U0putchar(now.timestamp());
  while(*msg != '\0'){
    U0putchar(*msg++);
  }
}

void UART_println(const char* msg){
  UART_print(msg);
  U0putchar('\n');
}

// Function Prototypes
void start_button();
void stop_button();
void reset_button();
void update_LCD(float temp, float humidity); // Needs to be written
void water_level_check(); // Needs to be written
void state_trans(CoolerState newState);
void store_event(const char* event); // Needs to be written

// State Definitions
enum CoolerState { DISABLED, IDLE, ERROR, RUNNING };
CoolerState currentState = DISABLED;

// Global Objects
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
DHT dht(DHT_PIN, DHT_TYPE);
RTC_DS1307 rtc;
Stepper stepperMotor(STEPS_PER_REV, MOTOR_1, MOTOR_2, MOTOR_3, MOTOR_4);

bool systemEnabled = false;
unsigned long lastUpdateTime = 0;

void setup(){
  U0init(9600);
}

void loop(){
  
}

void state_trans(CoolerState newState){
    DateTime now = rtc.now(); // Stack overflow help
    Serial.print("Transition to ");    //CANNOT use serial.print function
    Serial.print(newState);
    Serial.print(" at ");
    Serial.println(now.timestamp());

    digitalWrite(YELLOW, LOW);    // CANNOT USE digitalWrite function
    digitalWrite(GREEN, LOW);
    digitalWrite(RED, LOW);
    digitalWrite(BLUE, LOW);

    switch (newState){
        case DISABLED:
            digitalWrite(YELLOW, HIGH);
            digitalWrite(FAN, LOW);
            break;
        
        case IDLE:
            digitalWrite(GREEN, HIGH);
            break;
      
        case ERROR:
            digitalWrite(RED, HIGH);
            digitalWrite(FAN, LOW);
            lcd.clear();
            lcd.print("Error: Low Water");
            break;
      
        case RUNNING:
            digitalWrite(BLUE, HIGH);
            digitalWrite(FAN, HIGH);
            break;
    }
    currentState = newState;
}

void start_button(){
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
    if (currentState == ERROR){
        int waterLevel = analogRead(WATER_SENSOR);    // CANNOT use analogRead function 
        if (waterLevel > WATER_LEVEL_THRESHOLD){
            state_trans(IDLE);
          store_event("System Reset");
        }
    }
}

void update_LCD(float temp, float humidity){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" Temp(°C): ");
  lcd.print(temp);
  lcd.print(" Humidity(%): ");
  lcd.print(humidity);

  lcd.setCursor(0,1);
  lcd.print("State: ");
    //lcd.print(currentState) // Could use this directly instead of switch statement, not sure how the code will react with this tho
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

voide store_event(const char* event){
  UART_println(event);
}
