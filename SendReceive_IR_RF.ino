 /**************************************************************************
    This file is part of Nodo Due, (c) Copyright Paul Tonkes

    Nodo Due is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Nodo Due is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Nodo Due.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

/**********************************************************************************************
 * Opwekken draaggolf van 38Khz voor verzenden IR.
 * Deze code is gesloopt uit de library FrequencyTimer2.h omdat deze library niet meer door de compiler versie 0015 kwam.
 * Tevens volledig uitgekleed.
 *********************************************************************************************/
static uint8_t enabled = 0;
static void IR38Khz_set() {
  uint8_t pre, top;
  ulong period=208; // IR_TransmitCarrier=26 want pulsen van de IR-led op een draaggolf van 38Khz. (1000000/38000=26uSec.) Vervolgens period=IR_TransmitCarrier*clockCyclesPerMicrosecond())/2;  // period =208 bij 38Khz
  pre=1;
  top=period-1;
  TCCR2B=0;
  TCCR2A=0;
  TCNT2=0;
  ASSR&=~_BV(AS2);    // use clock, not T2 pin
  OCR2A=top;
  TCCR2A=(_BV(WGM21)|(enabled?_BV(COM2A0):0));
  TCCR2B=pre;
}

/**********************************************************************************************
 * Deze functie wacht totdat de 433 band vrij is of er een timeout heeft plaats gevonden
 * Window en delay tijd in milliseconden
 *********************************************************************************************/
#define WAITFREERF_TIMEOUT             30000 // tijd in ms. waarna het wachten wordt afgebroken als er geen ruimte in de vrije ether komt

void WaitFreeRF(int Delay, int Window) {
  ulong Timer, TimeOutTimer;

  // eerst de 'dode' wachttijd
  Timer=millis()+Delay; // set de timer.
  while(Timer>millis())
    digitalWrite(MonitorLedPin,(millis()>>7)&0x01);

  // dan kijken of de ether vrij is.
  Timer=millis()+Window; // reset de timer.
  TimeOutTimer=millis()+WAITFREERF_TIMEOUT; // tijd waarna de routine wordt afgebroken in milliseconden

  while(Timer>millis() && TimeOutTimer>millis())
    {
    if((*portInputRegister(RFport)&RFbit)==RFbit)// Kijk if er iets op de RF poort binnenkomt. (Pin=HOOG als signaal in de ether).
      {
      if(FetchSignal(RF_ReceiveDataPin,HIGH,SIGNAL_TIMEOUT_RF))// Als het een duidelijk signaal was
        Timer=millis()+Window; // reset de timer weer.
      }
    digitalWrite(MonitorLedPin,(millis()>>7)&0x01);
    }
 }

/*********************************************************************************************
 * Deze routine zendt een RAW code via RF.
 * De inhoud van de buffer pulseSpaceMicros moet de pulstijden bevatten.
 * PsCount(0) het aantal pulsen*2
 *********************************************************************************************/
void RawSendRF(void) {
  int x;

  digitalWrite(RF_ReceivePowerPin,LOW);   // Spanning naar de RF ontvanger uit om interferentie met de zender te voorkomen.
  digitalWrite(RF_TransmitPowerPin,HIGH); // zet de 433Mhz zender aan
  delay(5);// kleine pause om de zender de tijd te geven om stabiel te worden

  for(byte y=0; y<settings.TransmitRepeat; y++) // herhaal verzenden RF code
    {
    x=1;
    while(x<=PsCount(0))
      {
      digitalWrite(RF_TransmitDataPin,HIGH); // 1
      delayMicroseconds(PulseSpaceMicros(x++));
      digitalWrite(RF_TransmitDataPin,LOW); // 0
      delayMicroseconds(PulseSpaceMicros(x++));
      }
    }
  digitalWrite(RF_TransmitPowerPin,LOW); // zet de 433Mhz zender weer uit
  digitalWrite(RF_ReceivePowerPin,HIGH); // Spanning naar de RF ontvanger weer aan.
}

