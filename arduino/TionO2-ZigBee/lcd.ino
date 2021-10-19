void interuptSet(uint8_t Cmd) {
  // Save state and disable/restore interupts
  // Cmd 1 - save and disable
  // Cmd 0 - restore
  
  static uint8_t SavePCICR;
  static uint8_t SaveTIMSK0;
  static uint8_t SaveSPCR;
  if(Cmd) {
    SavePCICR = PCICR;
    SaveTIMSK0 = TIMSK0;
    SaveSPCR = SPCR>>7;
    
    PCICR = 0;
    TIMSK0 = 0;
    SPCR &= ~(B10000000);
    return;
  }
  PCICR = SavePCICR;
  TIMSK0 = SaveTIMSK0;
  SPCR |= SaveSPCR<<7;
}

uint8_t getDump(uint8_t *State) {
  uint8_t i, p;
  uint16_t serBuf; 
  for (i=0; i<READBUFFERSIZE; i++) {
    ReadBuffer[i] = 0;
  }
  StartRead = 1;
  ReadBitPointer = 0;
  ReadBufferPointer = 0;
  ReadTimer = 0;
  Active = 1;
  serBuf = Serial.available();
  interuptSet(1);  // Save and disable other interupt
  
  PORTC |= B00000001;
  TCNT2 = STARTREADTIMEOUT;
  EIFR = B00000001;
  TIFR2 = B00000001;
  EIMSK = B00000001;  // Enable INT0
  TIMSK2 = 1;
  while (Active == 1) ;
  interuptSet(0);  // Enable other interupts
  
  serBuf = Serial.available() - serBuf;
  if (NewData == 0 || ErrorRead == 1 || serBuf != 0) {
    for (i=0; i<LCDSIZE; i++) {
      State[i] = 0;
    }
    return 1;
  }
  i = 0;
  p = 1;
  while (i < LCDSIZE && p < READBUFFERSIZE) {
    State[i] = ReadBuffer[p]<<1;
    State[i] |= ReadBuffer[p+1]&B00000001;
    i++;
    p = p + 3;
  }
  return 0;
}

uint8_t readLCD() {
  uint8_t i;
  for (i=0; i < 8; i++) {
    if (getDump(LCDState)) continue;
    delay(LCDBLINKTIME);
    if (getDump(LCDState2)) continue;
    break;
  }
  
  if (i >= 8) return 1;
  for (i=0; i<LCDSIZE; i++) {
    LCDState[i] |= LCDState2[i];
  }
  return 0;
}

uint8_t smallDigit2BigDigit(uint8_t LByte, uint8_t HByte) {
  uint8_t BigDg;
  BigDg = (HByte&B00010000)<<2;
  BigDg |= (LByte&B00001000)<<2;
  BigDg |= (HByte&B00001000)<<1;
  BigDg |= (LByte&B00000110)<<1;
  BigDg |= (HByte&B00000110)>>1;
  return BigDg;
}

uint8_t segments2Digit(uint8_t InByte) {
  InByte &= ~(B10000000);
  uint8_t Return = 0xFF;
  uint8_t i;
  for (i=0; i<CHARARRAYSIZE; i++) {
    if (InByte == pgm_read_byte_near(&CharArray[i])) {
      Return = i;
      break;
    }
  }
  return Return;
}

int8_t decode2Digits(uint8_t HByte, uint8_t LByte) {
  int8_t Return;
  uint8_t HDigit = segments2Digit(HByte);
  if (HDigit > 9) {
    if (HDigit != 19 && HDigit != 20) return 127;
    Return = 0;
  } else Return = HDigit;
  
  uint8_t LDigit = segments2Digit(LByte);
  if (LDigit <= 9) {
    Return *= 10;
    Return += LDigit;
  }
  if (HDigit == 20 && LDigit == 20) return 127;
  if (HByte&B10000000 || HDigit == 19) Return *= -1;
  return Return;
}

uint16_t decode3Digits(uint8_t HByte, uint8_t MByte, uint8_t LByte) {
  uint16_t Return = segments2Digit(HByte) * 100;
  Return += segments2Digit(MByte) * 10;
  Return += segments2Digit(LByte);
  return Return;
}

uint8_t getSpeed() {
  if (LCDState[3]&B00001100) return 4;
  if (LCDState[3]&B00010000) return 3;
  if (LCDState[2]&B00010000) return 2;
  if (LCDState[2]&B00000110) return 1;
  return 0;
}

uint8_t getScreen() {
  uint8_t Byte0 = segments2Digit(LCDState[0]);
  if (Byte0 == 21) return 0;                   // One of the error screens 
  
  if (LCDState[2]&B1100) return 1;             // Main screen

  if (Byte0 == 19) return 2;                  // Off screen

  if (LCDState[3]&1) return 4;                // Set time

  if (LCDState[2]&B10) return 6;              // Set start timer screen

  if (LCDState[3]&B10) return 7;              // Set stop timer screen

  if (LCDState[8]&B10000) return 5;           // Set on/off timer screen

  if (LCDState[4] && !LCDState[0]) return 3;  // Set temperature screen

  if (LCDState[0] && !LCDState[4]) return 8;  // Set min outside temperature

  if (LCDState[6]&1) return 9;                // Set filter timer screen

  return 0;                                   // Error screen
}

