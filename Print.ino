/**************************************************************************\

    This file is part of Nodo Due, © Copyright Paul Tonkes

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


 /*********************************************************************************************\
 * Print een decimaal getal
 * Serial.Print neemt veel progmem in beslag.
 \*********************************************************************************************/
void PrintValue(ulong x)
  {
  if(x<=255)
    Serial.print(x,DEC);
  else
    {
    Serial.print("0x");
    Serial.print(x,HEX);
    }
  }

 /*********************************************************************************************\
 * Print een lijn met het teken '*'
 \*********************************************************************************************/
void PrintLine(void)
  {
	bool fShow = settings.Mode != omLirc;
	bool fAnalysIRComment = settings.Mode == omAnalysIR;
	if (!fShow) {
		return;
	}

	if (fAnalysIRComment) { // AnalysIR comment start
		PrintChar('!');
	}

  for(byte x=1;x<=60;x++)PrintChar('*');
  PrintTermRaw();
  }


 /*********************************************************************************************\
 * Serial.Print neemt veel progmem in beslag.
 * Print het teken '0'
 \*********************************************************************************************/
void PrintChar(byte S)
  {
  Serial.write(S);
  }


 /*********************************************************************************************\
 * Print een EventCode. de MMI van de Nodo
 * Een Evencode wordt weergegeven als "(<commando> <parameter-1>,<parameter-2>)"
 \*********************************************************************************************/
#define P_NOT   0
#define P_TEXT  1
#define P_KAKU  2
#define P_VALUE 3
#define P_DIM   4

void PrintEventCode(ulong Code)
  {
  byte P1,P2,Par2_b;
  boolean P2Z=true;     // vlag: true=Par2 als nul waarde afdrukken false=nulwaarde weglaten

  byte Type     = (Code>>28)&0xf;
  byte Command  = (Code>>16)&0xff;
  byte Par1     = (Code>>8)&0xff;
  byte Par2     = (Code)&0xff;

  PrintChar('(');

  if(Type==SIGNAL_TYPE_NEWKAKU)
    {
    // Aan/Uit zit in bit 5
    Serial.print(F("KakuNew"));
    PrintChar(' ');
#if 0
    PrintValue(Code&0x0FFFFFEF);
#else
        Serial.print(F("Id:"));
        Serial.print(Code>>5,HEX);

        if (((Code>>5)&0x01)) {
	        Serial.print(F(",All"));
		}
		else {
			Serial.print(F(",Unit:"));
			Serial.print(Code&0xF,HEX);
		}
#endif
    PrintChar(',');
#if 0 // 2015
    Serial.print(cmd2str(((Code>>4)&0x1)?VALUE_ON:VALUE_OFF));
#endif
    }

  else if(Type==SIGNAL_TYPE_NODO || Type==SIGNAL_TYPE_KAKU)
    {
    Serial.print(F("Kaku"));
    switch(Command)
      {
      // Par1 als KAKU adres [A0..P16] en Par2 als [On,Off]
      case CMD_KAKU:
      //case CMD_SEND_KAKU:
        P1=P_KAKU;
        P2=P_TEXT;
        Par2_b=Par2;
        break;

      case CMD_KAKU_NEW:
      //case CMD_SEND_KAKU_NEW:
        P1=P_VALUE;
        P2=P_DIM;
        break;
      // Par1 als waarde en par2 als waarde
      default:
        P1=P_VALUE;
        P2=P_VALUE;
      }

    // Print Par1
    if(P1!=P_NOT)
      {
      PrintChar(' ');
      switch(P1)
        {
        case P_TEXT:
          Serial.print(cmd2str(Par1));
          break;
        case P_VALUE:
          PrintValue(Par1);
          break;
        case P_KAKU:
          PrintChar('A'+((Par1&0xf0)>>4)); //  A..P printen.
          PrintValue(Par2_b&0x2?0:(Par1&0xF)+1);// als 2e bit in Par2 staat, dan is het een groep commando en moet adres '0' zijn.
          Par2=(Par2&0x1)?VALUE_ON:VALUE_OFF;
          break;
        }
      }// P1

    // Print Par2
    if(P2!=P_NOT)
      {
      PrintChar(',');
      switch(P2)
        {
        case P_TEXT:
          Serial.print(cmd2str(Par2));
          break;
        case P_VALUE:
          PrintValue(Par2);
          break;
        case P_DIM:
          {
          if(Par2==VALUE_OFF || Par2==VALUE_ON)
            Serial.print(cmd2str(Par2)); // Print 'On' of 'Off'
          else
            PrintValue(Par2);
          break;
          }
        }
      }// P2
    }//   if(Type==SIGNAL_TYPE_NODO || Type==SIGNAL_TYPE_OTHERUNIT)

  else // wat over blijft is het type UNKNOWN.
    PrintValue(Code);

  PrintChar(')');
  }


 /**********************************************************************************************\
 * Verzend teken(s) naar de seriele poort die een regel afsluiten.
 \*********************************************************************************************/
