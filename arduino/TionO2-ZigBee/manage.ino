void pressKey (uint8_t Keys, uint16_t Delay) {
  PORTD |= Keys<<5;
  PORTB |= (Keys>>3)&1;
  delay(Delay);
  PORTD &= ~(B11100000);
  PORTB &= ~1;
  delay(DELAYPRESS);
}

void setPwr(int16_t *Param) {
  analyseLCD();
  if (*Param == 1 && BrizerPower == 0) pressKey(1, SHORTPRESS);
  if (*Param == 0 && BrizerPower == 1) pressKey(1, SHORTPRESS);
}

void setSpd(int16_t *Param) {
  uint8_t Screen = analyseLCD();
  if (Screen == 0) return;
  if (Screen == 2) {
    pressKey(1, SHORTPRESS);
    setSpd(Param);
    pressKey(1, SHORTPRESS);
    return;
  }
  if (Screen == 1) {
    if (*Param < 1 || *Param > 4) return;
    if (BrizerSpeed == *Param) return;
    setSimpleValue(Param, &BrizerSpeed);
  }
  pressKey(B1000, LONGPRESS);
  setSpd(Param);
}

void setHte(int16_t *Param) {
  uint8_t Screen = analyseLCD();
  if (Screen == 1 && BrizerHeatStateSetting == *Param) return;
  Screen = switchToSetupScreen(3);
  if (Screen == 0) return;
  if (BrizerHeatStateSetting == *Param) {
    pressKey(B1000, LONGPRESS);
    return;
  }
  pressKey(B110, LONGPRESS);
  pressKey(B1000, LONGPRESS);
  if (Screen == 2) pressKey(1, SHORTPRESS);
}

void setHtt(int16_t *Param) {
  if (*Param < -20 || *Param > 25) return;
  uint8_t Screen = analyseLCD();
  if (Screen == 1) {
    if (BrizerHeatState == 1 && BrizerHeatTemp == *Param) return;
  }
  Screen = switchToSetupScreen(3);
  if (Screen == 0) return;
  if (BrizerHeatState == 1 && BrizerHeatTemp == *Param) {
    pressKey(B1000, LONGPRESS);
    return;
  }
  if (BrizerHeatStateSetting == 0) {
    pressKey(B110, LONGPRESS);
    analyseLCD();
  }
  setSimpleValue(Param, &BrizerHeatTemp);
  pressKey(B1000, LONGPRESS);
  if (Screen == 2) pressKey(1, SHORTPRESS);
}

void setMnt(int16_t *Param) {
  if (*Param < -40 || *Param > -25) return;
  int8_t Temp = (*Param * -1) % 10;
  *Param = *Param + Temp;
  if (Temp > 2) *Param = *Param - 5;
  if (Temp > 7) *Param = *Param - 5;
  uint8_t Screen = switchToSetupScreen(8);
  if (Screen == 0) return;
  uint8_t i = 0;
  while (*Param != BrizerMinOutTemp && i < 5) {
    pressKey(B10, SHORTPRESS);
    analyseLCD();
    i++;
  }
  pressKey(B1000, LONGPRESS);
  if (Screen == 2) pressKey(1, SHORTPRESS);
}

void setTms(int16_t *Param) {
  uint8_t Screen = analyseLCD();
  if (Screen == 1 || Screen == 2) {
    if (BrizerTimerState == *Param) return;
  }
  Screen = switchToSetupScreen(5);
  if (Screen == 0) return;
  if (BrizerTimerState == *Param) {
    pressKey(B1000, LONGPRESS);
    return;
  }
  pressKey(B10, SHORTPRESS);
  pressKey(B1000, LONGPRESS);
  if (Screen == 2) pressKey(1, SHORTPRESS);
}

void setTme(int16_t *Param) {
  if (checkInputTime(Param) != 0) return;
  uint8_t Screen = switchToSetupScreen(5);
  if (Screen == 0) return;
  if (BrizerTimerState == 0) pressKey(B10, SHORTPRESS);
  if (switchToSetupScreen(6) == 0) return;
  setTime(Param, &BrizerTimerOnHR, &BrizerTimerOnMIN, 1);
  if (Screen == 2) pressKey(1, SHORTPRESS);
}

void setTmd(int16_t *Param) {
  if (checkInputTime(Param) != 0) return;
  uint8_t Screen = switchToSetupScreen(5);
  if (Screen == 0) return;
  if (BrizerTimerState == 0) pressKey(B10, SHORTPRESS);
  if (switchToSetupScreen(7) == 0) return;
  setTime(Param, &BrizerTimerOffHR, &BrizerTimerOffMIN, 1);
  if (Screen == 2) pressKey(1, SHORTPRESS);
}

