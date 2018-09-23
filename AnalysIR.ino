//* AnalysIR Arduino firmware
//* Date: 9th September 2013
//* Release: 0.9.x
//* Note: Please use and read in conjunction with the READMEand the AnalysIR Getting Started Guide.
//* Licence: free to use & modify without restriction or warranty.
//*
//* Tested with Arduino IDE v1.05 only.
#ifndef NODO_DUE // Nodo Due just uses the ReportPulses code
#define RKR_MODIFICATIONS
#ifdef RKR_MODIFICATIONS
#undef ENABLE_MODULATION	// Rinie: disable to save memory
#define RKR_MIN_PULSE_DURATION	100
#define RKR_MIN_PULSE_COUNT		16
#define RKR_ALWAYS_START_WITH_MARK
#define PULSE_TIMEOUT	8000
#undef RKR_DEBUG
#else //disable rinie modifications
#define ENABLE_MODULATION
#define RKR_MIN_PULSE_DURATION	0
#define RKR_MIN_PULSE_COUNT		0
#undef RKR_ALWAYS_START_WITH_MARK
#define PULSE_TIMEOUT	8000
#endif

//see *** note 1 in README
#define enableIRrx attachInterrupt(0, rxIR_Interrupt_Handler, CHANGE) //set up interrupt handler for IR rx   on pin 2 - demodulated signal

#ifdef ENABLE_MODULATION
#define enableIRrxMOD attachInterrupt(1, rxIR3_Interrupt_Handler, FALLING); //Same for Pin3 - modulated signal
#endif

#
//set true for Arduino being used, all others to false
#define Arduino328 true   //Any standard Arduino - UNO, Duemilenova, Nano or compatible with ATMega328x @16Mhz
#define ArduinoLeonardo false
#define ArduinoMega1280 false    //Also Mega2560 - untested
#define ArduinoDUE false        //placeholder for future
#define ArduinoYUN false        //placeholder for future

//Definitions for Buffers here...size RAM dependent
#define maxPULSES 700 //increase until memory is used up.
#ifdef ENABLE_MODULATION
#define modPULSES 256 //increase until memory is used up, max 256, leave at 256.
#endif

//see *** note 2 in README
#if Arduino328
#define pin2HIGH (PIND & 0b00000100) //Port D bit 2
#elif ArduinoLeonardo
#define pin2HIGH (PIND & 0b00000010) //Port D bit 1
//Following must be redefined as interrupt pins are different on Leonardo
#define enableIRrx attachInterrupt(1, rxIR_Interrupt_Handler, CHANGE) //set up interrupt handler for IR rx   on pin 2 - demodulated signal
#define enableIRrxMOD attachInterrupt(0, rxIR3_Interrupt_Handler, FALLING); //Same for Pin3 - modulated signal
#define maxPULSES 906 //More RAM is available on Leonardo
#elif ArduinoMega1280
#define pin2HIGH (PINE & 0b00010000) //Port E bit 4
#define maxPULSES 1024 //even More RAM is available on Megas...can increase this for longer signals if neccessary
#endif

//see *** note 3 in README
uint16_t pulseIR[maxPULSES]; //temp store for pulse durations (demodulated)
#ifdef ENABLE_MODULATION
volatile byte modIR[modPULSES]; //temp store for modulation pulse durations - changed in interrupt
#endif

//see *** note 4 in README
//Pin definitions
const byte IR_Rx_PIN = 2;//IR digital input pin definition - drives interrupt
#ifdef ENABLE_MODULATION
const byte IR_Mod_PIN = 3;//IR digital Raw Modulation input pin definition - drives interrupt
#endif
const byte ledPin = 13;//used to signal waiting for serial COM port, when using Leonardo

//General variables
//see *** note 5 in README
volatile byte state = 127; //defines initial value for state normally HIGH or LOW. Set in ISR.
byte oldState = 127; //set both the same to start. Used to test for change in state signalled from ISR
unsigned long usLoop, oldTime; //timer values in uSecs. usLoops stores the time value of each loop, oldTime remembers the last time a state change was received
volatile unsigned long newMicros;//passes the time a state change occurred from ISR to main loop
unsigned long oldMicros;//passes the time a state change occurred from ISR to main loop
unsigned int countD = 0; // used as a pointer through the buffers for writing and reading (de-modulated signal)
unsigned int i = 0; //used to iterate in for loop...integer
#ifdef ENABLE_MODULATION
volatile byte countM = 0; // used as a pointer through the buffers for writing and reading (modulated signal)
byte countM2 = 0; //used as a counter, for the number of modulation samples used in calculating the period.
byte j = 0; //used to iterate in for loop..byte
unsigned long sum = 0; //used in calculating Modulation frequency
byte sigLen = 0; //used when calculating the modulation period. Only a byte is required.
#endif
#endif // NODO_DUE
//Serial Tx buffer - uses Serial.write for faster execution
//see *** note 6 in README
byte txBuffer[5];//Key(+-)/1,count/1,offset/4,CR/1

