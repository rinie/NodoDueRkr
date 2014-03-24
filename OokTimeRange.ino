/*
 * Based on RawSignal_2_32bit you can derive a lot of signal properties by looking at
 * Min/Max times of pulses/spaces and Pulse + Space times
 *
 * Time variations within a timerange are typically less than 10%
 * Time variantions between different ranges are much larger
 *
 * E.g. KAKU: Pulse [296-326/998-1012], Space [207-226/899-929], Pulse+Space: [1219-1229],
 *            no sync, total time 29731, 50 #P/S, 7+ repetitions
 * ..NewKAKU: Pulse  [179-198], Space [162-199/1174-1207], Pulse+Space: [356-386/1367-1395],
 *            Sync pulse + space 2750, total time 58748, 132 #P/S 3+ repetitions
 *
 *..X10 RF remote 0546:
 *            Pulse  [467-479], Space [465-477/1575-1583], Pulse+Space: [935-952/2046-2054],
 *            Sync pulse + space 4320, total time 31457, 44 #P/S,  5+ repetitions
 *
 * AGC preamble, sync, data
 * Sync, preamble?, data, checksum
 * preamble, data, checksum...
 */

typedef enum IXPS {
	ixPulse, ixSpace, ixPulseSpace
} IXPS;

struct OokProperties {
	uint iTime; // psmIndexStart: index in pulseSpaceMicros array for pulse/space times
	uint iTimeEnd; //  xEnd / pulseSpaceMicros[psmIndexStart] + psmIndexStart
	uint iTimeRange[3];// pulse, space, pulse + space
#if 1
	ulong lastHash;
	uint lastLength;
#endif
} Ook;

#define Ook_NrTimeRanges(x)  (pulseSpaceMicros[Ook.iTimeRange[x]] / 2)

void PrintDash(void)
  {
  PrintChar('-');
  }

#undef RKRMINMAX_VERBOSE
#define RKRRANGEANALYSE_VERBOSE
void PrintComma(void)
  {
  Serial.print(F(", "));
  }

void PrintNum(uint x, char c=0, uint digits=4) {
     // Rinie add space for small digits
     if(c) {
     	PrintChar(c);
 	}
	for (uint i=1, val=10; i < digits; i++, val *= 10) {
		if (x < val) {
			PrintChar(' ');
		}
	}

    Serial.print(x,DEC);
}

void PrintNumHex(uint x, char c, uint digits) {
	// Rinie add space for small digits
	if(c) {
		PrintChar(c);
	}
	for (uint i=1, val=16; i < digits; i++, val *= 16) {
		if (x < val) {
			PrintChar('0');
		}
	}

	Serial.print(x,HEX);
}

