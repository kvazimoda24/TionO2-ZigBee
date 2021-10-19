#include <avr/eeprom.h>

//INT0 - WD
//INT1 - RD
//PCINT2 - CS

#define READBUFFERSIZE 64
#define LCDSIZE 12
#define LCDBLINKTIME 450 //  step 16,384ms
#define STARTREADTIMEOUT 225
#define FREQUENCYSENDSTATE 2400 // step 0.5 seconds

#define SHORTPRESS 100
#define LONGPRESS 2200
#define DELAYPRESS 100

#define SERIAL_TX_BUFFER_SIZE 128
#define SERIAL_RX_BUFFER_SIZE 128
#define OUTPUT_SERIAL_BUFFER 100

#define EEPROMAUTOSTART 0

uint8_t SerialCmd[4];

char OutputSerialBuffer[OUTPUT_SERIAL_BUFFER];

volatile uint8_t Active = 0;
volatile uint8_t NewData = 0;

volatile uint8_t ReadBuffer[READBUFFERSIZE];
volatile uint8_t ReadBufferPointer = 0;
volatile uint8_t ReadBitPointer = 0;
volatile uint8_t ReadPause = 0;
volatile uint8_t Input;

volatile uint8_t StartRead = 0;
volatile uint8_t ReadTimer = 0;
volatile uint8_t ErrorRead = 0;
uint8_t LCDState[LCDSIZE];
uint8_t LCDState2[LCDSIZE];

uint8_t BrizerAutoStart, BrizerPower, BrizerSpeed, BrizerHeatStateSetting, BrizerHeatState;
int8_t  BrizerOutsideTemp, BrizerHeatTemp, BrizerMinOutTemp;
int16_t BrizerFilterTime;
uint8_t BrizerTimerState, BrizerTimerOnHR, BrizerTimerOnMIN;
uint8_t BrizerTimerOffHR, BrizerTimerOffMIN;
uint8_t BrizerTimeHR, BrizerTimeMIN;
char BrizerErrorStr[5];

uint8_t FlagChangeValues;
uint16_t Cicles;

#define CHARARRAYSIZE 22
const PROGMEM uint8_t CharArray[] = {
  0x77, 0x12, 0x6B, 0x5B, 0x1E, 0x5D, 0x7D, 0x13, 0x7F, 0x5F,  // 0-9
  0x3F, 0x7C, 0x65, 0x7A, 0x6D, 0x2D,                          // A-F
  0x38, 0x78, 0x28, 0x08, 0x00, 0x37                           // n, o, r, -, space, N
};
const PROGMEM uint8_t Char2CharArray[] = {
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,  // 0-9
  0x41, 0x42, 0x43, 0x44, 0x45, 0x46,                          // A-F
  0x6E, 0x6F, 0x72, 0x2D, 0x20, 0x4E                           // n, o, r, -, space, N
};

void setup() {
  // PORD 2-4 INPUT
  DDRD &= ~(B00011100);
  //PORTD 5-7 OUTPUT and DOWN
  DDRD |= B11100000;
  PORTD &= ~(B11100000);
  // PORTB 0 OUTPUT and DOWN
  DDRB |= B1;
  PORTB &= ~(B1);
  // PORTC 0 OUTPUT and DOWN
  DDRC |= B1;
  PORTC &= ~(B1);

  // Serial
  Serial.begin(9600);
  //Serial.println("Start");

  setupInterupts();
  autoStart();
  sendExtStatus();
}

void loop() {
  Serial.flush();
  checkSerialRX();
  uint8_t Screen = analyseLCD();
  savePowerState();
  if (Cicles > FREQUENCYSENDSTATE) FlagChangeValues = 1;
  sendState(Screen);
  Cicles++;
  delay(500);
}
