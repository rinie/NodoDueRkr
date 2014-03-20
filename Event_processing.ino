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


 /**********************************************************************************************\
 * Voert alle relevante acties in de eventlist uit die horen bij het binnengekomen event
 * Doorlopen van een volledig gevulde eventlist duurt ongeveer 15ms inclusief printen naar serial
 * maar exclusief verwerking n.a.v. een 'hit'.
 \*********************************************************************************************/
boolean ProcessEvent(unsigned long IncommingEvent, byte Direction, byte Port, unsigned long PreviousContent, byte PreviousPort)
  {
  byte x;
  //boolean CommandForThisNodo=CheckEventlist(IncommingEvent) || NodoType(IncommingEvent)==NODO_TYPE_COMMAND;
  boolean SetBusyOff=false;

  digitalWrite(MonitorLedPin,HIGH);           // LED aan als er iets verwerkt wordt

  //if(CommandForThisNodo && Hold)
  //  PrintText(Text_09);

  PrintEvent(IncommingEvent,Port,Direction);  // geef event weer op Serial
  // Als de RAW pulsen worden opgevraagd door de gebruiker...
  // dan de pulsenreeks weergeven en er verder niets mee doen
  if(RawsignalGet)
    {
	// RAWSIGNAL_MULTI
	for(int RawSignalStart = 0; (RawSignal[RawSignalStart] != 0) && (RawSignalStart < RAW_BUFFER_SIZE); RawSignalStart = RawSignalStart + RawSignal[RawSignalStart] + 2) {
		PrintRawSignal(RawSignalStart);
	}
	PrintRawSignalEnd();
	// RAWSIGNAL_TOGGLE // RKR repeat until toggle off
    RawsignalGet=false;
    return true;
    }
  }