// todo: add byte/nibble length, stopBit
void RkrPreAmbleAnalyse(boolean fBitWise, boolean fSkipEven) {
	uint iTimeSplitPulse = (pulseSpaceMicros[Ook.iTimeRange[ixPulse] + 2] + pulseSpaceMicros[Ook.iTimeRange[ixPulse] + 3]) / 2; //max Short + min long /2
	uint iTimeSplitSpace = (pulseSpaceMicros[Ook.iTimeRange[ixSpace] + 2] + pulseSpaceMicros[Ook.iTimeRange[ixSpace] + 2]) / 2; //max Short + min long / 2
	uint xEnd = Ook.iTimeEnd;
	boolean isLongPreamblePulse = pulseSpaceMicros[Ook.iTime + 3] > iTimeSplitPulse;
	boolean isLongPreambleSpace = pulseSpaceMicros[Ook.iTime + 4] > iTimeSplitSpace;
	boolean isLongPrev;
	uint iPreamble = 0;
	uint iBits = 0;
	byte	byteVal= 0;
	byte prevBit;
	//uint	byteValMSB= 0;
	uint x;

	// 0=countl, 1=startpulse, 2=space after startpulse, 3=1st data pulse
	// preamble: long until short or short until long...
	for (x = 1 + Ook.iTime; x <= xEnd-1; x++) {
		// pulse
		if (isLongPreamblePulse != (pulseSpaceMicros[x] > iTimeSplitPulse)) {
			break;
		}
		iPreamble++;
		x++;
		// space
		if (isLongPreambleSpace != (pulseSpaceMicros[x] > iTimeSplitSpace)) {
			break;
		}
		iPreamble++;
 	}
	uint iSync = 0;
	uint iTimeSplitSync = max(pulseSpaceMicros[Ook.iTimeRange[ixPulse] + 4], pulseSpaceMicros[Ook.iTimeRange[ixSpace] + 4]); // longer than max long
	for (;x <= xEnd-1; x++) {
		if (pulseSpaceMicros[x] <= iTimeSplitSync) {
			break;
		}
		iSync++;
	}
//	x++;
	// old
	boolean isLongPreamble = isLongPreamblePulse && isLongPreambleSpace;
	uint iTimeSplit = max(iTimeSplitPulse, iTimeSplitSpace);
	Serial.print(F("!PreAmble"));
	Serial.print((fSkipEven) ? F("Skip ") : ((iSync > 0) ? F("Sync ") : ((isLongPreamble) ? F("Long "): F("Short "))));

	PrintNum(iPreamble, 0, 1);
	PrintNum(pulseSpaceMicros[Ook.iTime] - iPreamble - iSync, ' ', 1);
	prevBit = (isLongPreamble) ? 1: 0; // last long was a 1
	isLongPrev = (isLongPreamble) ? false: true; // stop on short that we skipped
	PrintChar(':');

	// assume rest is manchester encoded
	for (;x <= xEnd-1; x++) {
		boolean isLong = pulseSpaceMicros[x] > iTimeSplit;
		if ((!isLong && !isLongPrev) || isLong) {  // double short or long
			if (isLong) {
				prevBit = (prevBit) ? 0 : 1;
			}
			if ((!fSkipEven) || ((iBits%2) == 1)) {
				if (fSkipEven) {
					byteVal = (byteVal >> 1) | (prevBit ? 0x80 : 0);
				}
				else {
					byteVal = (byteVal << 1) | (prevBit ? 1 : 0);
				}
				if (fBitWise) {
					PrintNumHex(prevBit, ((fSkipEven && (iBits == 1)) || (iBits == 0)) ? ' ' : 0, 0);
				}
			}
			iBits++;
			if ((!fSkipEven && (iBits >=8)) || (iBits >= 16)) {
				iBits = 0;
				if (!fBitWise) {
					if (fSkipEven) { // swap nibbles
						byte bbyteVal = ((byteVal) & 0x0F) & ((byteVal >> 4) & 0x0F);
						PrintNumHex(bbyteVal, ' ', 2);
					}
					else {
						PrintNumHex(byteVal, ' ', 2);
					}
				}
				byteVal = 0;
			}
		}
		isLongPrev = isLong;
	}
	if ((!fBitWise) && (byteVal != 0)) {
		if (fSkipEven) { // swap nibbles
			PrintNumHex(((byteVal) & 0x0F), ' ', 1);
			PrintNumHex(((byteVal >> 4) & 0x0F), ' ', 1);
		}
		else {
			PrintNumHex(byteVal, ' ', 2);
		}
	}
	PrintTermRaw();
}