#ifndef NODO_DUE // Nodo Due just uses the ReportPulses code
//see *** note 7 in README
void setup() {

  Serial.begin(2000000);//non-standard Baud Rate - as fast as possible (Use X-CTU to debug)
  delay(1000);//to avoid potential conflict with boot-loader on some systems
  txBuffer[4] = 13; //init ascii decimal value for CR in tx buffer

  pinMode(IR_Rx_PIN, INPUT);
#ifdef ENABLE_MODULATION
  pinMode(IR_Mod_PIN, INPUT);
#endif
  pinMode(ledPin, OUTPUT);

  digitalWrite(ledPin, HIGH); //if LED stays on after reset, then serial not recognised PC
  while (!Serial); //for Leonardo
  digitalWrite(ledPin, LOW);

  Serial.println(F("!AnalysIR!")); // HELLO STRING - ALL COMMENTS SENT IN !....! FORMAT

  //Initialise State
  oldState = digitalRead(IR_Rx_PIN);
  state = oldState;

  //Initialise Times
  oldTime = 0; //init
  newMicros == micros(); //init
  oldMicros = newMicros;

  //following line not required - just report free RAM on Arduino if there are problems
  reportFreeRAM(0xFFFF);// report free ram to host always, use max UInt value of 0xFFFF
  //

  //turn on interrupts and GO!
  enableIRrx;//set up interrupt handler for IR rx on pin 2 - demodulated signal
#ifdef ENABLE_MODULATION
  enableIRrxMOD; //set up interrupt handler for modulated IR rx on pin 3 - full modulated signal
#endif

}

//see *** note 8 in README
void loop() {
  while (true) { //avoid extra stuff inserted by Arduino IDE at end of each normal Loop
    usLoop = micros(); //used once every loop rather than multiple calls
    //see *** note 9 in README
    if (oldState != state && countD < maxPULSES) {
      oldState = state;
      if (oldState) pulseIR[countD++] = (newMicros - oldMicros) | 0x0001; //store for later & include state
      else pulseIR[countD++] = (newMicros - oldMicros) & 0xFFFE; //store for later & include state
      oldMicros = newMicros; //remember for next time
      oldTime = usLoop; //last time IR was received
    }

    //see *** note 10 in README
    //NB:*** Xsat has a header space of 4000 uSecs so lets allow 5000 to signify new signal
    //sequence gaps are handled by AnalysIR app
    //note: some protocols like RECS80 have space duration of circa 7500
    if (state && countD > 0 && (countD == maxPULSES ||  (usLoop - oldTime) > PULSE_TIMEOUT)) { //if we have receive 50*2 =100 pulses or its 10ms since last one
      reportPulses();
#ifdef ENABLE_MODULATION
      reportPeriod();//reports modulation frequency to host over serial
#endif
      countD = 0; //reset value for next time
    }

    //see *** note 11 in README
    /*  this code only used if we are having problem
     reportFreeRAM(200);//report freeram as comment to host if less than 200 bytes
     */
  }
}
#endif // NODO_DUE

#ifndef NODO_DUE // Nodo Due just uses the ReportPulses code
//see *** note 12 in README
void  reportPulses() {
#ifdef RKR_ALWAYS_START_WITH_MARK
	unsigned int j = 0; //used to iterate in for loop...integer
#endif
		// OOK 433 spike and noise suppression
	if (countD < RKR_MIN_PULSE_COUNT){
		#ifdef RKR_DEBUG
	    	Serial.print(F("!RKR_MIN_PULSE_COUNT!"));//send line as comment
	    #endif
		return;
	}
	if 	(pulseIR[0] < RKR_MIN_PULSE_DURATION) {
		#ifdef RKR_DEBUG
	    	Serial.print(F("!RKR_MIN_PULSE_DURATION!"));//send line as comment
	    #endif
		return;
	}
	i = 0;
	if (pulseIR[0] > PULSE_TIMEOUT) {
		if 	(pulseIR[1] < RKR_MIN_PULSE_DURATION) {
			#ifdef RKR_DEBUG
				Serial.print(F("!RKR_MIN_PULSE_DURATION2!"));//send line as comment
			#endif
			return;
		}
#ifdef RKR_ALWAYS_START_WITH_MARK
		j = 1;
		#ifdef RKR_DEBUG
			Serial.print(F("!RKR_ALWAYS_START_WITH_MARK2!"));//send line as comment
		#endif
#endif
	}
  for (; i < countD; i++) {
#ifdef RKR_ALWAYS_START_WITH_MARK
	txBuffer[0] = ((i+ j) & 1) ? '-' : '+';
#else
    //the following logic takes care of the inverted signal in the IR receiver
    if (pulseIR[i] & 0x01) txBuffer[0] = '+'; //Mark is sent as +...LSB bit of pulseIR is State(Mark or Space)
    else txBuffer[0] = '-';           //Space is sent as -
#endif
    txBuffer[1] = i; //count
    txBuffer[3] = pulseIR[i] >> 8; //byte 1
    txBuffer[2] = pulseIR[i] & 0xFE; //LSB 0 ..remove lat bit as it was State
    Serial.write(txBuffer, 5);
  }
}
#endif // NODO_DUE
#ifdef NODO_DUE // Nodo Due just uses the ReportPulses code
#define AVR_LIRC_BINARY // undef for text debugging

