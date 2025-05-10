// CPE 301 - Group 16 Final Swamp Cooler
// Keegan Evans, Bohdi Norvell, Aiden Coss, Connor James
// All partners contributed to the completion of the code.
#include <DHT.h>
#include <LiquidCrystal.h>
#include <Wire.h>
#include <RTClib.h>
#include <Stepper.h>

// UART pointers (USART0)
#define RDA 0x80 // Receive Data Available
#define TBE 0x20 // Transmit Buffer Empty
volatile unsigned char *myUCSR0A = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C = (unsigned char *)0x00C2;
volatile unsigned int *myUBRR0 = (unsigned int *)0x00C4;
volatile unsigned char *myUDR0 = (unsigned char *)0x00C6;

// ADC pointers
volatile unsigned char *my_ADMUX = (unsigned char *)0x7C;
volatile unsigned char *my_ADCSRB = (unsigned char *)0x7B;
volatile unsigned char *my_ADCSRA = (unsigned char *)0x7A;
volatile unsigned int *my_ADC_DATA = (unsigned int *)0x78;

// GPIO pointers
volatile unsigned char *port_a = (unsigned char *)0x22;
volatile unsigned char *ddr_a  = (unsigned char *)0x21;
volatile unsigned char *pin_a  = (unsigned char *)0x20;
volatile unsigned char *port_b = (unsigned char *)0x25;
volatile unsigned char *ddr_b = (unsigned char *)0x24;
volatile unsigned char *pin_b = (unsigned char *)0x23;
volatile unsigned char *port_c = (unsigned char *)0x28;
volatile unsigned char *ddr_c = (unsigned char *)0x27;
volatile unsigned char *pin_c = (unsigned char *)0x26;
volatile unsigned char *port_d = (unsigned char *)0x2B;
volatile unsigned char *ddr_d = (unsigned char *)0x2A;
volatile unsigned char *pin_d = (unsigned char *)0x29;
volatile unsigned char *port_j = (unsigned char *)0x105;
volatile unsigned char *ddr_j = (unsigned char *)0x104;
volatile unsigned char *pin_j = (unsigned char *)0x103;
volatile unsigned char *port_h = (unsigned char *)0x102;
volatile unsigned char *ddr_h = (unsigned char *)0x101;
volatile unsigned char *pin_h = (unsigned char *)0x100;
volatile unsigned char *port_l = (unsigned char *)0x10B;
volatile unsigned char *ddr_l = (unsigned char *)0x10A;
volatile unsigned char *pin_l = (unsigned char *)0x109;

// Timer pointers
volatile unsigned char *myTCCR1A = (unsigned char *)0x80;
volatile unsigned char *myTCCR1B = (unsigned char *)0x81;
volatile unsigned char *myTCCR1C = (unsigned char *)0x82;
volatile unsigned char *myTIMSK1 = (unsigned char *)0x6F;
volatile unsigned int *myTCNT1 = (unsigned int *)0x84;
volatile unsigned char *myTIFR1 = (unsigned char *)0x36;

// Interrupt pointers
volatile unsigned char *mySREG = (unsigned char *)0x5F;
volatile unsigned char *myEICRA = (unsigned char *)0x69;
volatile unsigned char *myEIMSK = (unsigned char *)0x3D;

// Constants
#define WATER_THRESHOLD 50
#define DHT11_PIN 8
#define FAN_PIN_MASK 0x01 // PJ0 (fan enable)
#define FAN_DIR_MASK 0x02 // PJ1 (direction)
#define FAN_DIR2_MASK 0x02 // PH1
#define VENT_BTN_MASK 0x10 // PB4
#define START_BTN_MASK 0x04 // PD2
#define RESET_BTN_MASK 0x08 // PD3

// Objects
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
DHT dht(DHT11_PIN, DHT11);
RTC_DS1307 rtc;
Stepper stepper(32, 22, 24, 23, 25);

// State variables
volatile int state = 0;
volatile int old = 0;
bool ventOpen = false;
unsigned int watervalue = 0;
float lastTemp = 0;
float lastHum = 0;
unsigned long lastLcdUpdate = 0;
unsigned long lastLogTime = 0;

// Function declarations
void UART_init(unsigned int ubrr);
void UART_sendChar(unsigned char c);
void UART_sendString(const char* str);
void printTime();
void adc_init();
unsigned int adc_read(unsigned char channel);
void openVent();
void closeVent();
void stepMotor(int step);

