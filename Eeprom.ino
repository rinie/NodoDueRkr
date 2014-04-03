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
 * Sla alle settings op in het EEPROM geheugen.
 \*********************************************************************************************/
void SaveSettings(void)
  {
  char ByteToSave,*pointerToByteToSave=pointerToByteToSave=(char*)&settings;    //pointer verwijst nu naar startadres van de struct. Ge-cast naar char omdat EEPROMWrite per byte wegschrijft
  for(int x=0; x<sizeof(struct Settings) ;x++)
    {
    EEPROM.write(x,*pointerToByteToSave);
    pointerToByteToSave++;
    }
  }

 /*********************************************************************************************\
 * Laad de settings uit het EEPROM geheugen.
 \*********************************************************************************************/
boolean LoadSettings(void)
  {
  char ByteToSave,*pointerToByteToRead=(char*)&settings;    //pointer verwijst nu naar startadres van de struct.
  for(int x=0; x<sizeof(struct Settings);x++)
    {
    *pointerToByteToRead=EEPROM.read(x);
    pointerToByteToRead++;// volgende byte uit de struct
    }
  }

void(*Reset)(void)=0;                               //reset functie op adres 0

 /*********************************************************************************************\
 * Alle settings van de Nodo weer op default.
 \*********************************************************************************************/
void ResetFactory(void)
  {
  settings.Version            = VERSION;
  settings.SizeofSettings = sizeof(settings);
  settings.Mode	= omNodoDueRkr;
  settings.BaudRate				= BAUD;
  settings.Display            = DISPLAY_RESET;
  settings.AnalyseSharpness   = 50;
  settings.AnalyseTimeOut     = SIGNAL_TIMEOUT_IR;
  settings.TransmitPort       = VALUE_SOURCE_IR_RF;
  settings.TransmitRepeat     = TX_REPEATS;
  settings.SendBusy           = false;
  settings.WaitBusy           = false;
  settings.WaitFreeRF_Window  = 0;
  settings.WaitFreeRF_Delay   = 0;

  SaveSettings();
  Serial.print(F("FactoryReset"));
  PrintTerm();
  delay(500);// kleine pauze, anders kans fout bij seriële communicatie
  Reset();
  }

void LoadSettingsFromEeprom(void) {
  	LoadSettings();      // laad alle settings zoals deze in de EEPROM zijn opgeslagen

  	if((settings.Version!=VERSION) || (settings.SizeofSettings!=sizeof(settings))) {
    	ResetFactory(); // Als versienummer in EEPROM niet correct is, dan een ResetFactory.
	}
}
