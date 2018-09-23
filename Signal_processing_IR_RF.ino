  /**************************************************************************\
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
  \**************************************************************************/

// timings NODO signalen
#define NODO_PULSE_0                    500   // PWM: Tijdsduur van de puls bij verzenden van een '0' in uSec.
#define NODO_PULSE_MID                 1000   // PWM: Pulsen langer zijn '1'
#define NODO_PULSE_1                   1500   // PWM: Tijdsduur van de puls bij verzenden van een '1' in uSec. (3x NODO_PULSE_0)
#define NODO_SPACE                      500   // PWM: Tijdsduur van de space tussen de bitspuls bij verzenden van een '1' in uSec.

#define NewKAKU_RawSignalLength      132
#define NewKAKUdim_RawSignalLength   148
#define KAKU_CodeLength    12

ulong AnalyzeRawSignal(uint psmIndexStart)
  {
  ulong Code=0L;

  if(PsCount(psmIndexStart)>=RAW_BUFFER_SIZE)return 0L;     // Als het signaal een volle buffer beslaat is het zeer waarschijnlijk ruis.

	switch (PsCount(psmIndexStart)) {
	case 66:
  		Code=RawSignal_2_Nodo(psmIndexStart);
  		break;
  	case KAKU_CodeLength*4+2:
    	Code=RawSignal_2_KAKU(psmIndexStart);
    	break;
    case NewKAKU_RawSignalLength:
    case NewKAKUdim_RawSignalLength:
      	Code=RawSignal_2_NewKAKU(psmIndexStart);
      	break;
 	}
  if (!Code) { // Geen Nodo, KAKU of NewKAKU code. Genereer uit het onbekende signaal een (vrijwel) unieke 32-bit waarde uit.
     Code=RawSignal_2_32bit(psmIndexStart, false, false);
  }
  return Code;
  }

/*********************************************************************************************\
* Deze routine berekent de uit een pulseSpaceMicros een NODO code
* Geeft een false retour als geen geldig NODO signaal
\*********************************************************************************************/
ulong RawSignal_2_Nodo(uint psmIndexStart)
  {
  ulong bitstream=0L;
  int x,y,z;
  int xEnd = PsCount(psmIndexStart) + psmIndexStart;
  // nieuwe NODO signaal bestaat altijd uit start bit + 32 bits. Ongelijk aan 66, dan geen Nodo signaal
  if (PsCount(psmIndexStart)!=66)return 0L;

  x=3 + psmIndexStart; // 0=aantal, 1=startpuls, 2=space na startpuls, 3=1e pulslengte
  do{
    if(PulseSpaceMicros(x)>NODO_PULSE_MID)
      bitstream|=(ulong)(1L<<z); //LSB in signaal wordt  als eerste verzonden
    x+=2;
    z++;
    }while(x<xEnd);

  if(((bitstream>>28)&0xf) == SIGNAL_TYPE_NODO)// is het type-nibble uit het signaal gevuld met de aanduiding NODO ?
    return bitstream;
  else
    return 0L;
  }




 /**********************************************************************************************\
 * Deze functie genereert uit een willekeurig gevulde pulseSpaceMicros afkomstig van de meeste
 * afstandsbedieningen een (vrijwel) unieke bit code.
 * Zowel breedte van de pulsen als de afstand tussen de pulsen worden in de berekening
 * meegenomen zodat deze functie geschikt is voor PWM, PDM en Bi-Pase modulatie.
 * LET OP: Het betreft een unieke hash-waarde zonder betekenis van waarde.
 \*********************************************************************************************/
