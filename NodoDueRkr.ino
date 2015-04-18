/*
 * NodoDueRkr IR and OOK RF receival
 * Based on Nodo Due 1.2.1 by Paul Tonkes
 *
 * (c) 2011-2015 Rinie Kervel
 * Use Nodo Due as a Nice base for pulseSpaceMicros analysis of pulse/space signals
 * but rip out event processing and other parts I do not need
 * Adapt I/O for LIRC serial and AnalysIR
 * Experiment with dynamic timing based on received signals independant of protocols
 *
 * Original Nodo Due copyrights below
 */

 /****************************************************************************************************************************\
 * Arduino project "Nodo Due" © Copyright 2010 Paul Tonkes
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You received a copy of the GNU General Public License
 * along with this program in tab '_COPYING'.
 *
 * voor toelichting op de licentievoorwaarden zie    : http://www.gnu.org/licenses
 * Voor discussie: Zie Logitech Harmony forum        : http://groups.google.com/group/arduinodo/topics
 * Uitgebreide documentatie is te vinden op          : http://www.nodo-domotica.nl
 * bugs kunnen worden gelogd op                      : https://code.google.com/p/arduino-nodo/
 * Compiler voor deze programmacode te downloaden op : http://arduino.cc
 * Voor vragen of suggesties, mail naar              : p.k.tonkes@gmail.com
 * Compiler                                          : - Arduino Compiler 0022
 * Hardware                                          : - Arduino UNO, Duemilanove of Nano met een ATMeg328 processor @16Mhz.
 *                                                     - Hardware en Arduino penbezetting volgens schema Nodo Due Rev.003
 \****************************************************************************************************************************/

#define VERSION        003        // Nodo Version nummer:
                                  // Major.Minor.Patch
                                  // Major: Grote veranderingen aan concept, besturing, werking.
                                  // Minor: Uitbreiding/aanpassing van commando's, functionaliteit en MMI aanpassingen
                                  // Patch: Herstel van bugs zonder (ingrijpende) functionele veranderingen.

#include "pins_arduino.h"
#include <EEPROM.h>
#include <Wire.h>
#include <avr/pgmspace.h>
#include "NodoDueRkr.h"

#if 1
#define VALUE_OFF 0
#define VALUE_ON 1 // Deze waarde MOET groter dan 16 zijn.
#define VALUE_SOURCE_IR 2
#define VALUE_SOURCE_IR_RF 3
#define VALUE_SOURCE_RF 4
#define VALUE_DIRECTION_INPUT 5
#define VALUE_DIRECTION_OUTPUT 6
#define CMD_KAKU 92
#define CMD_KAKU_NEW 93
const char Cmd_0[] PROGMEM ="Off";
const char Cmd_1[] PROGMEM ="On";
const char Cmd_2[] PROGMEM="IR";
const char Cmd_3[] PROGMEM="IR&RF";
const char Cmd_4[] PROGMEM="RF";
const char Cmd_5[] PROGMEM="Input";
const char Cmd_6[] PROGMEM="Output";
// tabel die refereert aan de commando strings
PGM_P const CommandText_tabel[]={
  Cmd_0 ,Cmd_1 ,Cmd_2 ,Cmd_3 ,Cmd_4 ,Cmd_5 ,Cmd_6};
#define COMMAND_MAX 7 // aantal commando's (dus geteld vanaf 0)
#endif