// 938-952/2043-2053
void RkrSyncPulseSpaceAnalyse(boolean fBitWise) {
	uint iTimeSplit = (pulseSpaceMicros[Ook.iTimeRange[ixPulseSpace] + 2] + pulseSpaceMicros[Ook.iTimeRange[ixPulseSpace] + 3]) / 2; // max short pulse + space
	uint xEnd = Ook.iTimeEnd;
	uint iBits = 0;
	byte byteValLSB= 0;
	uint x;

	Serial.print(F("!SyncP+S"));
	PrintNum(iTimeSplit, ' ', 4);
	PrintChar(':');
	// 0=countl, 1=startpulse, 2=space after startpulse, 3=1st data pulse
	for (x = 3 + Ook.iTime; x <= xEnd-1; x+=2) {
		boolean isLong = (pulseSpaceMicros[x] + pulseSpaceMicros[x + 1]) > iTimeSplit;
		byteValLSB = (byteValLSB << 1) | ((isLong) ? 0x01 : 0);
		if (fBitWise) {
			PrintNumHex(((isLong) ? 1 : 0), ((iBits == 0)) ? ' ' : 0, 0);
		}
		iBits++;
		if (iBits >=8) {
			iBits = 0;
			if (!fBitWise) {
				PrintNumHex(byteValLSB, ' ', 2);
			}
			byteValLSB = 0;
		}
	}
	PrintTermRaw();
}

byte RkrTimeRangeAnalyse() {
	uint iTimeRangePulse = RAW_BUFFER_TIMERANGE_START;
	uint iTimeRangeSpace = iTimeRangePulse + pulseSpaceMicros[iTimeRangePulse] + 1;
	uint iTimeRangePulseSpace = iTimeRangePulseSpace = iTimeRangeSpace + pulseSpaceMicros[iTimeRangeSpace] + 1;
	uint cPulseRanges = pulseSpaceMicros[iTimeRangePulse]/2;
	uint cSpaceRanges = pulseSpaceMicros[iTimeRangeSpace]/2;
	uint cPulseSpaceRanges = pulseSpaceMicros[iTimeRangePulseSpace]/2;
	uint iMatchRanges = 0;
#ifdef RKRRANGEANALYSE_VERBOSE
	PrintNum(cPulseRanges, 0, 1);
	PrintNum(cSpaceRanges, ',', 1);
	PrintNum(cPulseSpaceRanges, ',', 1);
#endif
	if (cPulseRanges <= 1 || cSpaceRanges <= 1) {
			return 2; // Pulse Or Space single range: don't bother just use pulse+space length
	}
	for (uint iPulse = iTimeRangePulse + 1; iPulse <= iTimeRangePulse + pulseSpaceMicros[iTimeRangePulse]; iPulse += 2) {
		int MinPulse = pulseSpaceMicros[iPulse];
		int MaxPulse = pulseSpaceMicros[iPulse + 1];
		for (uint iSpace = iTimeRangeSpace + 1; iSpace <= iTimeRangeSpace + pulseSpaceMicros[iTimeRangeSpace]; iSpace += 2) {
			int MinSpace = pulseSpaceMicros[iSpace];
			int MaxSpace = pulseSpaceMicros[iSpace + 1];
#if 0
			PrintNum(MinSpace, ',', 1);
			PrintNum(abs((MinPulse - MinSpace)), ',', 1);
			PrintNum(abs((MaxPulse - MaxSpace)), ',', 1);
#endif
			// pulse length same range as space length?
			if (((abs((MinPulse - MinSpace))) < 100) && (abs(((MaxPulse - MaxSpace))) <100)) {
				pulseSpaceMicros[iPulse] = min(MinPulse, MinSpace);
				pulseSpaceMicros[iSpace] = min(MinPulse, MinSpace);
				pulseSpaceMicros[iPulse+1] = max(MaxPulse, MaxSpace);
				pulseSpaceMicros[iSpace+1] = max(MaxPulse, MaxSpace);
				iMatchRanges++;
			}
		}
	}
#ifdef RKRRANGEANALYSE_VERBOSE
	PrintNum(iMatchRanges, ',', 1);
#endif
	// ? P+S constant: don't print p+s
	return (cPulseSpaceRanges <= 1) ? 3: 1;
}

