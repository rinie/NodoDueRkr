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
  char ByteToSave,*pointerToByteToSave=pointerToByteToSave=(char*)&S;    //pointer verwijst nu naar startadres van de struct. Ge-cast naar char omdat EEPROMWrite per byte wegschrijft
  for(int x=0; x<sizeof(struct Settings) ;x++)
    {
    EEPROM.write(x,*pointerToByteToSave);
    pointerToByteToSave++;
    }
  }

 /*********************************************************************************************\
 * Laad de settings uit het EEPROM geheugen.
 \*********************************************************************************************/
boolean LoadSettings()
  {
  char ByteToSave,*pointerToByteToRead=(char*)&S;    //pointer verwijst nu naar startadres van de struct.
  for(int x=0; x<sizeof(struct Settings);x++)
    {
    *pointerToByteToRead=EEPROM.read(x);
    pointerToByteToRead++;// volgende byte uit de struct
    }
  }


 /*********************************************************************************************\
 * Alle settings van de Nodo weer op default.
 \*********************************************************************************************/
void ResetFactory(void)
  {
  S.EnableSound        = ENABLE_SOUND;
//  Beep(2000,2000);
#ifdef CLOCK // RKR make optional to save space
  ClockRead();
#endif

  S.Version            = VERSION;
  S.Unit               = UNIT;
  S.Display            = DISPLAY_RESET;
  S.AnalyseSharpness   = 50;
  S.AnalyseTimeOut     = SIGNAL_TIMEOUT_IR;
  S.TransmitPort       = VALUE_SOURCE_IR_RF;
  S.TransmitRepeat     = TX_REPEATS;
  S.SendBusy           = false;
  S.WaitBusy           = false;
  S.WaitFreeRF_Window  = 0;
  S.WaitFreeRF_Delay   = 0;
#ifdef CLOCK // RKR make optional to save space
  S.DaylightSaving     = Time.DaylightSaving;
#endif
#ifdef WIRED // RKR make optional to save space
  for(byte x=0;x<4;x++)
    {
    S.WiredInputThreshold[x]=0x80;
    S.WiredInputSmittTrigger[x]=5;
    S.WiredInputRange[x]=0;
    S.WiredInputPullUp[x]=true;
    }
#endif
#ifdef USERVAR // RKR make optional to save space
  // maak alle variabelen leeg
  for(byte x=0;x<USER_VARIABLES_MAX;x++)
     S.UserVar[x]=0;
#endif
  SaveSettings();
//  FactoryEventlist();
  delay(500);// kleine pauze, anders kans fout bij seriële communicatie
  Reset();
  }

void LoadSettingsFromEeprom(void) {
  LoadSettings();      // laad alle settings zoals deze in de EEPROM zijn opgeslagen

  if(S.Version!=VERSION)ResetFactory(); // Als versienummer in EEPROM niet correct is, dan een ResetFactory.
}