// Declaratie aansluitingen I/O-pennen op de Arduino
// D0 en D1 kunnen niet worden gebruikt. In gebruik door de FTDI-chip voor seriele USB-communiatie (TX/RX).
// A4 en A5 worden gebruikt voor I2C communicatie voor o.a. de real-time clock
#define IR_ReceiveDataPin           3  // Op deze input komt het IR signaal binnen van de TSOP. Bij HIGH bij geen signaal.
#define IR_TransmitDataPin         11  // Aan deze pin zit een zender IR-Led. (gebufferd via transistor i.v.m. hogere stroom die nodig is voor IR-led)
#define RF_TransmitPowerPin         4  // +5 volt / Vcc spanning naar de zender.
#define RF_TransmitDataPin          5  // data naar de zender
#define RF_ReceiveDataPin           2  // Op deze input komt het 433Mhz-RF signaal binnen. LOW bij geen signaal.
#define RF_ReceivePowerPin         12  // Spanning naar de ontvanger via deze pin.
#define MonitorLedPin              13  // bij iedere ontvangst of verzending licht deze led kort op.
#define BuzzerPin                   6  // luidspreker aansluiting
#define WiredAnalogInputPin_1       0  // vier analoge inputs van 0 tot en met 3
#define WiredDigitalOutputPin_1     7  // vier digitale outputs van 7 tot en met 10

#define UNIT                       0x1 // Unit nummer van de Nodo. Bij gebruik van meerdere nodo's deze uniek toewijzen [1..F]
#define Eventlist_OFFSET            64 // Eerste deel van het EEPROM geheugen is voor de settings. Reserveer __ bytes. Deze niet te gebruiken voor de Eventlist.
#define Eventlist_MAX              120 // aantal events dat de lijst bevat in het EEPROM geheugen van de ATMega328. Iedere event heeft 8 bytes nodig. eerste adres is 0
#define USER_VARIABLES_MAX          15 // aantal beschikbare gebruikersvariabelen voor de user.
#define UNIT_MAX                    15
#define MACRO_EXECUTION_DEPTH       10 // maximale nesting van macro's.

#define SIGNAL_TYPE_UNKNOWN          0
#define SIGNAL_TYPE_NODO             1
#define SIGNAL_TYPE_KAKU             2
#define SIGNAL_TYPE_NEWKAKU          3

#define NODO_TYPE_EVENT              1
#define NODO_TYPE_COMMAND            2

#ifndef AVR_LIRC
#define BAUD                     57600 // Baudrate voor seriële communicatie. RKR 19200->57600 Abd CR/LF instead of just LF
#else
#ifdef ANALYSIR
#define BAUD                     2000000 // Baudrate voor seriële communicatie.
#else
#define BAUD                     38400 // LIRC
#endif
#endif
#define SERIAL_TERMINATOR_1       0x0D // Met dit teken wordt een regel afgesloten. 0x0A is een linefeed <LF>, default voor EventGhost
#define SERIAL_TERMINATOR_2       0x0A // Met dit teken wordt een regel afgesloten. 0x0D is een Carriage Return <CR>, 0x00 = niet in gebruik.

#define EVENT_PART_COMMAND           1
#define EVENT_PART_TYPE              2
#define EVENT_PART_UNIT              3
#define EVENT_PART_PAR1              4
#define EVENT_PART_PAR2              5

#define DISPLAY_TIMESTAMP            1
#define DISPLAY_UNIT                 2
#define DISPLAY_DIRECTION            4
#define DISPLAY_SOURCE               8
#define DISPLAY_TRACE               16
#define DISPLAY_TAG                 32
#define DISPLAY_SERIAL              64

#define DISPLAY_RESET               DISPLAY_UNIT + DISPLAY_SOURCE + DISPLAY_DIRECTION + DISPLAY_TAG
#define ENABLE_SOUND				false	// RKR default for EnableSound
// settings voor verzenden en ontvangen van IR/RF
#define ENDSIGNAL_TIME          1500 // Dit is de tijd in milliseconden waarna wordt aangenomen dat het ontvangen één reeks signalen beëindigd is
#define SIGNAL_TIMEOUT_RF       2600 // na deze tijd in uSec. wordt één RF signaal als beëindigd beschouwd. RKR was 5000 but use preamble timing to enlarge...
#define SIGNAL_TIMEOUT_IR      10000 // na deze tijd in uSec. wordt één IR signaal als beëindigd beschouwd.
#define TX_REPEATS                 5 // aantal herhalingen van een code binnen één RF of IR reeks
#define MIN_PULSE_LENGTH         75 // pulsen korter dan deze tijd uSec. worden als stoorpulsen beschouwd. RKR was 100 try 75

