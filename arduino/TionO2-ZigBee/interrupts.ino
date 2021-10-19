void setupInterupts() {
    // Timer2 Setup
  TCCR2A = 0;
  TCCR2B = B00000111;
  TCNT2 = 0;
  TIMSK2 = 0;                                  // Disable interrupt

  // External interupts
  EICRA = B00001110;                                    // INT1 rising adge and INT0 fail edge
}

ISR(TIMER2_OVF_vect) {
  if (StartRead == 1) {
    TCNT2 = STARTREADTIMEOUT;
    ReadTimer++;
    if (ReadTimer > 55) {
      PORTC &= B11111110;
      TIMSK2 = 0;
      EIMSK = 0;
      Active = 0;
      StartRead = 0;
      NewData = 0;
    }
    return;
  }
  Active = 0;
  PORTC &= B11111110;
  TIMSK2 = 0;
  EIMSK = 0;                                   // Disable INT0 and INT1
}

ISR(INT0_vect) {
  if (StartRead == 1) {
    if (ReadTimer > 0) {
      StartRead = 0;
      EIFR = B00000010;
      EIMSK = B00000011;
      return;
    }
    TCNT2 = STARTREADTIMEOUT;
    TIFR2 = 1;
    return;
  }
  ReadBufferPointer++;;
  ReadBitPointer = 0;
  ReadBufferPointer &= READBUFFERSIZE - 1;
}

ISR(INT1_vect) {
  Input = PIND;
  NewData = 1;
  TCNT2 = 220;
  if (StartRead == 0) {
    ReadBuffer[ReadBufferPointer] <<= 1;
    ReadBuffer[ReadBufferPointer] |= (Input>>4)&B00000001;
    ReadBitPointer++;
    ReadBufferPointer += (ReadBitPointer>>3);
    ReadBitPointer &= B00000111;
    ReadBufferPointer &= READBUFFERSIZE - 1 ;
  }
}
