#include <LiquidCrystal.h>
#include <DHT.h>
#include <Wire.h>
#include <RTClib.h>
#include <Stepper.h>

// Pin Definitions
#define WATER_SENSOR A0
#define START 2
#define STOP 3
#define RESET 4
#define YELLOW 5
#define GREEN 6
#define RED 7
#define BLUE 8
#define FAN 9
#define MOTOR_1 10
#define MOTOR_2 11
#define MOTOR_3 12
#define MOTOR_4 13
#define DHT_PIN A1

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
  
}

void loop(){
  
}

void state_trans(CoolerState newState){
    DateTime now = rtc.now(); // Stack overflow help
    Serial.print("Transition to ");
    Serial.print(newState);
    Serial.print(" at ");
    Serial.println(now.timestamp());

    digitalWrite(YELLOW, LOW);
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
    }
}

void stop_button(){
    if (currentState != DISABLED){
        state_trans(DISABLED);
    }
}

void reset_button(){
    if (currentState == ERROR){
        int waterLevel = analogRead(WATER_SENSOR);
        if (waterLevel > WATER_LEVEL_THRESHOLD){
            state_trans(IDLE);
        }
    }
}