void RkrTimeRangeReplaceMedian(uint Min, uint Max, uint Median, int What) {
//	int x = 5 + Ook.iTime;
	uint xEnd = Ook.iTimeEnd;
	if (What > 1) {
		return;
	}
	// 0=countl, 1=startpulse, 2=space after startpulse, 3=1st data pulse
	for (uint x = 1 + Ook.iTime; x <= xEnd-1; x+=2) {
		uint value = pulseSpaceMicros[x + What];
		if (value >= Min && value <= Max) {
				pulseSpaceMicros[x + What] = Median;
		}
	}
}

#define RKR_MEDIANROUNDING
#define RKR_ROUGHSYNC // clear sync or assume same range as first pulse/space

#ifdef 	RKR_MEDIANROUNDING	// Try Median rounding
uint RkrRoundTime(uint Median) {
	if (Median > 2000) { // round 100
		Median = ((Median + 50) / 100) * 100;
	}
	else if (Median > 100) { // round 10
		Median = ((Median + 5) / 10) * 10;
	}
	return Median;
}
#endif

void RkrTimeRangePsReplaceMedian() {
	uint iTimeRange = RAW_BUFFER_TIMERANGE_START;
	for (int What = 0; What < 2; What++) {
		int i=1;
		int iEnd=pulseSpaceMicros[iTimeRange];
		for (i = 1; i < iEnd; i+=2) {
				uint Min = pulseSpaceMicros[iTimeRange + i];
				uint Max = pulseSpaceMicros[iTimeRange + i + 1];
				uint Median = (Min + Max) / 2;
#ifdef 	RKR_MEDIANROUNDING	// Try Median rounding
				Median = RkrRoundTime(Median);
#endif
				RkrTimeRangeReplaceMedian(Min, Max, Median, What);
		}
#ifdef 	RKR_ROUGHSYNC
		if (pulseSpaceMicros[Ook.iTime + 1 + What] * 2 <= pulseSpaceMicros[Ook.iTime + 3 + What] * 3) { // 1.5 sync
			pulseSpaceMicros[Ook.iTime + 1 + What] = pulseSpaceMicros[Ook.iTime + 3 + What];
		}
#endif
#ifdef 	RKR_MEDIANROUNDING	// Rounding sync/preamble as well
		pulseSpaceMicros[Ook.iTime + 1 + What] = RkrRoundTime(pulseSpaceMicros[Ook.iTime + 1 + What]);
		pulseSpaceMicros[Ook.iTime + 3 + What] = RkrRoundTime(pulseSpaceMicros[Ook.iTime + 3 + What]);
#endif
		iTimeRange += pulseSpaceMicros[iTimeRange] + 1;
	}
}

