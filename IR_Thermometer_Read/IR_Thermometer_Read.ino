

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

int timer1_counter;
volatile byte _comState;
volatile byte _displayScan[4];
volatile byte _scanComplete = 0;
volatile byte _triggeredCounter = 0;

byte digitA, digitB, digitC, digitD;

void setup() {
  noInterrupts();  
  Serial.begin(9600);
  // set ADC prescale to 16 to speed up the read speed.
  sbi(ADCSRA,ADPS2) ;
  cbi(ADCSRA,ADPS1) ;
  cbi(ADCSRA,ADPS0) ;
  // Comparator Pins
  pinMode(7,INPUT);
  pinMode(6, INPUT);
  // comparator interrupt enabled and tripped on falling edge, 
  // which is the rising edge of the input signal.
  ACSR = B00011010; 

  // Timer setup
  TCCR1A = 0;
  TCCR1B = 0;
  timer1_counter = 61453;
  TCCR1B |= (1 << CS11);    // 8 prescaler 
  
  interrupts(); 
}

int convertDigit(byte digit)
{
  if(digit == 0x80) return -1;
  if(digit == 0x7E) return 0;
  if(digit == 0x18) return 1;
  if(digit == 0xB6) return 2;
  if(digit == 0xBC) return 3;
  if(digit == 0xD8) return 4;
  if(digit == 0xEC) return 5;
  if(digit == 0xEE) return 6;
  if(digit == 0x38) return 7;
  if(digit == 0xFE) return 8;
  if(digit == 0xFC) return 9;
  if(digit == 0x00) return -1000;
  return -2000;
}

int generateOutput()
{
  int converted;

  converted = convertDigit(digitD);
  if(converted < 0) return -2001; // Error
  int output = converted;
  
  converted = convertDigit(digitC);
  if(converted < 0) return -2002; // Maybe an error???
  output += converted * 10;
  
  converted = convertDigit(digitB);
  if(converted == -1){
    return output * -1;
  }else if(converted == -1000){
    return output; 
  } else if(converted == -2000){
    return -2003;
  } else {
    output += converted * 100;
  }

  converted = convertDigit(digitA);
  if(converted == -1){
    return output * -1;
  }else if(converted == -1000){
    return output; 
  } else if(converted == -2000){
    return -2004;
  } else {
    output += converted * 1000;
  }

  return output;
}

void sortDigits()
{
  digitA = digitB = digitC = digitD = 0;
  if(_displayScan[0] & (1<<0)) digitD |= (1<<5);
  if(_displayScan[0] & (1<<2)) digitC |= (1<<5);
  if(_displayScan[0] & (1<<4)) digitB |= (1<<5);
  if(_displayScan[0] & (1<<6)) digitA |= (1<<5);

  if(_displayScan[1] & (1<<0)) digitD |= (1<<4);
  if(_displayScan[1] & (1<<1)) digitD |= (1<<6);
  if(_displayScan[1] & (1<<2)) digitC |= (1<<4);
  if(_displayScan[1] & (1<<3)) digitC |= (1<<6);
  if(_displayScan[1] & (1<<4)) digitB |= (1<<4);
  if(_displayScan[1] & (1<<5)) digitB |= (1<<6);
  if(_displayScan[1] & (1<<6)) digitA |= (1<<4);
  if(_displayScan[1] & (1<<7)) digitA |= (1<<6);

  if(_displayScan[2] & (1<<0)) digitD |= (1<<7);
  if(_displayScan[2] & (1<<1)) digitD |= (1<<1);
  if(_displayScan[2] & (1<<2)) digitC |= (1<<7);
  if(_displayScan[2] & (1<<3)) digitC |= (1<<1);
  if(_displayScan[2] & (1<<4)) digitB |= (1<<7);
  if(_displayScan[2] & (1<<5)) digitB |= (1<<1);
  if(_displayScan[2] & (1<<6)) digitA |= (1<<7);
  if(_displayScan[2] & (1<<7)) digitA |= (1<<1);

  if(_displayScan[3] & (1<<0)) digitD |= (1<<3);
  if(_displayScan[3] & (1<<1)) digitD |= (1<<2);
  if(_displayScan[3] & (1<<2)) digitC |= (1<<3);
  if(_displayScan[3] & (1<<3)) digitC |= (1<<2);
  if(_displayScan[3] & (1<<4)) digitB |= (1<<3);
  if(_displayScan[3] & (1<<5)) digitB |= (1<<2);
  if(_displayScan[3] & (1<<6)) digitA |= (1<<3);
  if(_displayScan[3] & (1<<7)) digitA |= (1<<2);
}

void scanInputs()
{
    byte segments = 0;
    if(analogRead(A0) < 155) segments |= (1 << 0);
    if(analogRead(A1) < 155) segments |= (1 << 1);
    if(analogRead(A2) < 155) segments |= (1 << 2);
    if(analogRead(A3) < 155) segments |= (1 << 3);
    if(analogRead(A4) < 155) segments |= (1 << 4);
    if(analogRead(A5) < 155) segments |= (1 << 5);
    if(analogRead(A6) < 155) segments |= (1 << 6);
    if(analogRead(A7) < 155) segments |= (1 << 7);

    _displayScan[_comState] = segments;
    _comState++;
}

ISR(TIMER1_OVF_vect)
{
  TCNT1 = timer1_counter;   // preload timer

  delayMicroseconds(100);
  scanInputs();
  
  if(_comState > 3)
  {
    TIMSK1 &= ~(1 << TOIE1);   // disable timer overflow interrupt
    _scanComplete = 1;
  }
}


ISR(ANALOG_COMP_vect)
{
    
    // Discard the first 5 triggers, as on power up the display has some odd behaviour.
    if(_triggeredCounter++ < 5) return;
    _triggeredCounter = 0;
    
    // Setup timer for the next set
    TCNT1 = timer1_counter;
    TIFR1 |= (1 << TOV1); // Clear any pending interrupt
    TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
    _comState = 0;

    delayMicroseconds(100);
    
    scanInputs();

    // Disable the analog comparator until this result has been processed
    ACSR &= ~(1<<ACIE);
}

void loop() {
  if(_scanComplete)
  {
    _scanComplete = 0;

    sortDigits();
    Serial.println(_displayScan[0], HEX);
    Serial.println(_displayScan[1], HEX);
    Serial.println(_displayScan[2], HEX);
    Serial.println(_displayScan[3], HEX);

    Serial.println("out:");
    Serial.println(generateOutput());
    Serial.println("---");
    delay(1000);

    //Clear any pending comparator interrupt
    ACSR |= (1<<ACI);
    // Enable the interrupt
    ACSR |= (1<<ACIE);
  }
}



