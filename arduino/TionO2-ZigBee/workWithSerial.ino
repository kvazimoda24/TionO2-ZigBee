// PWR - power
// SPD - speed
// HTE - heater enable
// HTT - heater temperature
// MNT - min outside temperature
// TMS - timer enable
// TME - timer on
// TMD - timer off
// TIM - time
// APW - auto power on
// EXS - get timers and min outside temperature

void checkSerialRX() {
  uint8_t SerialRX[SERIAL_RX_BUFFER_SIZE];
  uint8_t i;
  if (Serial.available() > 0) {
    i = Serial.readBytesUntil("\r", SerialRX, 127);
    SerialRX[i] = '\0';
    uint8_t *index_t = strtok(SerialRX, ":");
    while (index_t != NULL) {
      if (strlen(index_t) > 3) break;
      
      strcpy(SerialCmd, index_t);
      index_t = strtok(NULL, ",");
      if (strcmp(SerialCmd, "PWR") == 0) setPwr(strToInt(index_t));
      if (strcmp(SerialCmd, "SPD") == 0) setSpd(strToInt(index_t));
      if (strcmp(SerialCmd, "HTE") == 0) setHte(strToInt(index_t));
      if (strcmp(SerialCmd, "HTT") == 0) setHtt(strToInt(index_t));
      if (strcmp(SerialCmd, "MNT") == 0) setMnt(strToInt(index_t));
      if (strcmp(SerialCmd, "TMS") == 0) setTms(strToInt(index_t));
      if (strcmp(SerialCmd, "TME") == 0) setTme(strToInt(index_t));
      if (strcmp(SerialCmd, "TMD") == 0) setTmd(strToInt(index_t));
      if (strcmp(SerialCmd, "TIM") == 0) setTim(strToInt(index_t));
      if (strcmp(SerialCmd, "APW") == 0) setApw(strToInt(index_t));
      if (strcmp(SerialCmd, "EXS") == 0) sendExtStatus();

      index_t = strtok(NULL, ":");
    }
  }
}

void sendState(uint8_t Screen) {
  if (FlagChangeValues == 0) return;
  if (Screen > 2) return;
  
  uint8_t Convert_Buffer[5];
  if (Screen == 0 && FlagChangeValues < 2) {
    sprintf(OutputSerialBuffer, "{\"ERROR\":\"%s\"}\r", BrizerErrorStr);
    FlagChangeValues = 2;
  }
  if (Screen == 2) {
    strcpy(OutputSerialBuffer, "{\"PWR\":\"0\"}\r");
    FlagChangeValues = 0;
  }
  if (Screen == 1) {
    sprintf(OutputSerialBuffer, \
            "{\"PWR\":\"1\",\"APW\":\"%d\",\"OUT\":\"%d\",\"SPD\":\"%d\",\"HTE\":\"%d\",\"TMS\":\"%d\"", \
            BrizerAutoStart, BrizerOutsideTemp, BrizerSpeed, BrizerHeatStateSetting, BrizerTimerState);
    if(BrizerHeatState) {
      strcat(OutputSerialBuffer, ",\"HTT\":\"");
      sprintf(Convert_Buffer, "%d\"", BrizerHeatTemp);
      strcat(OutputSerialBuffer, Convert_Buffer);
    }
    if(BrizerFilterTime < 31) {
      strcat(OutputSerialBuffer, ",\"FLT\":\"");
      sprintf(Convert_Buffer, "%d\"", BrizerFilterTime);
      strcat(OutputSerialBuffer, Convert_Buffer);
    }
    strcat(OutputSerialBuffer, "}\r");
    FlagChangeValues = 0;
  }
  Serial.print(OutputSerialBuffer);
  Serial.flush();
  Cicles = 0;
}

void sendExtStatus() {
  uint8_t T;
  uint8_t Screen = switchToSetupScreen(5);
  if (Screen == 0) {
    FlagChangeValues = 1;
    return;
  }
  T = BrizerTimerState;
  if (T == 0) pressKey(B10, SHORTPRESS);
  switchToSetupScreen(9);
  if (T == 0) {
    switchToSetupScreen(5);
    pressKey(B10, SHORTPRESS);
  }
  pressKey(B1000, LONGPRESS);
  sprintf(OutputSerialBuffer, \
            "{\"MNT\":\"%d\",\"FLT\":\"%d\",\"TMS\":\"%d\",\"TME\":\"%02d%02d\",\"TMD\":\"%02d%02d\",\"TIM\":\"%02d%02d\"}\r", \
            BrizerMinOutTemp, BrizerFilterTime, T, BrizerTimerOnHR, BrizerTimerOnMIN, BrizerTimerOffHR, BrizerTimerOffMIN, BrizerTimeHR, BrizerTimeMIN);
  Serial.print(OutputSerialBuffer);
  Serial.flush();
  FlagChangeValues = 1;
  if (Screen == 2) pressKey(1, SHORTPRESS);
}

int16_t *strToInt(char *s) {
  static int16_t temp;
  temp = 0; // число
  uint8_t i = 0;
  uint8_t sign = 0; // знак числа 0 — положительное, 1 — отрицательное
  if (s[i] == '-')
  {
    sign = 1;
    i++;
  }
  while (s[i] >= 0x30 && s[i] <= 0x39 && i < 4)
  {
    temp = temp * 10;
    temp = temp + (s[i] & 0x0F);
    i++;
  }
  if (sign == 1)
    temp = -temp;
  return(&temp);
}