// called from RawSignal_2_32bit, that is called by PrintPulseSpaceTiming...
//int RkrTimeRange(uint iTime, uint iTimeRange, int What) {
int RkrTimeRange(uint MinTime, uint MaxTime, byte What) {
	uint i=1;
	uint iEnd = 2;
	uint iTimeRange = (What == 0) ? RAW_BUFFER_TIMERANGE_START : Ook.iTimeRange[What - 1] + pulseSpaceMicros[Ook.iTimeRange[What - 1]] + 1;
	Ook.iTimeRange[What] = iTimeRange;
	pulseSpaceMicros[iTimeRange] = iEnd;
	pulseSpaceMicros[iTimeRange+1] = MinTime;
	pulseSpaceMicros[iTimeRange+2] = MaxTime;

	if (pulseSpaceMicros[Ook.iTime] < 10) {
		return 0;
	}
	if (Ook.iTime + pulseSpaceMicros[Ook.iTime] >= iTimeRange) {
		PrintNum(Ook.iTime,0, 3);
		PrintNum(Ook.iTime + pulseSpaceMicros[Ook.iTime],',', 3);
		PrintNum(iTimeRange,',', 3);
		Serial.print(F("!RkrTimeRange Overflow!\n"));
		return 0;
	}

	while ((i < iEnd) && (iEnd < (RAW_BUFFER_TIMERANGE_SIZE - 2))) {
		uint Min=pulseSpaceMicros[iTimeRange + i + 1];
		uint Max=pulseSpaceMicros[iTimeRange + i + 0]; //RKR

		if (i > 16) {
			return iEnd;
		}
		if ((Min > Max) && ((Min-Max) > 200)) {
			uint Median=Max + (((Min-Max) > 400) ? 200 : ((Min-Max)/2)); // currently inverted min/max
			uint x = 5 + Ook.iTime;
			uint xEnd = Ook.iTimeEnd;

			if (i > 12 || (xEnd >= RAW_BUFFER_SIZE+2)) {
				break;
			}
		//	uint MinCount =0;
		//	uint MaxCount =0;

			// Kleinste, groter dan mid
			// Grootste, kleinder dan mid
			// diff > x?
			// dan replace by median
			// zoek de kortste tijd (PULSE en SPACE)
			// 0=countl, 1=startpulse, 2=space after startpulse, 3=1st data pulse
			// RKR try 3 instead of 5 for starters
			for (x = 3 + Ook.iTime; x <= xEnd-4; x+=2) {
				uint value = ((What == 0) ? pulseSpaceMicros[x] // pulse
									   : ((What == 1) ? pulseSpaceMicros[x + 1] //space
													:  (pulseSpaceMicros[x] + pulseSpaceMicros[x + 1]))); // pulse + space
				if (value < Min && value >= Median) {
					Min=value; // Zoek naar de kortste pulstijd.
				}
				if (value > Max  && value <= Median) {
					Max=value; // Zoek naar de langste pulstijd.
				}
			}
			// at least one new value found: extend with 2
			if (/* (Min != Max) && */ !((Min==pulseSpaceMicros[iTimeRange + i + 1]) && (Max==pulseSpaceMicros[iTimeRange + i + 0]))) {
				//return iEnd;
				// extend array with 2
				iEnd += 2;
				pulseSpaceMicros[iTimeRange] = iEnd;
				for (int j = iEnd; j > i+2; j--) {
					pulseSpaceMicros[iTimeRange + j] = pulseSpaceMicros[iTimeRange + j - 2];
				}
				pulseSpaceMicros[iTimeRange + i + 1] = Max;
				pulseSpaceMicros[iTimeRange + i + 2] = Min;
			}
			else {
				i++;
			}
		}
		else {
			i++;
		}
	}
	PrintChar(' ');
	PrintChar('[');
	for(uint j = 1; j <= iEnd; j++) {
		if (j > 1) {
			if (j%2==0) {
				PrintChar('-');
			}
			else {
				PrintChar('/');
			}

		}
		Serial.print(pulseSpaceMicros[iTimeRange+j],DEC);
	}
	PrintChar(']');
	return iEnd;
}


int RkrPrintTimeRangeStats(byte ixPs, uint StartTime, uint MinTime, uint MaxTime, uint count, ulong hash) {
		PrintTermRaw();

		Serial.print((ixPs == ixPulseSpace) ? F("!RawP+S") : ((ixPs== ixPulse)? F("!RawP  "):F("!RawS  ")));
		PrintNum(StartTime, ' ', 4); // start space/preamble
		PrintNum(MinTime, ',', 4);
		PrintNum(MaxTime, ',', 4);
		PrintNum(MaxTime-MinTime, ',', 4);
		PrintNum(count, ',', 4);
		PrintComma();
		PrintValue(hash);
		RkrTimeRange(MinTime, MaxTime, ixPs); // // Pulse + Space
}

/*
 * Transfer point From Nodo_Due code into OokTimeRangeCode
 *
 *	pulseSpaceMicros[Ook.iTime] contains start of Nodo measured timings
 *	Ook.iTime == 0: original nodo code
 *	              != 0: measure repetitions before decoding...
 *
 */