ulong RawSignal_2_32bit(uint psmIndexStart, bool fRkrTimeRangeStats, bool fPrint) {
	int x,y,z;
	int Counter_pulse=0,Counter_space=0;
	int MinPulse=0xffff;
	int MinSpace=0xffff;
	int MaxPulse=0x0;//RKR
	int MaxSpace=0x0;
	int MinPulseSpace=0xffff;
	int MaxPulseSpace=0x0;//RKR
	int MinPulseP;
	int MinSpaceP;
	ulong CodeP=0L;
	ulong CodeS=0L;
	int xEnd = PsCount(psmIndexStart) + psmIndexStart;

	// Kleinste, groter dan mid
	// Grootste, kleinder dan mid
	// diff > x?
	// dan replace by median
	// zoek de kortste tijd (PULSE en SPACE)
	x = 5 + psmIndexStart; // 0=aantal, 1=startpuls, 2=space na startpuls, 3=1e puls
	while (x <= xEnd-4) {
		if (PulseSpaceMicros(x) < MinPulse) {
			MinPulse=PulseSpaceMicros(x); // Zoek naar de kortste pulstijd.
		}
		if (PulseSpaceMicros(x) > MaxPulse) {
			MaxPulse=PulseSpaceMicros(x); // Zoek naar de langste pulstijd.
		}

		x++;

		if (PulseSpaceMicros(x) < MinSpace && PulseSpaceMicros(x) > 10) {
			MinSpace=PulseSpaceMicros(x); // Zoek naar de kortste spacetijd.
		}
		if (PulseSpaceMicros(x) > MaxSpace) {
			MaxSpace=PulseSpaceMicros(x); // Zoek naar de langste spacetijd.
		}

		if (PulseSpaceMicros(x)+PulseSpaceMicros(x+1) < MinPulseSpace && PulseSpaceMicros(x) > 10) {
				MinPulseSpace=PulseSpaceMicros(x)+PulseSpaceMicros(x+1); // Zoek naar de kortste pulse + spacetijd.
		}

		if (PulseSpaceMicros(x)+PulseSpaceMicros(x+1) > MaxPulseSpace) {
			MaxPulseSpace = PulseSpaceMicros(x) + PulseSpaceMicros(x+1); // Zoek naar de langste pulse + spacetijd.
		}
		x++;
	}

	MinPulseP = MinPulse; // RKR print without rounding
	MinSpaceP = MinSpace;
	if (MaxPulse - MinPulse < 200) {
		// RKR Original
		MinPulse+=(MinPulse*settings.AnalyseSharpness)/100;
	}
	else {
		// try half way
		MinPulse+= (MaxPulse-MinPulse) / 2;
	}

	if (MaxSpace - MinSpace < 200) {
		MinSpace+=(MinSpace*settings.AnalyseSharpness)/100;
	}
	else {
		// try half way
		MinSpace+= (MaxSpace-MinSpace) / 2;
	}

	x=3 + psmIndexStart; // 0=aantal, 1=startpuls, 2=space na startpuls, 3=1e pulslengte
	z=0; // bit in de Code die geset moet worden
	do {
		if (z>31) {
			CodeP=CodeP>>1;
			CodeS=CodeS>>1;
		}

		if (PulseSpaceMicros(x)>MinPulse) {
			if (z <= 31) {// de eerste 32 bits vullen in de 32-bit variabele
				CodeP |= (long)(1L<<z); //LSB in signaal wordt  als eerste verzonden
			}
			else { // daarna de resterende doorschuiven
				CodeP |= 0x80000000L;
			}
			Counter_pulse++;
		}
		x++;

		if (PulseSpaceMicros(x)>MinSpace) {
			if (z<=31) {// de eerste 32 bits vullen in de 32-bit variabele
				CodeS |= (long)(1L<<z); //LSB in signaal wordt  als eerste verzonden
			}
			else { // daarna de resterende doorschuiven
				CodeS |= 0x80000000L;
			}
			Counter_space++;
		}
		x++;

		z++;
	}
	while (x<xEnd);

	if (fRkrTimeRangeStats) { // RKR print Pulse/Space stats, to my OokTimeRange code...
		RkrTimeRangeStats(ixPulse,
			PulseSpaceMicros(psmIndexStart+1),
			MinPulseP, MaxPulse,
			Counter_pulse,
			CodeP,
			fPrint);
		RkrTimeRangeStats(ixSpace,
			PulseSpaceMicros(psmIndexStart+2),
			MinSpaceP, MaxSpace,
			Counter_space,
			CodeS,
			fPrint);
		RkrTimeRangeStats(ixPulseSpace,
			PulseSpaceMicros(psmIndexStart+1) + PulseSpaceMicros(psmIndexStart+2),
			MinPulseSpace, MaxPulseSpace,
			Counter_pulse + Counter_space,
			CodeS^CodeP,
			fPrint);
	}

	if(Counter_pulse>=1 && Counter_space<=1) {
		return CodeP; // data zat in de pulsbreedte
	}
	if(Counter_pulse<=1 && Counter_space>=1) {
		return CodeS; // data zat in de pulse afstand
	}
	return (CodeS^CodeP); // data zat in beide = bi-phase, maak er een leuke mix van.
}

 /*********************************************************************************************\
 * Deze routine berekend de RAW pulsen van een 32-bit Nodo-code en plaatst deze in de buffer pulseSpaceMicros
 * pulseSpaceMicros.Bits het aantal pulsen*2+startbit*2 ==> 66
 *
 \*********************************************************************************************/
