#define RAWSIGNAL_TOGGLE	// RKR instead of just one rawsignal, repeat until you send RawSignalGet; again...
#define RAWSIGNAL_MULTI		// RKR Rawsignal can be multiple signals: <count> <timing data> <count> <timing data> etc...

#undef JEELABS_DECODERS
#ifndef JEELABS_DECODERS
#define RAW_BUFFER_SIZE            472 // Maximaal aantal te ontvangen bits*2
#else
#define RAW_BUFFER_SIZE            236 // Maximaal aantal te ontvangen bits*2
#endif
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