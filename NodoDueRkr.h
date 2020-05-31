#define RAWSIGNAL_TOGGLE	// RKR instead of just one rawsignal, repeat until you send RawSignalGet; again...
#define RAWSIGNAL_MULTI		// RKR Rawsignal can be multiple signals: <count> <timing data> <count> <timing data> etc...
#define NINJA_BLOCK			// RKR on Ninjablock hardware: extra leds...
#undef NINJA_BLOCK
#undef JEELABS_DECODERS
#ifndef JEELABS_DECODERS
#define RAW_BUFFER_SIZE            472 // Maximaal aantal te ontvangen bits*2
#else
#define RAW_BUFFER_SIZE            236 // Maximaal aantal te ontvangen bits*2
#endif
#define RAW_BUFFER_SIZE            466 // Maximaal aantal te ontvangen bits*2
#define RAW_BUFFER_TIMERANGE_SIZE	36 // RKR pulse time range calculations
#define RAW_BUFFER_TIMERANGE_START	(RAW_BUFFER_SIZE + 4) // RKR borrow same array
typedef unsigned long ulong; //RKR U N S I G N E D is so verbose
typedef unsigned int uint;
#define AVR_LIRC 1
#undef AVR_LIRC
#define AVR_LIRC_BINARY 1
#define ANALYSIR 1
#undef ANALYSIR
#define NODO_DUE	// adapt AnalysIR code to Nodo Due
#ifdef ANALYSIR
#define AVR_LIRC 1	// reuse AVR_LIRC binary usage for AnalysIR
#define AVR_LIRC_BINARY 1
//#undef AVR_LIRC_BINARY
#endif
#define PULSESPACEINDEX // try 20180915 psiNibbles
#ifndef PULSESPACEINDEX
// Old NodoDueRkr
// uint pulseSpaceMicros[RAW_BUFFER_SIZE+4 + RAW_BUFFER_TIMERANGE_SIZE];          // Tabel met de gemeten pulsen in microseconden. eerste waarde is het aantal bits*2
// get
#define PulseSpaceMicros(i)	(pulseSpaceMicros[(i)])
#define PsCount(psmIndexStart) (pulseSpaceMicros[(psmIndexStart)])

// set
#define PulseSpaceMicrosSet(i, value)	((pulseSpaceMicros[(i)]) = (value))
#define PsCountSet(psmIndexStart, value) PulseSpaceMicrosSet((psmIndexStart), (value))
#define PsCountSetS(psmIndexStart, value, s) PsCountSet(psmIndexStart, value)
#else
/* Use pulsespaceindex.h like rfm69-ook-receive-dio2.ino processBitRkr
 * processBit: processBitRkr(pulse_dur, signal, rssi)
 * processBitRkr(pulse_dur, (i + 1) % 2, 1);
 * Force end: processBitRkr(1, 0, 0);
 *
 * Interface to existing NodoDueRkr code is
 *
 *	PulseSpaceMicrosSet: next pulse/sparce via processBitRkr
 * 	PsCountSetS: extra calls in old situation for inter/inra message processing and signalling ready
 *  PsCountSet: unused for signal generation?
 */
#define EDGE_TIMEOUT 60000 // was 10000
#include "pulsespaceindex.h"
#if 1
#define PulseSpaceMicrosSet(i, value)
#else
#define PulseSpaceMicrosSet(i, value)	((void)processBitRkr(value, (i)-1, 1))
#endif

typedef enum {pscsData = 0, pscsIntraTime = 1, pscsReady = 2, pscsPsmCount = 3, pscsInterTimeout = 4} pscS; //
void PsCountSetS(uint psmIndexStart, uint value, pscS s) {
#if 0
	if ((s == pscsPsmCount) || (s == pscsInterTimeout)) {
		return; // inter message pulse count, not used in PULSESPACEINDEX
	}
#if 1
	if (/*(psiCount & 1) && */psmIndexStart > 0 && s == pscsIntraTime && value > 1) {
		//Serial.println(F("PsCountSet fix"));
		PulseSpaceMicrosSet(0, value);
	}
#endif
#if 0
	if (psiCount > 2) {
		Serial.print(F("PsCountSet: "));
		psiPrintComma(psmIndexStart, 'I', 1);
		psiPrintComma(value, 'V', 1);
		psiPrintComma(s, 'S', 1);
		psiPrintComma(psiCount, 'C', 1);
		Serial.println();
	}
#endif
	if ((psiCount > 1) && value == 0 && s == pscsReady) {
		processBitRkr(1, 0, 0);	// terminate signal by 'no rssi'.
	}
#endif
}

#define PsCountSet(psmIndexStart, value) PsCountSetS(psmIndexStart, value, pscsData)

#define PulseSpaceMicros(i)	(psiNibblePS(psiNibbles,(i)-1))
#define PsCount(psmIndexStart) (psiCount * 2)
#endif