void PrintPulseSpaceTimingOokTimeRange(uint iTime, boolean fIsRf) {
	uint x, xEnd;
	uint i;
	byte iPrintPulseAndSpace = 3; // normal 3, now 0 to show only results
	byte iPrintPulseAndSpaceRounded = 3;
	Ook.iTime = iTime;
	Ook.iTimeEnd = pulseSpaceMicros[iTime] + iTime;
	xEnd = Ook.iTimeEnd;
	int iTimeRange = RAW_BUFFER_TIMERANGE_START;
	ulong hash;
	pulseSpaceMicros[iTimeRange] = 0;
	Ook.iTimeRange[ixPulse] = iTimeRange;
	Ook.iTimeRange[ixSpace] = iTimeRange;
	Ook.iTimeRange[ixPulseSpace] = iTimeRange;
	//total time
	if ((Ook.iTime > RAW_BUFFER_SIZE+2) || (Ook.iTimeEnd > RAW_BUFFER_SIZE+2)) {
		PrintNum(Ook.iTime,0, 3);
		PrintNum(Ook.iTimeEnd,',', 3);
		Serial.print(F("!PrintPulseSpaceTiming Overflow\n!"));
		return;
	}
	i = 0;
	for(int x=1+Ook.iTime;x<=xEnd;x++) {
		i += pulseSpaceMicros[x];
	}

	if (i <= 0) {
		return;
	}
	hash = AnalyzeRawSignal(Ook.iTime);
	if (Ook.iTime <= 0) { // first signal/no repetition yet
		//PrintEventCode(AnalyzeRawSignal(0));
		//PrintTerm();
		//PrintTermRaw();
		Serial.print(fIsRf ? F("!Rf") : F("!Ir"));
		Serial.print(F("* "));
		// inter message time
		PrintNum(psStartSignalMillis - psStartSignalMillisLast,0, 5);
		psStartSignalMillisLast = psStartSignalMillis;
	}
	else {
#if 1
		if (hash != Ook.lastHash && xEnd-Ook.iTime < Ook.lastLength) {
			return;
		}
#endif
		Serial.print(fIsRf ? F("!Rf") : F("!Ir"));
		if (hash == Ook.lastHash && xEnd-Ook.iTime == Ook.lastLength) {
			Serial.print(F("+ ")); // last = current, so likely good match!
		}
		else {
			Serial.print(F("- "));
		}
		// intra message
		PrintNum(pulseSpaceMicros[xEnd+1],0, 5);
	}
#if 1
	Ook.lastHash = hash;
	Ook.lastLength = xEnd-Ook.iTime;
#endif
//	PrintComma();
	//total time
	i = 0;
	for(int x=1+Ook.iTime;x<=xEnd;x++) {
		i += pulseSpaceMicros[x];
	}
	PrintNum(i,',', 5);
//	PrintComma();

	// count
	PrintNum(pulseSpaceMicros[Ook.iTime],',', 2);
//	PrintComma();

	// net count min spikes
	i = 0;
	for(int x=1+Ook.iTime;x<=xEnd;x++) {
		if (pulseSpaceMicros[x] < 100) {
			i++;
		}
	}

	PrintNum(pulseSpaceMicros[Ook.iTime]-i,',', 0);
//	PrintComma();

	PrintNum(i,',', 0);
	PrintComma();

	PrintEventCode(hash);
	// todo print min/max and minButOne/maxButOne
	RawSignal_2_32bit(Ook.iTime, true);
	PrintTermRaw();
	for (i=0; i < 2; i++) {
		//  PrintText(Text_07,false);
		if (iPrintPulseAndSpace != 0) {
			for(int x=1+Ook.iTime;x<=xEnd;x++) {
				if ((x - (1+Ook.iTime))%16==0) {
						if ((x - (1+Ook.iTime))> 0) {
							PrintTerm();
						}
						PrintNum(x - (1+Ook.iTime), 0, 4);
						PrintChar(':');
						if (iPrintPulseAndSpace & 1) { // mark, space
							PrintNum(pulseSpaceMicros[x], 0, 4);
						}
				}
				else {
					if (iPrintPulseAndSpace & 1) { // mark, space
						PrintNum(pulseSpaceMicros[x], ',', 4);
					}
				}
				if (iPrintPulseAndSpace & 2) { // mark + space
					if ((x - (1+Ook.iTime))%2==1) {
						Serial.print(F(" ["));
						PrintNum(pulseSpaceMicros[x] + pulseSpaceMicros[x-1], 0, 4);
						PrintChar(']');
					}
				}

			}
			PrintTerm();
		}
		if (i == 0) {
			if (pulseSpaceMicros[iTimeRange] != 0) {
				Serial.print(F("!Rounded "));
				iPrintPulseAndSpaceRounded = RkrTimeRangeAnalyse();
				RkrTimeRangePsReplaceMedian();
				PrintTermRaw();
				if (iPrintPulseAndSpace != 0) {
					iPrintPulseAndSpace = iPrintPulseAndSpaceRounded;
				}
				if (iPrintPulseAndSpace == 2 || iPrintPulseAndSpace == 0) {
					break;
				}
			}
			else {
				i = 2;
			}
		}
	}
	if (iPrintPulseAndSpace == 2) {
		if (iPrintPulseAndSpace == 2) {
			for(int x=1+Ook.iTime;x<=xEnd;x++) {
				if ((x - (1+Ook.iTime))%16==0) {
						if (((x - (1+Ook.iTime))/2)>0) {
							PrintTerm();
						}
						PrintNum((x - (1+Ook.iTime))/2, 0, 4);
						PrintChar(':');
				}
				if ((x - (1+Ook.iTime))%2==1) {
					Serial.print(F(" ["));
					PrintNum(pulseSpaceMicros[x] + pulseSpaceMicros[x-1], 0, 4);
					PrintChar(']');
				}
			}
		}
		PrintTermRaw();
	}
	// Preamble experiment 2,2,3
	if ((Ook_NrTimeRanges(ixPulseSpace)>=3) && ((Ook_NrTimeRanges(ixPulse)>=2) && (Ook_NrTimeRanges(ixSpace)>=2))) { // preamble/manchester like
		if (pulseSpaceMicros[iTime] <= 180) {
			RkrPreAmbleAnalyse(false, false);
			RkrPreAmbleAnalyse(true, false);
		}
		else {// oregon scientific v2 protocol?
			RkrPreAmbleAnalyse(false, false);
			//RkrPreAmbleAnalyse(false, true); does not work!
			//RkrPreAmbleAnalyse(true, true);
		}
	}
	// 1,2,2 or 2,1,2 signals: double min/max pairs...
	if ((Ook_NrTimeRanges(ixPulseSpace)==2) && ((Ook_NrTimeRanges(ixPulse)==1) || (Ook_NrTimeRanges(ixSpace)==1))) { // x10 like
		RkrSyncPulseSpaceAnalyse(false); // seems ok for 44 X10...
		RkrSyncPulseSpaceAnalyse(true); // seems ok for 44 X10...
	}
}