void setTim(int16_t *Param) {
  if (checkInputTime(Param) != 0) return;
  uint8_t Screen = switchToSetupScreen(4);
  if (Screen == 0) return;
  setTime(Param, &BrizerTimeHR, &BrizerTimeMIN, 0);
  if (Screen == 2) pressKey(1, SHORTPRESS);
}

void setApw(int16_t *Param) {
  //BrizerAutoStart = (eeprom_read_byte(EEPROMAUTOSTART)&B10)>>1;
  analyseLCD();
  if (BrizerAutoStart == 1 && *Param == 0) eeprom_write_byte(EEPROMAUTOSTART, 0);
  if (BrizerAutoStart == 0 && *Param == 1) eeprom_write_byte(EEPROMAUTOSTART, B10 + BrizerPower);
  BrizerAutoStart = *Param&1;
}

void autoStart() {
  BrizerAutoStart = eeprom_read_byte(EEPROMAUTOSTART);
  if (BrizerAutoStart == B11) {
    delay(1000);
    if (analyseLCD() == 2) pressKey(1, SHORTPRESS);
  }
  if (BrizerAutoStart == B10 || BrizerAutoStart == B11) BrizerAutoStart = 1;
  else BrizerAutoStart = 0;
}

void savePowerState() {
  static uint8_t OldBrizerPower;
  if (OldBrizerPower == BrizerPower) return;
  uint8_t EepromState = eeprom_read_byte(EEPROMAUTOSTART);
  if (EepromState>>1 != 1) return;
  if (BrizerPower == EepromState&1) return;
  eeprom_write_byte(EEPROMAUTOSTART, B10 + BrizerPower);
  OldBrizerPower = BrizerPower;
}

void setTime(int16_t *NeedTime, uint8_t *CurrHR, uint8_t *CurrMIN, uint8_t IsTimer) {
  uint8_t NeedHR = *NeedTime / 100;
  uint8_t NeedMIN = *NeedTime % 100;
  uint8_t p = 0;
  uint8_t i = 0;
  if (IsTimer) {
    uint8_t TempMIN = NeedMIN % 10;
    NeedMIN = NeedMIN - TempMIN;
    if (TempMIN > 2) NeedMIN = NeedMIN + 5;
    if (TempMIN > 7) NeedMIN = NeedMIN + 5;
  }
  if (*CurrMIN > NeedMIN) p = 60 - *CurrMIN + NeedMIN;
  else p = NeedMIN - *CurrMIN;
  if (IsTimer) p = p / 5;
  while (i < p) {
    pressKey(B100, SHORTPRESS);
    i++;
  }
  i = 0;
  if (*CurrHR > NeedHR) p = 24 - *CurrHR + NeedHR;
  else p = NeedHR - *CurrHR;
  while (i < p) {
    pressKey(B10, SHORTPRESS);
    i++;
  }
  pressKey(B1000, LONGPRESS);
}

uint8_t checkInputTime(int16_t *Param) {
  if (*Param < 0) return 1;
  if ((*Param / 100) > 23) return 1;
  if ((*Param % 100) > 59) return 1;
  return 0;
}

void setSimpleValue(int16_t *NeedValue, int8_t *CurrValue) {
  if (*CurrValue < *NeedValue) {
    pressKey(B100, SHORTPRESS);
    analyseLCD();
    if (*CurrValue == *NeedValue) return;
    uint8_t i;
    for (i=0; i<(*NeedValue - *CurrValue); i++) {
      pressKey(B100, SHORTPRESS);
    }
  }
  if (*CurrValue > *NeedValue) {
    pressKey(B10, SHORTPRESS);
    analyseLCD();
    if (*CurrValue == *NeedValue) return;
    uint8_t i;
    for (i=0; i<(*CurrValue - *NeedValue); i++) {
      pressKey(B10, SHORTPRESS);
    }
  }
}

uint8_t switchToSetupScreen(uint8_t NeedScreen) {
  uint8_t Screen = analyseLCD();
  if (Screen == 0) return 0;
  uint8_t Return;
  if (Screen == 2) {
    pressKey(1, SHORTPRESS);
    Return = 2;
  } else {
    Return = 1;
  }
  uint8_t i = 0;
  while (i < 10) {
    if (analyseLCD() == NeedScreen) break;
    pressKey(B1000, SHORTPRESS);
    i++;
  }
  if (i >= 10) {
    if (Return == 2) {
      pressKey(1, SHORTPRESS);
      delay(DELAYPRESS);
    }
    return 0;
  }
  return Return;
}