void mainScr() {
  int8_t T;
  if (BrizerPower == 0) FlagChangeValues = 1;
  BrizerPower = 1;
  T = getSpeed();
  if (BrizerSpeed != T) FlagChangeValues = 1;
  BrizerSpeed = T;
  heatScr();
  T = decode2Digits(LCDState[0], LCDState[1]);
  if (BrizerOutsideTemp != T) FlagChangeValues = 1;
  BrizerOutsideTemp = T;
  T = (LCDState[8]>>4)&1;
  if (BrizerTimerState != T) FlagChangeValues = 1;
  BrizerTimerState = T;
  if (LCDState[6]&1) filterScr();
}

void heatScr() {
  int8_t T;
  if (BrizerPower == 0) FlagChangeValues = 1;
  BrizerPower = 1;
  T = (~(LCDState[3]>>5)&1);
  if (BrizerHeatStateSetting != T) FlagChangeValues = 1;
  BrizerHeatStateSetting = T;
  int8_t HeatTemp = decode2Digits(LCDState[4], LCDState[5]);
  if (HeatTemp > 100) {
    if (BrizerHeatState != 0) FlagChangeValues = 1;
    BrizerHeatState = 0;
  } else {
    if (BrizerHeatState != 1) FlagChangeValues = 1;
    BrizerHeatState = 1;
    if (BrizerHeatTemp != HeatTemp) FlagChangeValues = 1;
    BrizerHeatTemp = HeatTemp;
  }
}

void filterScr() {
  if (BrizerPower == 0) FlagChangeValues = 1;
  BrizerPower = 1;
  uint16_t T;
  uint8_t HByte, MByte, LByte;
  HByte = smallDigit2BigDigit(LCDState[6], LCDState[7]);
  MByte = smallDigit2BigDigit(LCDState[8], LCDState[9]);
  LByte = smallDigit2BigDigit(LCDState[10], LCDState[11]);
  T= decode3Digits(HByte, MByte, LByte);
  if (BrizerFilterTime != T) FlagChangeValues = 1;
  BrizerFilterTime = T;
}

void timeScr(uint8_t Screen) {
  BrizerPower = 1;
  uint8_t Hours, Minutes;
  Hours = decode2Digits(LCDState[0], LCDState[1]);
  Minutes = decode2Digits(LCDState[4], LCDState[5]);
  switch (Screen) {
    case 4:
      BrizerTimeHR = Hours;
      BrizerTimeMIN = Minutes;
      break;
    case 6:
      BrizerTimerOnHR = Hours;
      BrizerTimerOnMIN = Minutes;
      break;
    case 7:
      BrizerTimerOffHR = Hours;
      BrizerTimerOffMIN = Minutes;
      break;
  }
}

void timerStScr() {
  BrizerPower = 1;
  uint8_t Char2 = smallDigit2BigDigit(LCDState[8], LCDState[9]);
  if (Char2 == 0x38) BrizerTimerState = 1;
  else BrizerTimerState = 0;
}

void minOutTempScr() {
  BrizerPower = 1;
  BrizerMinOutTemp = decode2Digits(LCDState[0], LCDState[1]);
}

void offScr() {
  if (BrizerPower == 1) FlagChangeValues = 1;
  BrizerPower = 0;
  BrizerTimerState = (LCDState[8]>>4)&1;
}

void errorScr() {
  if (FlagChangeValues < 2) FlagChangeValues = 1;
  BrizerErrorStr[0] = pgm_read_byte_near(&Char2CharArray[segments2Digit(LCDState[0])]);
  BrizerErrorStr[1] = pgm_read_byte_near(&Char2CharArray[segments2Digit(LCDState[1])]);
  if (BrizerErrorStr[0] == 0x4E && BrizerErrorStr[1] == 0x30 ) BrizerErrorStr[1] = 0x4F;
  BrizerErrorStr[2] = pgm_read_byte_near(&Char2CharArray[segments2Digit(LCDState[4])]);
  BrizerErrorStr[3] = pgm_read_byte_near(&Char2CharArray[segments2Digit(LCDState[5])]);
  BrizerErrorStr[4] = '\0';
}

uint8_t analyseLCD() {
  // Screen:
  // 1 - main
  // 2 - off
  // 3 - settings heat temperature
  // 4 - time
  // 5 - timer on/off
  // 6 - timer start
  // 7 - timer stop
  // 8 - min outside temperature
  // 9 - filter
  // 0 - error
  if (readLCD()) return 1;
  uint8_t Screen = getScreen();
  switch(Screen) {
    case 1:
      mainScr();
      break;
    case 2:
      offScr();
      break;
    case 3:
      heatScr();
      break;
    case 4:
    case 6:
    case 7:
      timeScr(Screen);
      break;
    case 5:
      timerStScr();
      break;
    case 8:
      minOutTempScr();
      break;
    case 9:
      filterScr();
      break;
    case 0:
      errorScr();
      break;
  }
  return Screen;
}
