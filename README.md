NodoDueRkr
- Receive 433 MHZ OOK RF and IR pulse/space timings in microseconds
- Report them in text, LIRC or AnalysIR formats

Nodo_Due forked from http://code.google.com/p/arduino-nodo/

Receive/Sends KAKU OOK Signals (old and new) with an Arduino

See Dutch references
http://www.nodo-domotica.nl
And
https://groups.google.com/group/arduinodo

TimeRangeAnalysis
- cPulseRanges, cSpaceRanges, cPulseSpaceRanges: regular DATA pulse/space/pulse + space timings (3..)
- pulse ranges same as space ranges: 0,1 decoding
- Check 1/2: same range as data
-- Preamble: lots of repetitions (say 4,8 or...) 32*1,S,0
-- Sync/Leader: first longer than all others (X10), S or L or H, Ss,010101
-- None: direct start of signal (KAKU): 0101

-- Marklength != spacelength: 0=short mark, 1 = short space, 2=longer mark, 3=longer space: odd means space?

-Specials: bitcombination encodings:
--Oregon scientific:
-- Manchester + inverted bits

- Kaku old:
-- 0 = T,3T,T,3T, 1 = T,3T,3T,T, short 0 = T,3T,T,T
In bitwise format 0101 0110 0100 or hex 5, 6, 4
- Kaku New:
-- 0 = T,T,T,4T, 1 = T,4T,T,T, dim = T,T,T,T on bit 27
In bitwise format 0001 0100 0000 or hex, all pulses are constant so you can 
use only spaces or only total time (P+S):
The 0 = 2T + 5T, 1 = 5T + 2T and dim = 2T 2T or Hex 01, 10 and 00, so 11 indicates no KakuNew

Look for nibble patterns? store 4 nibbles and see if all signals conform to that pattern

