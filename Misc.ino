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
 * kopiëer de string van een commando naar een string[]
 \*********************************************************************************************/
char* cmd2str(int i)
  {
  static char string[20];
  if(i<COMMAND_MAX)
    strcpy_P(string,(char*)pgm_read_word(&(CommandText_tabel[i])));
  else {
    string[0]=0;// als er geen gevonden wordt, dan is de string leeg
//    string[1]='\0';
	}
  return string;
  }

 /*********************************************************************************************\
 * Haal uit een string de commando code. False indien geen geldige commando code.
 \*********************************************************************************************/
int str2cmd(char *command)
  {
#if 0
  for(int x=0;x<=COMMAND_MAX;x++)
    if(strcasecmp(command,cmd2str(x))==0)
      return x;
#endif
  return false;
  }


 /*********************************************************************************************\
 * Zet het hoogste nibbel van een event zodat het event het Type kenmerk krijgt
 \*********************************************************************************************/
unsigned long SetEventType(unsigned long Event, byte Type)
  {
  return (Event&0x0fffffff) | (unsigned long)(Type)<<28;
  }


 /*********************************************************************************************\
 * Bouw een Code op uit commando, data1 en data2
 \*********************************************************************************************/
unsigned long command2event(int Command, byte Par1, byte Par2)
    {
    return ((unsigned long)SIGNAL_TYPE_NODO)<<28   |
           ((unsigned long)S.Unit)<<24            |
           ((unsigned long)Command)<<16           |
           ((unsigned long)Par1)<<8               |
            (unsigned long)Par2;
    }


/*********************************************************************************************\
 * String mag HEX, DEC of een NODO commando zijn.
 * Deze routine converteert uit een string een unsigned long waarde.
 * De string moet beginnen met het eerste teken(dus geen voorloop spaties).
 * bij ongeldige tekens in de string wordt een false terug gegeven.
 \*********************************************************************************************/
unsigned long str2val(char *string)
  {
  byte v,w,x,y,c;
  unsigned long Value;

  // Check of het een DEC waarde is.
  Value=0;
  x=0;
  do
    {
    if(string[x]>='0' && string[x]<='9')
      {
      Value=Value*10;
      Value+=string[x]-48;
      }
    else
      {
      x=false; // geen geldig DEC getal
      break;
      }
    x++;
    }while(string[x]!=0);
  if(x)return Value;

  // check of het een HEX code is.
  Value=0;
  x=0;
  do
    {
    if(string[x]>='0' && string[x]<='9')
      {
      Value=Value<<4;
      Value+=string[x]-48;
      }
    else
      {
      if(string[x]>='a' && string[x]<='f')
        {
        Value=Value<<4;
        Value+=string[x]-87;
        }
      else
        {
        if(x<1 || string[x-1]!='0' || string[x]!='x')
          {
          x=false; // geen geldig HEX getal
          break;
          }
        }
      }
    x++;
    }while(string[x]!=0);
  if(x)return Value;
  return str2cmd(string);
  }

 /*********************************************************************************************\
 * Haal voor het opgegeven Command de status op.
 * Let op: call by reference!
 \*********************************************************************************************/
boolean GetStatus(byte *Command, byte *Par1, byte *Par2)
 {
  byte x;
  byte xPar1=*Par1,xPar2=*Par2;

  *Par1=0;
  *Par2=0;
  switch (*Command)
    {
    default:
      return false;
    }
  return true;
  }


 /**********************************************************************************************\
 * Deze functie haalt een tekst op uit PROGMEM en geeft als string terug
 * BUILD 01, 09-01-2010, P.K.Tonkes@gmail.com
 \*********************************************************************************************/
 char* Text(prog_char* text)
  {
  byte x=0;
  static char buffer[40];

  do
    {
    buffer[x]=pgm_read_byte_near(text+x);
    }while(buffer[x++]!=0);
  return buffer;
  }


/**********************************************************************************************\
 * Converteert een string volgens formaat "<Home><address>" naar een absoluut adres [0..255]
 \*********************************************************************************************/
int HA2address(char* HA, byte *group)
  {
  byte x=0,y=false; // teller die wijst naar het het te behandelen teken
  byte c;   // teken uit de string die behandeld wordt
  byte Home=0,Address=0;// KAKU-Home en KAKU-adres

  while((c=HA[x++])!=0)
    {
    if(c>='0' && c<='9'){Address=Address*10;Address=Address+c-'0';}
    if(c>='a' && c<='p'){Home=c-'a';y=true;} // KAKU home A is intern 0
    }

  if(y)// notatie [A1..P16]
    {
    if(Address==0)
      {// groep commando is opgegeven: 0=alle adressen
      *group=true;
      return Home<<4;
      }
    else
      return (Home<<4) | (Address-1);
    }
  else // absoluut adres [0..255]
    return Address; // KAKU adres 1 is intern 0
  }



/* This function places the current value of the heap and stack pointers in the
 * variables. You can call it from any place in your code and save the data for
 * outputting or displaying later. This allows you to check at different parts of
 * your program flow.
 * The stack pointer starts at the top of RAM and grows downwards. The heap pointer
 * starts just above the static variables etc. and grows upwards. SP should always
 * be larger than HP or you'll be in big trouble! The smaller the gap, the more
 * careful you need to be. Julian Gall 6-Feb-2009.
 */
uint8_t *heapptr, *stackptr;
void FreeMemory()
  {
  stackptr = (uint8_t *)malloc(4);          // use stackptr temporarily
  heapptr = stackptr;                     // save value of heap pointer
  free(stackptr);      // free up the memory again (sets stackptr to 0)
  stackptr =  (uint8_t *)(SP);           // save value of stack pointer
  Serial.print(" => Free RAM=");Serial.print(stackptr-heapptr,DEC);
  }