#ifdef ANALYSIR
#define MIN_RAW_PULSES            16 // =8 bits. Minimaal aantal ontvangen bits*2 alvorens cpu tijd wordt besteed aan decodering, etc. Zet zo hoog mogelijk om CPU-tijd te sparen en minder 'onzin' te ontvangen.
#else
#define MIN_RAW_PULSES            16 // =8 bits. Minimaal aantal ontvangen bits*2 alvorens cpu tijd wordt besteed aan decodering, etc. Zet zo hoog mogelijk om CPU-tijd te sparen en minder 'onzin' te ontvangen.
#endif
#define SHARP_TIME               500 // tijd in milliseconden dat de nodo gefocust moet blijven luisteren naar één dezelfde poort na binnenkomst van een signaal
//****************************************************************************************************************************************
typedef enum OUTPUTMODE {
	omNodoDueRkr, omLirc, omAnalysIR
} OUTPUTMODE;

struct Settings
  {
  int     Version;
  uint	SizeofSettings;
  byte	Mode;
  ulong	BaudRate;
  byte    AnalyseSharpness;
  int     AnalyseTimeOut;
  byte    Display;
  byte    TransmitPort;
  byte    TransmitRepeat;
  byte    WaitFreeRF_Window;
  byte    WaitFreeRF_Delay;
  boolean SendBusy;
  boolean WaitBusy;
  }settings;

// timers voor verwerking op intervals
#define Loop_INTERVAL_1          250  // tijdsinterval in ms. voor achtergrondtaken.
#define Loop_INTERVAL_2         5000  // tijdsinterval in ms. voor achtergrondtaken.
ulong StaySharpMillis=millis();
ulong LoopIntervalTimer_1=millis();// millis() maakt dat de intervallen van 1 en 2 niet op zelfde moment vallen => 1 en 2 nu asynchroon
ulong LoopIntervalTimer_2=0L;
ulong psStartSignalMillis=millis(); // RKR measure time between signals
ulong psStartSignalMillisLast= 0; // RKR measure time between signals for filtered signals
uint pulseSpaceMicros[RAW_BUFFER_SIZE+4 + RAW_BUFFER_TIMERANGE_SIZE];          // Tabel met de gemeten pulsen in microseconden. eerste waarde is het aantal bits*2

// RAWSIGNAL_TOGGLE
boolean RawsignalGet=true;

ulong Content=0L,ContentPrevious;
ulong Checksum=0L;
ulong SupressRepeatTimer;
ulong HoldTimer;
ulong EventTimeCodePrevious;                // t.b.v. voorkomen herhaald ontvangen van dezelfde code binnen ingestelde tijd
uint8_t RFbit,RFport,IRbit,IRport;

void setup()
  {
  byte x;

  pinMode(IR_ReceiveDataPin,INPUT);
  pinMode(RF_ReceiveDataPin,INPUT);
  pinMode(RF_TransmitDataPin,OUTPUT);
  pinMode(RF_TransmitPowerPin,OUTPUT);
  pinMode(RF_ReceivePowerPin,OUTPUT);
  pinMode(IR_TransmitDataPin,OUTPUT);
  pinMode(MonitorLedPin,OUTPUT);
  pinMode(BuzzerPin, OUTPUT);

  digitalWrite(IR_ReceiveDataPin,HIGH);  // schakel pull-up weerstand in om te voorkomen dat er rommel binnenkomt als pin niet aangesloten
  digitalWrite(RF_ReceiveDataPin,HIGH);  // schakel pull-up weerstand in om te voorkomen dat er rommel binnenkomt als pin niet aangesloten
  digitalWrite(RF_ReceivePowerPin,HIGH); // Spanning naar de RF ontvanger aan.

  RFbit=digitalPinToBitMask(RF_ReceiveDataPin);
  RFport=digitalPinToPort(RF_ReceiveDataPin);
  IRbit=digitalPinToBitMask(IR_ReceiveDataPin);
  IRport=digitalPinToPort(IR_ReceiveDataPin);

	LoadSettingsFromEeprom();	// store baudrate and mode in Eeprom
  Serial.begin(settings.BaudRate);  // Initialiseer de seriële poort
  IR38Khz_set();       // Initialiseet de 38Khz draaggolf voor de IR-zender.
  PrintWelcome();
  }