void PrintPulseSpaceTimingAvrLirc(uint iTime) {
	uint x, xEnd;
	uint i;

	Ook.iTime = iTime;
	Ook.iTimeEnd = pulseSpaceMicros[iTime] + iTime;
	xEnd = Ook.iTimeEnd;
	int iTimeRange = RAW_BUFFER_TIMERANGE_START;
	pulseSpaceMicros[iTimeRange] = 0;
	Ook.iTimeRange[ixPulse] = iTimeRange;
	Ook.iTimeRange[ixSpace] = iTimeRange;
	Ook.iTimeRange[ixPulseSpace] = iTimeRange;
	i = 0;
	for(int x=1+Ook.iTime;x<=xEnd;x++) {
		i += pulseSpaceMicros[x];
	}

	if (i <= 0) {
		return;
	}
	emit_pulse_data(1+Ook.iTime, xEnd); // use AvrLirc code...
}

void PrintPulseSpaceTimingAnalysIr(uint iTime) {
	uint x, xEnd;
	uint i;
	Ook.iTime = iTime;
	Ook.iTimeEnd = pulseSpaceMicros[iTime] + iTime;
	xEnd = Ook.iTimeEnd;
	int iTimeRange = RAW_BUFFER_TIMERANGE_START;
	pulseSpaceMicros[iTimeRange] = 0;
	Ook.iTimeRange[ixPulse] = iTimeRange;
	Ook.iTimeRange[ixSpace] = iTimeRange;
	Ook.iTimeRange[ixPulseSpace] = iTimeRange;
	i = 0;
	for(int x=1+Ook.iTime;x<=xEnd;x++) {
		i += pulseSpaceMicros[x];
	}

	if (i <= 0) {
		return;
	}
	reportPulses(1+Ook.iTime, xEnd); // use AnalysIR code...
}

