NodoDueRkr
- Receive 433 MHZ OOK RF and IR pulse/space timings in microseconds
- Report them in text, LIRC or AnalysIR formats

Default start with baudrate 57600 8N1 in mode omNodoDueRkr.
Settings are stored in EEPROM, so after a mode change and reset it will use the last settings

You can change the mode with
m x<cr><lf>
Where x is:
a	AnalysIR format Baudrate 2000000
A	AnalysIR format Baudrate 115200
l	Lirc format Baudrate 38400
n	NodoDueRkr text format baudrate 57600

r<cr><lf> resets to factory setting 57600 mode n


Nodo_Due forked from http://code.google.com/p/arduino-nodo/

Receive/Sends KAKU OOK Signals (old and new) with an Arduino

See Dutch references
http://www.nodo-domotica.nl
And
https://groups.google.com/group/arduinodo