void Nodo_2_RawSignal(ulong Code)
  {
  byte BitCounter,y=1;

  // begin met een startbit.
  PulseSpaceMicrosSet(y++, NODO_PULSE_1*2);
  PulseSpaceMicrosSet(y++, NODO_SPACE*4);

  // de rest van de bits LSB als eerste de lucht in
  for(BitCounter=0; BitCounter<=31; BitCounter++)
    {
    if(Code>>BitCounter&1) {
      PulseSpaceMicrosSet(y++, NODO_PULSE_1);
  }
    else {
      PulseSpaceMicrosSet(y++, NODO_PULSE_0);
   }
    PulseSpaceMicrosSet(y++, NODO_SPACE);
    }
  PulseSpaceMicrosSet(y-1, NODO_PULSE_1*10); // pauze tussen de pulsreeksen
  PsCountSet(0, 66); //  1 startbit bestaande uit een pulse/space + 32-bits is 64 pulse/space = totaal 66
  }

/**********************************************************************************************\
* verzendt een event en geeft dit tevens weer op SERIAL
* als het Event gelijk is aan 0L dan wordt alleen de huidige inhoud van de buffer als RAW
* verzonden.
\**********************************************************************************************/

boolean TransmitCode(ulong Event,byte SignalType)
  {

  if(SignalType!=SIGNAL_TYPE_UNKNOWN)
    if((settings.WaitFreeRF_Window + settings.WaitFreeRF_Delay)>=0)
        WaitFreeRF(settings.WaitFreeRF_Delay*100, settings.WaitFreeRF_Window*100); // alleen WaitFreeRF als type bekend is, anders gaat SendSignal niet goed a.g.v. overschrijven buffer

  switch(SignalType)
    {
    case SIGNAL_TYPE_KAKU:
      KAKU_2_RawSignal(Event);
      break;

    case SIGNAL_TYPE_NEWKAKU:
      NewKAKU_2_RawSignal(Event);
      break;

    case SIGNAL_TYPE_NODO:
      Nodo_2_RawSignal(Event);
      break;

    case SIGNAL_TYPE_UNKNOWN:
      break;

    default:
      return false;
    }

  if(settings.TransmitPort==VALUE_SOURCE_RF || settings.TransmitPort==VALUE_SOURCE_IR_RF)
    {
    RawSendRF();
    }

  if(settings.TransmitPort==VALUE_SOURCE_IR || settings.TransmitPort==VALUE_SOURCE_IR_RF)
    {
    RawSendIR();
    }
  }