void  reportPulses(uint xStart, uint xEnd) {
  txBuffer[4] = 13; //init ascii decimal value for CR in tx buffer
    word len;
    byte high = 1;
    byte overflow = 0;
	byte had_overflow = 1;
#ifndef AVR_LIRC_BINARY
	Serial.println("!AnalysIr!");
#else
	Serial.println(F("!NodoDueRkr!"));
#endif
	//return;
	for(uint x=xStart;x<=xEnd;x++, high = !high) {
		word pulseIR = PulseSpaceMicros(x);
		len = pulseIR;
		overflow = had_overflow;
    if (high) txBuffer[0] = '+'; //Mark is sent as +...LSB bit of pulseIR is State(Mark or Space)
    else txBuffer[0] = '-';           //Space is sent as -
    txBuffer[1] = x-xStart; //count
    txBuffer[3] = pulseIR >> 8; //byte 1
    txBuffer[2] = pulseIR & 0xFF; // tja
#ifndef AVR_LIRC_BINARY // debug in text
	    PrintChar(txBuffer[0]);
	    PrintNum(x-xStart,',',2);
	    PrintNum(pulseIR, ',', 4);
	    PrintTerm();
#else
    Serial.write(txBuffer, 5);
#endif
  }
}
#endif // NODO_DUE

#ifndef NODO_DUE // Nodo Due just uses the ReportPulses code
//see *** note 13 in README
#ifdef ENABLE_MODULATION
void  reportPeriod() { //report period of modulation frequency in nano seconds for more accuracy
  sum = 0; // UL
  sigLen = 0; //byte
  countM2 = 0; //byte

  for (j = 1; j < (modPULSES - 1); j++) { //i is byte
    sigLen = (modIR[j] - modIR[j - 1]); //siglen is byte
    if (sigLen > 50 || sigLen < 10) continue; //this is the period range length exclude extraneous ones
    sum += sigLen; // sum is UL
    countM2++; //countM2 is byte
    modIR[j - 1] = 0; //finished with it so clear for next time
  }
  modIR[j - 1] = 0; //now clear last one, which was missed in loop

  if (countM2 == 0) return; //avoid div by zero = nothing to report
  sum =  sum * 1000 / countM2; //get it in nano secs
  // now send over serial using buffer
  txBuffer[0] = 'M'; //Modulation report is sent as 'M'
  txBuffer[1] = countM2; //number of samples used
  txBuffer[3] = sum >> 8 & 0xFF; //byte Period MSB
  txBuffer[2] = sum & 0xFF; //byte Period LSB
  Serial.write(txBuffer, 5);
  return;
}
#endif

//see *** note 14 in README
void rxIR_Interrupt_Handler() { //important to use few instruction cycles here
  //digital pin 2 on Arduino
  newMicros = micros(); //record time stamp for main loop
  state = pin2HIGH; //read changed state of interrupt pin 2 (return 4 or 0 for High/Low)
}

//see *** note 15 in README
#ifdef ENABLE_MODULATION
void rxIR3_Interrupt_Handler() { //important to use few instruction cycles here
  //digital pin 3 on Arduino - FALLING edge only
  modIR[countM++] = micros(); //just continually record the time-stamp, will be mostly modulations
  //just save LSB as we are measuring values of 20-50 uSecs only - so only need a byte (LSB)
}
#endif
#endif // NODO_DUE
//see *** note 16 in README
void reportFreeRAM(unsigned int f) { //report freeRam to host if less than f or f is false
  extern int __heap_start, *__brkval;
  int v;
  int freeRAM = (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
  if (freeRAM <= f) {
#ifndef NODO_DUE // Nodo Due just uses the ReportPulses code
    Serial.print(F("!Free RAM: "));//send line as comment
    Serial.print(freeRAM);
    Serial.println(F("!"));//send line as comment
#else
    PrintStartRaw(F("Free RAM: "));//send line as comment
    Serial.print(freeRAM);
    PrintTermRaw();//send line as comment
#endif
  }
}