#ifdef JEELABS_DECODERS
void PrintPulseSpaceTimingJeeLabsOok(uint iTime) {
	uint x, xEnd;
	uint i;
	Ook.iTime = iTime;
	Ook.iTimeEnd = pulseSpaceMicros[iTime] + iTime;
	xEnd = Ook.iTimeEnd;
	int iTimeRange = RAW_BUFFER_TIMERANGE_START;
	pulseSpaceMicros[iTimeRange] = 0;
	Ook.iTimeRange[ixPulse] = iTimeRange;
	Ook.iTimeRange[ixSpace] = iTimeRange;
	Ook.iTimeRange[ixPulseSpace] = iTimeRange;
	i = 0;
	for(int x=1+Ook.iTime;x<=xEnd;x++) {
		i += pulseSpaceMicros[x];
	}

	if (i <= 0) {
		return;
	}
	jeeLabsOokOsv2(1+Ook.iTime, xEnd); // use AnalysIR code...
}
#endif

void PrintPulseSpaceTiming(uint iTime, boolean fIsRf) {
	if (settings.Mode != omLirc) {
		PrintPulseSpaceTimingOokTimeRange(iTime, fIsRf);
#ifdef JEELABS_DECODERS
		PrintPulseSpaceTimingJeeLabsOok(iTime);
#endif
	}
	if (settings.Mode == omAnaysIR) {
		Serial.print(F("!AnalysIr!"));
		PrintPulseSpaceTimingAnalysIr(iTime);
	}
	if (settings.Mode == omLirc) {
		// Serial.print(F("!Avr!"));
		PrintPulseSpaceTimingAvrLirc(iTime);
		PrintTerm();
	}
}

void PrintPulseSpaceTimingEnd() {
	if (settings.Mode !=omLirc) {
		Serial.print(F("!End"));
		PrintTermRaw();
	}
}


 /**********************************************************************************************\
 * Voert alle relevante acties in de eventlist uit die horen bij het binnengekomen event
 * Doorlopen van een volledig gevulde eventlist duurt ongeveer 15ms inclusief printen naar serial
 * maar exclusief verwerking n.a.v. een 'hit'.
 \*********************************************************************************************/
void PrintPulseSpaceTimings(ulong Hash, boolean fIsRf) {
	digitalWrite(MonitorLedPin,HIGH);           // LED aan als er iets verwerkt wordt
	// RAWSIGNAL_MULTI
	for(int psmStart = 0; (pulseSpaceMicros[psmStart] != 0) && (psmStart < RAW_BUFFER_SIZE); psmStart = psmStart + pulseSpaceMicros[psmStart] + 2) {
		PrintPulseSpaceTiming(psmStart, fIsRf);
	}
	PrintPulseSpaceTimingEnd();
}