/*
 *	ParseArgs
 *
 * commands:
 *	m mode a, l, n
 *  b baudrate 9600, 38400, 57600, 115200 or analysIR speed
 *  s save and reset
 *  r factory reset?
 *
 * space, separators, commands and numbers
 * autosave at eol
 * \n\r, \r, \n as terminators
 * ; as command separator
 * space, tab, =, -, \, / param separator
 *
 * for now: command, arg, argn
 * autosave at eol unless last is ; for second command?
 */
char ParseArgs() {
	byte c=0;
      while (Serial.available()>0) {
			byte SerialByte=Serial.read();
			switch(SerialByte) {
				case '\r':
					if (Serial.peek() == '\n') {
						SerialByte=Serial.read();
					}
				case '\n':
					return '\n';
				case ' ':
				case '\t':
					break;
				case '=':
				case '-':
				case '/':
				case '\\':
					c = SerialByte;
					break;
				case ';':
					return ';';
				default:
					return SerialByte;
			}
		}
		return c;
}

void ParseCommand() {
	static char cmd='\0';
	static boolean fAutoSave = true;
	char c=ParseArgs();
	if (c) {
		if (!cmd) {
			switch(c) {
				case 'm': // mode n, l, a
					cmd = c;
					break;
				case 'b':
					cmd = c;
					break;
				case ';':
					fAutoSave = false;
					break;
				case '\n':
					if (fAutoSave) {
						SaveSettingsToEepromAndReset();
					}
					fAutoSave=true;
					break;
				case 's':
					SaveSettingsToEepromAndReset();
					break;
				case 'r':
					ResetFactory();
					break;
				default:
					fAutoSave=true;
					PrintLnStartRaw(F("Usage \nmode: m n/l/a\nbaudrate b baud\nresetfactory r"));
					break;
			}
		}
		else { // cmd + arg
			switch(cmd) {
				case 'm': // mode n, l, a
					switch(c) {
						case 'n':
							  settings.Mode	= omNodoDueRkr;
							  settings.BaudRate = 57600; // Baudrate voor seriële communicatie. RKR 19200->57600 Abd CR/LF instead of just LF
							  break;
						case 'l':
							  settings.Mode	= omLirc;
							  settings.BaudRate = 38400; // LIRC
							  break;
						case 'a':
							  settings.Mode	= omAnalysIR;
							  settings.BaudRate = 2000000;
							  break;
						case 'A':
							  settings.Mode	= omAnalysIR;
							  settings.BaudRate = 115200;
							  break;
					}
					cmd = '\0';
					break;
				case 'b': // baudrate... todo
					cmd = '\0';
					break;
				default:
					cmd = '\0';
			}
			ParseCommand();
		}
	}
}