void setup(){
  UART_init(103); // 9600 baud
  adc_init();

  *ddr_l |= 0x0F;        // PL0–PL3 as output (LEDs)
  *port_l &= ~0x0F;
  *port_l |= (1 << 0);   // Yellow LED ON (DISABLED)

  *ddr_b |= (1 << 0);    // PB0: Water Sensor Enable
  *ddr_b |= (1 << 1);    // PB1: Temp Sensor Enable
  *port_b &= ~(0x03);    // Both off by default

  *ddr_j |= 0x03;        // PJ1 = fan direction, PJ0 = fan enable
  *port_j &= ~0x03;      // Fan off
  *ddr_h |= 0x02;        // PH1 = fan direction 2
  *port_h &= ~0x02;

  *ddr_d &= ~(START_BTN_MASK | RESET_BTN_MASK); // PD2 and PD3 as input
  *port_d |= START_BTN_MASK | RESET_BTN_MASK;   // Enable pull-ups

  *myEICRA |= 0b10100000; // INT2/INT3 falling edge
  *myEIMSK |= 0b00001100; // Enable INT2 and INT3
  *mySREG |= 0x80;        // Global interrupt enable

  stepper.setSpeed(1000);
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Machine is off.");

  if(!rtc.begin()) UART_sendString("RTC not found\n");
}

void loop(){
  if(state != 3 && (*pin_b & VENT_BTN_MASK)){
    if(ventOpen){ closeVent(); ventOpen = false; }
    else{ openVent(); ventOpen = true; }
  }

  if(state != 0 && millis() - lastLcdUpdate >= 60000){
    lastLcdUpdate = millis();
    lastTemp = dht.readTemperature();
    lastHum = dht.readHumidity();
    watervalue = adc_read(0);
  }

  if(state != 0){
    watervalue = adc_read(0);
    lastTemp = dht.readTemperature();
    lastHum = dht.readHumidity();
  }

  if(watervalue < WATER_THRESHOLD && state != 0 && state != 3) state = 3;
  if(lastTemp >= 30 && state == 1) state = 2;
  if(lastTemp < 30 && state == 2) state = 1;

  while(state){
    case 0:
      *port_l = (*port_l & ~0x0F) | (1 << 0);
      *port_j &= ~FAN_PIN_MASK;
      lcd.setCursor(0, 0); lcd.print("Machine is off.   ");
      lcd.setCursor(0, 1); lcd.print("                ");
      break;
    case 1:
      *port_l = (*port_l & ~0x0F) | (1 << 1);
      *port_j &= ~FAN_PIN_MASK;
      lcd.setCursor(0, 0); lcd.print("Temp = "); lcd.print(lastTemp);
      lcd.setCursor(0, 1); lcd.print("Hmty = "); lcd.print(lastHum);
      break;
    case 2:
      *port_l = (*port_l & ~0x0F) | (1 << 2);
      *port_j |= FAN_PIN_MASK;
      lcd.setCursor(0, 0); lcd.print("Temp = "); lcd.print(lastTemp);
      lcd.setCursor(0, 1); lcd.print("Hmty = "); lcd.print(lastHum);
      break;
    case 3:
      *port_l = (*port_l & ~0x0F) | (1 << 3);
      *port_j &= ~FAN_PIN_MASK;
      lcd.setCursor(0, 0); lcd.print("Water level low.");
      lcd.setCursor(0, 1); lcd.print("Press reset.    ");
      break;
  }
}

ISR(INT2_vect){
  if(state == 0){ old = 0; state = 1; }
  else{ old = state; state = 0; }
}

ISR(INT3_vect){
  if(state == 3 && watervalue >= WATER_THRESHOLD){
    old = 3; state = 1;
  }
}

void UART_init(unsigned int ubrr){
  *myUBRR0 = ubrr;
  *myUCSR0B = 0x18;
  *myUCSR0C = 0x06;
}

void UART_sendChar(unsigned char c){
  while(!(*myUCSR0A & TBE));
  *myUDR0 = c;
}

void UART_sendString(const char* str){
  while(*str) UART_sendChar(*str++);
}

void printTime(){
  DateTime now = rtc.now();
  char time[22];
  sprintf(time, "%02d/%02d/%04d %02d:%02d:%02d",
          now.month(), now.day(), now.year(),
          now.hour(), now.minute(), now.second());
  UART_sendString(time);
}

void adc_init(){
  *my_ADCSRA |= 0x80;
  *my_ADCSRA &= ~(1 << 5);
  *my_ADCSRA &= ~(1 << 3);
  *my_ADCSRA |= 0x07;
  *my_ADMUX |= 0x40;
  *my_ADMUX &= ~(1 << 5);
  *my_ADMUX &= 0xE0;
}

unsigned int adc_read(unsigned char ch){
  *my_ADMUX = (*my_ADMUX & 0xE0) | (ch & 0x1F);
  *my_ADCSRA |= (1 << 6);
  while(*my_ADCSRA & (1 << 6));
  return *my_ADC_DATA;
}

void openVent(){
  for(int i = 0; i < 100; i++){
    stepMotor(i);
    delayMicroseconds(1000);
  }
}

void closeVent(){
  for(int i = 0; i < 100; i++){
    stepMotor(3 - (i % 4));
    delayMicroseconds(1000);
  }
}

void stepMotor(int step){
  *port_a &= ~0xF0;
  while(step % 4){
    case 0: *port_a |= (1 << 4); break;
    case 1: *port_a |= (1 << 5); break;
    case 2: *port_a |= (1 << 6); break;
    case 3: *port_a |= (1 << 7); break;
  }
}