void PrintTerm()
  {
  // FreeMemory();//??? t.b.v. debugging
  if(SERIAL_TERMINATOR_1)PrintChar(SERIAL_TERMINATOR_1);
  if(SERIAL_TERMINATOR_2)PrintChar(SERIAL_TERMINATOR_2);
  }

void PrintTermRaw()
{
	bool fShow = settings.Mode != omLirc;
	bool fAnalysIRComment = settings.Mode == omAnalysIR;
	if (!fShow) {
		return;
	}

	if (fAnalysIRComment) { // AnalysIR comment end
		PrintChar('!');
	}
	PrintTerm();
}

void PrintStartRaw(const __FlashStringHelper *s) {
	bool fShow = settings.Mode != omLirc;
	bool fAnalysIRComment = settings.Mode == omAnalysIR;
	if (!fShow) {
		return;
	}

	if (fAnalysIRComment) { // AnalysIR comment start
		PrintChar('!');
	}
	Serial.print(s);
}

void PrintLnStartRaw(const __FlashStringHelper *s) {
	PrintStartRaw(s);
	PrintTermRaw();
}


 /**********************************************************************************************\
 * Print de welkomsttekst van de Nodo.
 \*********************************************************************************************/
void PrintWelcome(void)
  {
  // Print Welkomsttekst
  PrintTerm();
  PrintLine();


  PrintLnStartRaw(F("NodoDueRkr (c)2011-2020 Rinie Kervel."));
  PrintLnStartRaw(F("based on Nodo Due (c)2011 P.K.Tonkes."));
  PrintLnStartRaw(F("License: GNU General Public License."));
  PrintStartRaw(F("Version="));
  PrintValue(settings.Version/100);
  PrintChar('.');
  PrintValue((settings.Version%100)/10);
  PrintChar('.');
  PrintValue(settings.Version%10);
  PrintChar('.');
  PrintTermRaw();
  reportFreeRAM(0xFFFF);
  PrintStartRaw(F("BaudRate="));
  Serial.print(settings.BaudRate,DEC);
  PrintTermRaw();
  PrintStartRaw(F("Mode(NodoDueRkr/LIRC/AnalysIR):"));
  Serial.print(((settings.Mode == omNodoDueRkr) ? F("NodoDueRkr") : ((settings.Mode == omLirc) ? F("LIRC") : F("AnalysIR"))));
  PrintTermRaw();
	//Serial.print("rawsignalget; on");
  PrintLine();
  }

 /**********************************************************************************************\
 * Print een string uit PROGMEM
 \*********************************************************************************************/
void PrintText(PGM_P const text)
  {
  byte x=0;
  char buffer[60];

  do
    {
    buffer[x]=pgm_read_byte_near(text+x);
    }while(x < 60 && (buffer[x++]!=0)); // RKR prevent overflow
  buffer[59] = 0;
  Serial.print(buffer);
  }