/*********************************************************************************************
 * Deze routine zendt een 32-bits code via IR.
 * De inhoud van de buffer pulseSpaceMicros moet de pulstijden bevatten.
 * PsCount(0) het aantal pulsen*2
 * Pulsen worden verzonden op en draaggolf van 38Khz.
 *********************************************************************************************/
void RawSendIR(void)
{
  int x,y;


  for(y=0; y<settings.TransmitRepeat; y++) // herhaal verzenden IR code
    {
    x=1;
    while(x<=PsCount(0))
      {
      TCCR2A|=_BV(COM2A0); // zet IR-modulatie AAN
      delayMicroseconds(PulseSpaceMicros(x++));
      TCCR2A&=~_BV(COM2A0); // zet IR-modulatie UIT
      delayMicroseconds(PulseSpaceMicros(x++));
      }
    }
}

/**********************************************************************************************
 * Wacht totdat de pin verandert naar status state. Geeft de tijd in uSec. terug.
 * Als geen verandering, dan wordt na timeout teruggekeerd met de waarde 0L
 *********************************************************************************************/
ulong WaitForChangeState(uint8_t pin, uint8_t state, ulong timeout) {
	uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);
	uint8_t stateMask = (state ? bit : 0);

	ulong numloops = 0; // keep initialization out of time critical area
	ulong maxloops = microsecondsToClockCycles(timeout) / 19;

	// wait for the pulse to stop. One loop takes 19 clock-cycles
	while((*portInputRegister(port) & bit) == stateMask) {
		if (numloops++ == maxloops) {
			return 0;//timeout opgetreden
		}
	}
	return clockCyclesToMicroseconds(numloops * 19 + 16);
}

/**********************************************************************************************
 * Haal de pulsen en plaats in buffer. Op het moment hier aangekomen is de startbit actief.
 * bij de TSOP1738 is in rust is de uitgang hoog. StateSignal moet LOW zijn
 * bij de 433RX is in rust is de uitgang laag. StateSignal moet HIGH zijn
 *
 * RKR: Added uint psmIndexStart: receive repeated signals in one go
 * 2020: integrate processBitRkr/psiNibbles[psiCount++] = psNibbleIndex(lastPulseDur, pulse_dur);
 * Logic.
 * space (no signal) timeout is relevant.
 * pulse signal may be long (real signal).
 *********************************************************************************************/
int FetchSignal(byte DataPin, boolean StateSignal, ulong TimeOut) {
	pstime firstPulseDur;
	pstime firstSpaceDur;
	pstime maxSpaceDur = 0;
	if (psiCount + MIN_RAW_PULSES >= NRELEMENTS(psiNibbles)) {
		return 0;
	}
	uint psiCountStart = psiCount++; // save for first Pulse/Space
	pstime PulseLength;
	pstime SpaceLength;
	ulong PulseTimeOut = 2 * TimeOut;

	// support for long preamble start Pulse
	PulseLength = WaitForChangeState(DataPin, StateSignal, PulseTimeOut); // meet hoe lang signaal LOW (= PULSE van IR signaal)
	// Start space
	SpaceLength = WaitForChangeState(DataPin, !StateSignal, TimeOut); // meet hoe lang signaal HIGH (= SPACE van IR signaal)

	if (PulseLength < MIN_PULSE_LENGTH || SpaceLength == 0) {
		return 0;
	}
	firstPulseDur = PulseLength;
	firstSpaceDur = SpaceLength;

	// Original code
	do { // lees de pulsen in microseconden en plaats deze in een tijdelijke buffer
		// pulse
		PulseLength = WaitForChangeState(DataPin, StateSignal, PulseTimeOut); // meet hoe lang signaal LOW (= PULSE van IR signaal)
		// space
		SpaceLength = WaitForChangeState(DataPin, !StateSignal, TimeOut); // meet hoe lang signaal HIGH (= SPACE van IR signaal)

		if (psiCount >= NRELEMENTS(psiNibbles)) {
			break;
		}
		if (PulseLength < MIN_PULSE_LENGTH || SpaceLength < MIN_SPACE_LENGTH) { // last space should be timeout...
			break;
		}
		psiNibbles[psiCount++] = psNibbleIndex(PulseLength, SpaceLength);
	}
	while (true);// Zolang nog niet alle bits ontvangen en er niet vroegtijdig een timeout plaats vindt
	// start as last for short first pulse...
	psiNibbles[psiCountStart] = psNibbleIndex(firstPulseDur, firstSpaceDur);
	if ((psiCount < NRELEMENTS(psiNibbles)) && (PulseLength >= MIN_PULSE_LENGTH)) { // last Trailer space
		SpaceLength = TimeOut + WaitForChangeState(DataPin, !StateSignal, 8 * TimeOut); // meet hoe lang signaal HIGH (= SPACE van IR signaal)
		psiNibbles[psiCount++] = psNibbleIndex(PulseLength, SpaceLength);
	}

	uint psmCount = (psiCount - psiCountStart) * 2;
	if (psmCount>MIN_RAW_PULSES && (psiCount < NRELEMENTS(psiNibbles))) {
		return psmCount;
	}
	psiCount = psiCountStart;
	//PsCountSetS(psmIndexStart, 0, pscsInterTimeout);
	return 0;
}