void loop()
  {
  int x,y,z;

//  SerialHold(false); // er mogen weer tekens binnen komen van SERIAL

  // hoofdloop: scannen naar signalen
  // dit is een tijdkritische loop die wacht tot binnengekomen event op IR, RF, SERIAL, CLOCK, DAYLIGHT, TIMER
  // als er geen signalen binnenkomen duurt deze hoofdloop +/- 35uSec. snel genoeg om geen signalen te missen.
  while(true)
    {
    // SERIAL: *************** kijk of er data klaar staat op de seriële poort **********************
      if(Serial.available()>0)
        {
		ParseCommand();
	}
	{ // RKR RawsignalGet measure repetitions
		int psmStart = 0;
	    //StaySharpMillis=millis()+SHARP_TIME;

		// RF: *************** kijk of er data start op IR en genereer een event als er een code ontvangen is **********************
		do// met StaySharp wordt focus gezet op luisteren naar IR, doordat andere input niet wordt opgepikt
		  {
		  while((*portInputRegister(IRport)&IRbit)==0)// Kijk if er iets op de IR poort binnenkomt. (Pin=LAAG als signaal in de ether).
			{ulong StartSignalTime = millis();
			if(FetchSignal(IR_ReceiveDataPin,LOW,SIGNAL_TIMEOUT_IR/2, psmStart))// Als het een duidelijk IR signaal was
			  {
				  	if (psmStart == 0) { //inter messages time
						psStartSignalMillis = StartSignalTime;
					}
					psmStart = psmStart + pulseSpaceMicros[psmStart] + 2;
					 // intra message time
					StartSignalTime -= (StaySharpMillis  - SHARP_TIME*2);
					pulseSpaceMicros[psmStart-1] = (StartSignalTime > 0) ? StartSignalTime : 1;

					StaySharpMillis=millis()+SHARP_TIME*2;
			}
			else { // Noise/Spikes
					break;
			}
		  }
		} while(millis()<StaySharpMillis);
	    pulseSpaceMicros[psmStart]=0; // next count 0
	    if (psmStart > 0){
			Content=AnalyzeRawSignal(0); // Bereken uit de tabel met de pulstijden de 32-bit code.
			if(Content)// als AnalyzeRawSignal een event heeft opgeleverd
			{
			   PrintPulseSpaceTimings(Content,false); // verwerk binnengekomen event.
			}
		}
	}

	{ // RKR RawsignalGet measure repetitions
		int psmStart = 0;

		// RF: *************** kijk of er data start op RF en genereer een event als er een code ontvangen is **********************
		do// met StaySharp wordt focus gezet op luisteren naar RF, doordat andere input niet wordt opgepikt
		  {
		  while((*portInputRegister(RFport)&RFbit)==RFbit)// Kijk if er iets op de RF poort binnenkomt. (Pin=HOOG als signaal in de ether).
			{ulong StartSignalTime = millis();
			if(FetchSignal(RF_ReceiveDataPin,HIGH,SIGNAL_TIMEOUT_RF, psmStart))// Als het een duidelijk RF signaal was
			  {
				  	if (psmStart == 0) { //inter messages time
						psStartSignalMillis = StartSignalTime;
					}
					psmStart = psmStart + pulseSpaceMicros[psmStart] + 2;
					 // intra message time
					StartSignalTime -= (StaySharpMillis  - SHARP_TIME);
					pulseSpaceMicros[psmStart-1] = (StartSignalTime > 0) ? StartSignalTime : 1;

					StaySharpMillis=millis()+SHARP_TIME;
			}
			else { // Noise/Spikes
					break;
			}
		  }
		} while(millis()<StaySharpMillis);
	    pulseSpaceMicros[psmStart]=0; // next count 0
	    if (psmStart > 0){
			Content=AnalyzeRawSignal(0); // Bereken uit de tabel met de pulstijden de 32-bit code.
			if(Content)// als AnalyzeRawSignal een event heeft opgeleverd
			{
			   PrintPulseSpaceTimings(Content,true); // verwerk binnengekomen event.
			}
		}
	}
#if 0
    // 2: niet tijdkritische processen die periodiek uitgevoerd moeten worden
    if(LoopIntervalTimer_2<millis()) // lange interval
      {
      LoopIntervalTimer_2=millis()+Loop_INTERVAL_2; // reset de timer
      }// lange interval

    // 1: niet tijdkritische processen die periodiek uitgevoerd moeten worden
    if(LoopIntervalTimer_1<millis())// korte interval
      {
      LoopIntervalTimer_1=millis()+Loop_INTERVAL_1; // reset de timer
      }// korte interval
#endif
  	digitalWrite(MonitorLedPin,LOW);
    }// // while
  }