/**********************************************************************************************
 * Deze functie zendt gedurende Window seconden de IR ontvangst direct door naar RF
 * Window tijd in seconden.
 *********************************************************************************************/
void CopySignalIR2RF(byte Window)
  {
  ulong Timer=millis()+((ulong)Window)*1000; // reset de timer.

  digitalWrite(RF_ReceivePowerPin,LOW);   // Spanning naar de RF ontvanger uit om interferentie met de zender te voorkomen.
  digitalWrite(RF_TransmitPowerPin,HIGH); // zet de 433Mhz zender aan
  while(Timer>millis())
    {
    digitalWrite(RF_TransmitDataPin,(*portInputRegister(IRport)&IRbit)==0);// Kijk if er iets op de IR poort binnenkomt. (Pin=LAAG als signaal in de ether).
    digitalWrite(MonitorLedPin,(millis()>>7)&0x01);
    }
  digitalWrite(RF_TransmitPowerPin,LOW); // zet de 433Mhz zender weer uit
  digitalWrite(RF_ReceivePowerPin,HIGH); // Spanning naar de RF ontvanger weer aan.
  }

/**********************************************************************************************
 * Deze functie zendt gedurende Window seconden de RF ontvangst direct door naar IR
 * Window tijd in seconden.
 *********************************************************************************************/
#define MAXPULSETIME 50 // maximale zendtijd van de IR-LED in mSec. Ter voorkoming van overbelasting
void CopySignalRF2IR(byte Window)
{
  ulong Timer=millis()+((ulong)Window)*1000; // reset de timer.
  ulong PulseTimer;

  while(Timer>millis())// voor de duur van het opgegeven tijdframe
    {
    while((*portInputRegister(RFport)&RFbit)==RFbit)// Zolang de RF-pulse duurt. (Pin=HOOG bij puls, laag bij SPACE).
      {
      if(PulseTimer>millis())// als de maximale zendtijd van IR nog niet verstreken
        {
        digitalWrite(MonitorLedPin,HIGH);
        TCCR2A|=_BV(COM2A0);  // zet IR-modulatie AAN
        }
      else // zendtijd IR voorbij, zet IR uit.
        {
        digitalWrite(MonitorLedPin,LOW);
        TCCR2A&=~_BV(COM2A0); // zet IR-modulatie UIT
        }
      }
    PulseTimer=millis()+MAXPULSETIME;
    }
  digitalWrite(MonitorLedPin,LOW);
  TCCR2A&=~_BV(COM2A0);
}
