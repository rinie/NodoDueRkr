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

typedef enum IXPS {	// ixPulse 0 = pulse timings, ixSpace = 1 space timings, ixPulseSpace = 2 for Pulse + Space timings
	ixPulse, ixSpace, ixPulseSpace
} IXPS;

struct OokProperties {
	uint iTime; // psmIndexStart: index in pulseSpaceMicros array for pulse/space times
	uint iTimeEnd; //  xEnd / pulseSpaceMicros(psmIndexStart) + psmIndexStart
	uint iTimeRange[3];// pulse, space, pulse + space
		// repeated signal detection:
	ulong lastHash;
	uint lastLength;
	ulong lastTotalTime;
	byte nRepeats;
} Ook;

#define OokNrTimeRanges(ixps)  (PulseSpaceMicros(Ook.iTimeRange[(ixps)]) / 2)

void PrintDash(void)
  {
  PrintChar('-');
  }

void PrintComma(void)
  {
  Serial.print(F(", "));
  }

void PrintNum(uint x, char c, int digits) {
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

/*
 * RkrManchesterAnalysis
 *
 * To save memory use this twice
 * First with fPrint == false to check if this is a Manchester code (positive return value).
 * Second wiht fPrint true to print bytewise or bitwise
 * To check the Oregon Scientific V2 sensor I have also enable fDoubleInvertedBits
 */
int RkrManchesterAnalysis(boolean fPrint, boolean fBitWise, boolean fDoubleInvertedBits) {
	uint iTimeSplitPulse = (PulseSpaceMicros(Ook.iTimeRange[ixPulse] + 2) + PulseSpaceMicros(Ook.iTimeRange[ixPulse] + 3)) / 2; //max Short + min long /2
	uint iTimeSplitSpace = (PulseSpaceMicros(Ook.iTimeRange[ixSpace] + 2) + PulseSpaceMicros(Ook.iTimeRange[ixSpace] + 2)) / 2; //max Short + min long / 2
	boolean lsbFirst = fDoubleInvertedBits;
	uint xEnd = Ook.iTimeEnd;
	uint xStart = 1 + Ook.iTime;
	uint x;
	uint iTimeSplit = max(iTimeSplitPulse, iTimeSplitSpace);
	// Atmel http://www.atmel.com/Images/doc9164.pdf
	// Timing Based Manchester Decode
	// find start of long->short crossing
	for (x = xStart; x <= xEnd-1; x++) {
		boolean isLong = PulseSpaceMicros(x) > iTimeSplit;
		if (!isLong) {
			break;
		}
	}
	if (fPrint && fBitWise) {
		PrintChar(' ');
	}
	byte prevBitVal;
	boolean fSecondBit = false;
	byte bitVal = ((x - xStart) & 1) ? 1 : 0; // start with Pulse == 1
	byte byteVal= 0;
	uint iBits = 0;
	uint byteCount = 0;
	for (;x <= xEnd-1; x++) {
		// Compare stored count value with T
		boolean isLong = PulseSpaceMicros(x) > iTimeSplit;
		if (!isLong) { // T
			// Capture next edge and make sure this value also = T (else error)
			if ((x+1 <= xEnd-1) && !(PulseSpaceMicros(x+1) > iTimeSplit)) {
				x++;
				// Next bit = current bit
				bitVal = (bitVal) ? 1 : 0;
				if (fDoubleInvertedBits) {
					if (fSecondBit) {
						if (bitVal == prevBitVal) {
							break; // no DoubleInvertedBits
						}
						fSecondBit = false;
					}
					else {
						prevBitVal = bitVal;
						fSecondBit = true;
						continue; // only second bit is used for value
					}
				}
				// Return next bit
				if (fPrint && fBitWise) {
					PrintNumHex(bitVal, 0, 0);
				}
				if (!lsbFirst) {
					byteVal = (byteVal << 1) | ((bitVal) ? 1 : 0);
				}
				else {
					byteVal = (byteVal >> 1) | ((bitVal) ? 0x80 : 0);
				}
				iBits++;
				if (iBits>= 8) {
					if (fPrint) {
						if (!fBitWise) {
							PrintNumHex(byteVal, ' ', 2);
						}
						else {
							PrintChar(' ');
						}
					}
					iBits = 0;
					byteVal = 0;
					byteCount++;
				}
			}
			else {
				break;
			}
		}
		else { // 2T
			// Next bit = opposite of current bit
			bitVal = (bitVal) ? 0 : 1;
			if (fDoubleInvertedBits) {
				if (fSecondBit) {
					if (bitVal == prevBitVal) {
						break; // no DoubleInvertedBits
					}
					fSecondBit = false;
				}
				else {
					prevBitVal = bitVal;
					fSecondBit = true;
					continue; // only second bit is used for value
				}
			}
			// Return next bit
			if (fPrint && fBitWise) {
					PrintNumHex(bitVal, 0, 0);
			}
			if (!lsbFirst) {
				byteVal = (byteVal << 1) | ((bitVal) ? 1 : 0);
			}
			else {
				byteVal = (byteVal >> 1) | ((bitVal) ? 0x80 : 0);
			}
			iBits++;
			if (iBits>=8) {
				if (fPrint) {
					if (!fBitWise) {
						PrintNumHex(byteVal, ' ', 2);
					}
					else {
						PrintChar(' ');
					}
				}
				iBits = 0;
				byteVal = 0;
				byteCount++;
			}
		}
	}
	//some sanity check, end may be garbled
	// 8 or 4 may be too strict
	if ((x < xEnd - 3) && (byteCount < 4)) {
		return -byteCount;
	}

	if (iBits > 0) {
		if (fPrint && !fBitWise) {
			PrintNumHex(byteVal, ' ', 2);
		}
	}
	return byteCount;
}

/*
 * RkrPreAmbleAnalysis
 *
 * Signal starts with a lot of regular pulse/space timings so that the AGC gets tuned.
 * Decode this
 */
int RkrPreAmbleAnalysis(void) {
	uint iTimeSplitPulse = (PulseSpaceMicros(Ook.iTimeRange[ixPulse] + 2) + PulseSpaceMicros(Ook.iTimeRange[ixPulse] + 3)) / 2; //max Short + min long /2
	uint iTimeSplitSpace = (PulseSpaceMicros(Ook.iTimeRange[ixSpace] + 2) + PulseSpaceMicros(Ook.iTimeRange[ixSpace] + 3)) / 2; //max Short + min long / 2
	boolean isLongPreamblePulse = PulseSpaceMicros(Ook.iTime + 3) > iTimeSplitPulse;
	boolean isLongPreambleSpace = PulseSpaceMicros(Ook.iTime + 4) > iTimeSplitSpace;
	uint xEnd = Ook.iTimeEnd;
	uint iPreamble = 0;
	uint iBits = 0;
	//uint	byteValMSB= 0;
	uint x;
	int iManchester;

	// 0=countl, 1=startpulse, 2=space after startpulse, 3=1st data pulse
	// preamble: long until short or short until long...
	for (x = 1 + Ook.iTime; x <= xEnd-1; x++) {
		// pulse
		if (isLongPreamblePulse != (PulseSpaceMicros(x) > iTimeSplitPulse)) {
			break;
		}
		iPreamble++;
		x++;
		// space
		if (isLongPreambleSpace != (PulseSpaceMicros(x) > iTimeSplitSpace)) {
			break;
		}
		iPreamble++;
 	}
 	return iPreamble;
}


/*
 * RkrSyncAnalysis
 * After preamble check pulse/space larger than long
 */
int RkrSyncAnalysis(uint xStart) {
	uint xEnd = Ook.iTimeEnd;
	uint x;
	uint iSync = 0;
	uint iTimeSplitSync = max(PulseSpaceMicros(Ook.iTimeRange[ixPulse] + 4), PulseSpaceMicros(Ook.iTimeRange[ixSpace] + 4)); // longer than max long
	for (x = 1 + Ook.iTime + xStart;x <= xEnd-1; x++) {
		if (PulseSpaceMicros(x) <= iTimeSplitSync) {
			break;
		}
		iSync++;
	}
	return iSync;
}

/*
 * RkrPreAmbleSyncManchesterAnalysis
 *
 * Signal starts with a lot of regular pulse/space timings so that the AGC gets tuned.
 * Decode this and determine if that is followed by a Sync pulse.
 *
 * If the preamble consists of long pulses/spaces it is probably Manchester Encoded
 */
int RkrPreAmbleSyncManchesterAnalysis(boolean fPrint, boolean fBitWise, boolean fDoubleInvertedBits) {
	uint iTimeSplitPulse = (PulseSpaceMicros(Ook.iTimeRange[ixPulse] + 2) + PulseSpaceMicros(Ook.iTimeRange[ixPulse] + 3)) / 2; //max Short + min long /2
	uint iTimeSplitSpace = (PulseSpaceMicros(Ook.iTimeRange[ixSpace] + 2) + PulseSpaceMicros(Ook.iTimeRange[ixSpace] + 3)) / 2; //max Short + min long / 2
	boolean isLongPreamblePulse = PulseSpaceMicros(Ook.iTime + 3) > iTimeSplitPulse;
	boolean isLongPreambleSpace = PulseSpaceMicros(Ook.iTime + 4) > iTimeSplitSpace;
	uint iPreamble = RkrPreAmbleAnalysis();
	uint iSync = RkrSyncAnalysis(iPreamble);
	int rv = ((iPreamble > 0) ? 1 : 0);
	rv |= ((iSync > 0) ? 2 : 0);

	boolean isLongPreamble = isLongPreamblePulse && isLongPreambleSpace;
	//uint iTimeSplit = max(iTimeSplitPulse, iTimeSplitSpace);
	int iManchester = RkrManchesterAnalysis(false, fBitWise, fDoubleInvertedBits);
#if 0
	if (!fPrint) {
		PrintStartRaw(F("ManDebug"));
		PrintNum(iPreamble, ' ', 4);
		PrintNum(iSync, ',', 4);
		PrintNum(iManchester, ',', 4);
		PrintTermRaw();
	}
#endif
	if (iManchester <= 0 && iManchester > -16) {
		return rv;
	}
	rv |= ((iManchester > 0) ? 4 : 0);
	if (fPrint) {
		if (iManchester > 7) {
			PrintStartRaw(F("Manchester"));
		}
		else {
			PrintStartRaw(F("PreAmble"));
		}
		Serial.print((fDoubleInvertedBits) ? F("Skip ") : ((iSync > 0) ? F("Sync ") : ((isLongPreamble) ? F("Long "): F("Short "))));

		PrintNum(iPreamble, 0, 1);
		PrintNum(PsCount(Ook.iTime) - iPreamble - iSync, ' ', 1);
		PrintChar(':');
		if (iManchester > 0) {
			RkrManchesterAnalysis(true, fBitWise, fDoubleInvertedBits);
		}
		else {
			PrintNum(-1*RkrManchesterAnalysis(false, fBitWise, fDoubleInvertedBits), 0, 1);
		}
		PrintTermRaw();
	}
	return rv;
}

/*
 * RkrPulseDistanceAnalysis
 *
 *	P+S has 2 timings, P is constant or S is constant, use P+S for most reliable timings
 */
void RkrPulseDistanceAnalysis(boolean fBitWise, boolean fSyncPs) {
	uint iTimeSplit = (PulseSpaceMicros(Ook.iTimeRange[ixPulseSpace] + 2) + PulseSpaceMicros(Ook.iTimeRange[ixPulseSpace] + 3)) / 2; // max short pulse + space
	uint xEnd = Ook.iTimeEnd;
	uint iBits = 0;
	byte byteValLSB= 0;
	uint x = 1 + Ook.iTime;
	uint xStep = 2;
	if (fSyncPs) {
		PrintStartRaw(F("SyncP+S"));
		PrintNum(iTimeSplit, ' ', 4);
		PrintChar(':');
		x += 2;
	}
	else {
		iTimeSplit = (PulseSpaceMicros(Ook.iTimeRange[ixSpace] + 2) + PulseSpaceMicros(Ook.iTimeRange[ixSpace] + 3)) / 2; // max short pulse + space
		PrintStartRaw(F("PDM    "));
		PrintNum(iTimeSplit, ' ', 4);
		PrintChar(':');
	}

	// 0=countl, 1=startpulse, 2=space after startpulse, 3=1st data pulse
	for (; x <= xEnd-1; x+=(fSyncPs) ? 2:1) {
		boolean isLong =
			(fSyncPs)
			? (PulseSpaceMicros(x) + PulseSpaceMicros(x + 1)) > iTimeSplit
			: (PulseSpaceMicros(x)) > iTimeSplit;

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

/*
 * RkrTimeRangeAnalysis
 *
 * Look in the signal for the number of Pulse/Space and Pulse+Space timeranges
 * Used for rounding the signal.
 * Currently hardcode 100microsecs difference is used
 * TODO: make 100 a setting
 */
byte RkrTimeRangeAnalysis() {
	uint iTimeRangePulse = RAW_BUFFER_TIMERANGE_START;
	uint iTimeRangeSpace = iTimeRangePulse + PulseSpaceMicros(iTimeRangePulse) + 1;
	uint iTimeRangePulseSpace = iTimeRangePulseSpace = iTimeRangeSpace + PulseSpaceMicros(iTimeRangeSpace) + 1;
	uint cPulseRanges = PulseSpaceMicros(iTimeRangePulse)/2;
	uint cSpaceRanges = PulseSpaceMicros(iTimeRangeSpace)/2;
	uint cPulseSpaceRanges = PulseSpaceMicros(iTimeRangePulseSpace)/2;
	byte rv;
	uint iMatchRanges = 0;

	PrintNum(cPulseRanges, 0, 1);
	PrintNum(cSpaceRanges, ',', 1);
	PrintNum(cPulseSpaceRanges, ',', 1);
	if (cPulseRanges <= 1 || cSpaceRanges <= 1) {
		rv = 2; // Pulse Or Space single range: don't bother just use pulse+space length
	}
	else {
		for (uint iPulse = iTimeRangePulse + 1; iPulse <= iTimeRangePulse + PulseSpaceMicros(iTimeRangePulse); iPulse += 2) {
			int MinPulse = PulseSpaceMicros(iPulse);
			int MaxPulse = PulseSpaceMicros(iPulse + 1);
			for (uint iSpace = iTimeRangeSpace + 1; iSpace <= iTimeRangeSpace + PulseSpaceMicros(iTimeRangeSpace); iSpace += 2) {
				int MinSpace = PulseSpaceMicros(iSpace);
				int MaxSpace = PulseSpaceMicros(iSpace + 1);

				// pulse length same range as space length?
				if (((abs((MinPulse - MinSpace))) < 100) && (abs(((MaxPulse - MaxSpace))) <100)) {
					PulseSpaceMicrosSet(iPulse, min(MinPulse, MinSpace));
					PulseSpaceMicrosSet(iSpace, min(MinPulse, MinSpace));
					PulseSpaceMicrosSet(iPulse+1, max(MaxPulse, MaxSpace));
					PulseSpaceMicrosSet(iSpace+1, max(MaxPulse, MaxSpace));
					iMatchRanges++;
				}
			}
		}
		PrintNum(iMatchRanges, ',', 1);
		// ? P+S constant: don't print p+s
		rv = (cPulseSpaceRanges <= 1) ? 3: 1;
	}
#if 0
	// rv=2: P+S, rv=3 p,s, rv=1 P,S, P+S:
	// 01
	// 2: P=S
	// 3: Sync

	 /* Do this in RkrTimeRangeAnalysis
	 * 0, 1, 2, 3 (00, 01, 10, 11)
	 * Sync
	 * PreAmble
	 * Manchester
	 *
	 * More than 4: use shortest...long
	 * Then on index: S, IntermediateSync, Gap/Trailer, Pause whatever...
	 * newkaku 1,2,2  sync
	 * oldkaku 2,2,1 no sync
	 * x10 1,2,2  sync
	 * oregonv2 2,2,3 no sync
	*/
#endif
	// now check Sync: start out of range from normal timings
	// todo
	// check preamble: repeated start P/S signals, maybe Sync after that of just ManchesterEncoding...
	// todo
	return rv;
}

void RkrTimeRangeReplaceMedian(uint Min, uint Max, uint Median, byte ixPs) {
//	int x = 5 + Ook.iTime;
	uint xEnd = Ook.iTimeEnd;
	if (ixPs > 1) {
		return;
	}
	// 0=countl, 1=startpulse, 2=space after startpulse, 3=1st data pulse
	for (uint x = 1 + Ook.iTime; x <= xEnd-1; x+=2) {
		uint value = PulseSpaceMicros(x + ixPs);
		if (value >= Min && value <= Max) {
				PulseSpaceMicrosSet(x + ixPs, Median);
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
	for (byte ixPs = 0; ixPs < 2; ixPs++) {
		int i=1;
		int iEnd=PsCount(iTimeRange);
		for (i = 1; i < iEnd; i+=2) {
				uint Min = PulseSpaceMicros(iTimeRange + i);
				uint Max = PulseSpaceMicros(iTimeRange + i + 1);
				uint Median = (Min + Max) / 2;
#ifdef 	RKR_MEDIANROUNDING	// Try Median rounding
				Median = RkrRoundTime(Median);
#endif
				RkrTimeRangeReplaceMedian(Min, Max, Median, ixPs);
		}
#ifdef 	RKR_ROUGHSYNC
		if (PulseSpaceMicros(Ook.iTime + 1 + ixPs) * 2 <= PulseSpaceMicros(Ook.iTime + 3 + ixPs) * 3) { // 1.5 sync
			PulseSpaceMicrosSet(Ook.iTime + 1 + ixPs, PulseSpaceMicros(Ook.iTime + 3 + ixPs));
		}
#endif
#ifdef 	RKR_MEDIANROUNDING	// Rounding sync/preamble as well
		PulseSpaceMicrosSet(Ook.iTime + 1 + ixPs, RkrRoundTime(PulseSpaceMicros(Ook.iTime + 1 + ixPs)));
		PulseSpaceMicrosSet(Ook.iTime + 3 + ixPs, RkrRoundTime(PulseSpaceMicros(Ook.iTime + 3 + ixPs)));
#endif
		iTimeRange += PsCount(iTimeRange) + 1;
	}
}

// called from RawSignal_2_32bit, that is called by PrintPulseSpaceTiming...
//int RkrTimeRange(uint iTime, uint iTimeRange, byte ixPs) {
int RkrTimeRange(uint MinTime, uint MaxTime, byte ixPs, boolean fPrint) {
	uint i=1;
	uint iEnd = 2;
	uint iTimeRange = (ixPs == 0) ? RAW_BUFFER_TIMERANGE_START : Ook.iTimeRange[ixPs - 1] + PulseSpaceMicros(Ook.iTimeRange[ixPs - 1]) + 1;
	Ook.iTimeRange[ixPs] = iTimeRange;
	PulseSpaceMicrosSet(iTimeRange, iEnd);
	PulseSpaceMicrosSet(iTimeRange+1, MinTime);
	PulseSpaceMicrosSet(iTimeRange+2, MaxTime);

	if (PsCount(Ook.iTime) < 10) {
		return 0;
	}
	if (Ook.iTime + PsCount(Ook.iTime) >= iTimeRange) {
		if (fPrint) {
			PrintNum(Ook.iTime,0, 3);
			PrintNum(Ook.iTime + PsCount(Ook.iTime),',', 3);
			PrintNum(iTimeRange,',', 3);
			PrintLnStartRaw(F("RkrTimeRange Overflow"));
		}
		return 0;
	}

	while ((i < iEnd) && (iEnd < (RAW_BUFFER_TIMERANGE_SIZE - 4))) { // was -2 seems to overflow -3 or -4?
		uint Min=PulseSpaceMicros(iTimeRange + i + 1);
		uint Max=PulseSpaceMicros(iTimeRange + i + 0); //RKR

		if (i > 8) { /// was 16, limit 8
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
				uint value = ((ixPs == 0) ? PulseSpaceMicros(x) // pulse
									   : ((ixPs == 1) ? PulseSpaceMicros(x + 1) //space
													:  (PulseSpaceMicros(x) + PulseSpaceMicros(x + 1)))); // pulse + space
				if (value < Min && value >= Median) {
					Min=value; // Zoek naar de kortste pulstijd.
				}
				if (value > Max  && value <= Median) {
					Max=value; // Zoek naar de langste pulstijd.
				}
			}
			// at least one new value found: extend with 2
			if (/* (Min != Max) && */ !((Min==PulseSpaceMicros(iTimeRange + i + 1)) && (Max==PulseSpaceMicros(iTimeRange + i + 0)))) {
				//return iEnd;
				// extend array with 2
				iEnd += 2;
				PulseSpaceMicrosSet(iTimeRange, iEnd);
				for (int j = iEnd; j > i+2; j--) {
					PulseSpaceMicrosSet(iTimeRange + j, PulseSpaceMicros(iTimeRange + j - 2));
				}
				PulseSpaceMicrosSet(iTimeRange + i + 1, Max);
				PulseSpaceMicrosSet(iTimeRange + i + 2, Min);
			}
			else {
				i++;
			}
		}
		else {
			i++;
		}
	}
	if (fPrint) {
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
			Serial.print(PulseSpaceMicros(iTimeRange+j),DEC);
		}
		PrintChar(']');
	}
	return iEnd;
}


int RkrTimeRangeStats(byte ixPs, uint StartTime, uint MinTime, uint MaxTime, uint count, ulong hash, bool fPrint) {
		if (fPrint) {
			PrintTermRaw();

			PrintStartRaw((ixPs == ixPulseSpace) ? F("RawP+S") : ((ixPs== ixPulse)? F("RawP  "):F("RawS  ")));
			PrintNum(count, ' ', 3);
			PrintNum(MinTime, ',', 4);
			PrintNum(MaxTime, ',', 4);
			PrintNum(MaxTime-MinTime, ',', 4);
			PrintNum(StartTime, ',', 4); // start space/preamble
			PrintComma();
			PrintValue(hash);
		}
		RkrTimeRange(MinTime, MaxTime, ixPs, fPrint); // // Pulse + Space
}

int RkrCheckRepeatedPackage(uint iTime, boolean fIsRf) {
	int rc;
	uint xEnd;
	ulong totalMicros;
	byte iPrintPulseAndSpace = 3; // normal 3, now 0 to show only results
	byte iPrintPulseAndSpaceRounded = 3;
	Ook.iTime = iTime;
	Ook.iTimeEnd = PulseSpaceMicros(iTime) + iTime;
	xEnd = Ook.iTimeEnd;
	int iTimeRange = RAW_BUFFER_TIMERANGE_START;
	ulong hash;
	PulseSpaceMicrosSet(iTimeRange,  0);
	Ook.iTimeRange[ixPulse] = iTimeRange;
	Ook.iTimeRange[ixSpace] = iTimeRange;
	Ook.iTimeRange[ixPulseSpace] = iTimeRange;
	//total time
	totalMicros = 0;
	for(uint x=1+Ook.iTime;x<=xEnd;x++) {
		totalMicros += PulseSpaceMicros(x);
	}
	hash = RawSignal_2_32bit(Ook.iTime, false, false); // original nodo due hash
	if (Ook.iTime <= 0) { // first signal/no repetition yet
		Ook.nRepeats = 0;
		rc = 0;
	}
	else {
		// skip incomplete last signal
		if (hash != Ook.lastHash && abs(xEnd-Ook.iTime - Ook.lastLength) > 24) {
			rc = -1;
		}
		if (hash == Ook.lastHash && abs(xEnd-Ook.iTime - Ook.lastLength) < 24) { // sync missing, hash is pretty good
			Ook.nRepeats++;
			rc = Ook.nRepeats;
		}
		else {
			rc = -2;
		}
	}
	// store for next comparison also on failure (first package may be incomplete...
	Ook.lastHash = hash;
	Ook.lastLength = xEnd-Ook.iTime;
	Ook.lastTotalTime = totalMicros;
	return rc;
}

int RkrCheckSignalOrNoise(uint iTime, boolean fIsRf) {
	uint minTime = (fIsRf) ? 50: 50;
	uint xEnd = PulseSpaceMicros(iTime) + iTime;
	uint xFirstSpike;
	int spikeCount = 0;
	for(uint x=1+iTime;x<=xEnd;x++) {
		if (PulseSpaceMicros(x) < minTime) {
			if (spikeCount == 0) {
				xFirstSpike = x;
			}
			spikeCount++;
		}
	}
	if (spikeCount == 1) {
		if ((xFirstSpike > (iTime + 16)) && (xFirstSpike > (xEnd - 16))) {
			spikeCount = 0;
		}
	}
	return (spikeCount > 0) ? 0 : 1;
}

/*
 * Transfer point From Nodo_Due code into OokTimeRangeCode
 *
 *	PulseSpaceMicros(Ook.iTime] contains start of Nodo measured timings
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
	Ook.iTimeEnd = PulseSpaceMicros(iTime) + iTime;
	xEnd = Ook.iTimeEnd;
	int iTimeRange = RAW_BUFFER_TIMERANGE_START;
	ulong hash;
	PulseSpaceMicrosSet(iTimeRange,  0);
	Ook.iTimeRange[ixPulse] = iTimeRange;
	Ook.iTimeRange[ixSpace] = iTimeRange;
	Ook.iTimeRange[ixPulseSpace] = iTimeRange;
	//total time -4 test 2015
	if ((Ook.iTime > (RAW_BUFFER_SIZE+2)) || (Ook.iTimeEnd > (RAW_BUFFER_SIZE+2))) {
		PrintNum(Ook.iTime,0, 3);
		PrintNum(Ook.iTimeEnd,',', 3);
		PrintLnStartRaw(F("PrintPulseSpaceTiming Overflow"));
		return;
	}
	i = 0;
	for(int x=1+Ook.iTime;x<=xEnd;x++) {
		i += PulseSpaceMicros(x);
	}
#if 1
	Ook.lastTotalTime = i;
#endif
	if (i <= 0) {
		return;
	}
	hash = AnalyzeRawSignal(Ook.iTime);
	PrintStartRaw(fIsRf ? F("Rf") : F("Ir"));
	if (Ook.nRepeats > 0) {
			Serial.print(F("* "));
			// count
			PrintNum(PsCount(Ook.iTime),' ', 3);
			// repeats
			PrintNum(Ook.nRepeats+1,',', 2);
			// inter message time
			PrintNum(psStartSignalMillis - psStartSignalMillisLast,',', 5);
			psStartSignalMillisLast = psStartSignalMillis;
	}
	else {
		if (Ook.iTime <= 0) { // first signal/no repetition yet
			//PrintEventCode(AnalyzeRawSignal(0));
			//PrintTerm();
			//PrintTermRaw();
			Serial.print(F("? "));
			// count
			PrintNum(PsCount(Ook.iTime),',', 4);
			// inter message time
			PrintNum(psStartSignalMillis - psStartSignalMillisLast,0, 5);
			psStartSignalMillisLast = psStartSignalMillis;
		}
		else {
	#if 1
			// skip incomplete last signal
			if (hash != Ook.lastHash && abs(xEnd-Ook.iTime - Ook.lastLength) > 4) {
				return;
			}
	#endif
			if (hash == Ook.lastHash && xEnd-Ook.iTime == Ook.lastLength) {
				Serial.print(F("+ ")); // last = current, so likely good match!
			}
			else {
				Serial.print(F("- "));
			}
			// count
			PrintNum(PsCount(Ook.iTime),',', 4);
			// intra message
			PrintNum(PulseSpaceMicros(xEnd+1),0, 5);
		}
	}
#if 1
	Ook.lastHash = hash;
	Ook.lastLength = xEnd-Ook.iTime;
#endif
//	PrintComma();
	//total time
	i = 0;
	for(int x=1+Ook.iTime;x<=xEnd;x++) {
		i += PulseSpaceMicros(x);
	}
	PrintNum(i,',', 5);
//	PrintComma();

#if 0
	// count
	PrintNum(PsCount(Ook.iTime),',', 2);
//	PrintComma();

	// net count min spikes
	i = 0;
	for(int x=1+Ook.iTime;x<=xEnd;x++) {
		if (PulseSpaceMicros(x) < 100) {
			i++;
		}
	}
	PrintNum(PsCount(Ook.iTime)-i,',', 0);
//	PrintComma();

	PrintNum(i,',', 0);
	PrintComma();
#endif

	PrintEventCode(hash);
	// todo print min/max and minButOne/maxButOne
	RawSignal_2_32bit(Ook.iTime, true, true);
	PrintTermRaw();
	for (i=0; i < 2; i++) {
		//  PrintText(Text_07,false);
		if ((iPrintPulseAndSpace != 0) && (settings.Mode == omNodoDueRkr)) {
			for(int x=1+Ook.iTime;x<=xEnd;x++) {
				if ((x - (1+Ook.iTime))%16==0) {
						if ((x - (1+Ook.iTime))> 0) {
							PrintTerm();
						}
						PrintNum(x - (1+Ook.iTime), 0, 4);
						PrintChar(':');
						if (iPrintPulseAndSpace & 1) { // mark, space
							PrintNum(PulseSpaceMicros(x), 0, 4);
						}
				}
				else {
					if (iPrintPulseAndSpace & 1) { // mark, space
						PrintNum(PulseSpaceMicros(x), ',', 4);
					}
				}
				if (iPrintPulseAndSpace & 2) { // mark + space
					if ((x - (1+Ook.iTime))%2==1) {
						Serial.print(F(" ["));
						PrintNum(PulseSpaceMicros(x) + PulseSpaceMicros(x-1), 0, 4);
						PrintChar(']');
					}
				}

			}
			PrintTerm();
		}
		if (i == 0) {
#if 0
			if (PsCount(iTimeRange) != 0) {
				PrintStartRaw(F("Rounded "));
				iPrintPulseAndSpaceRounded = RkrTimeRangeAnalysis();
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
#else	// disable rounded display
			i = 2;
			iPrintPulseAndSpace = 0;
#endif
		}
	}
	if (iPrintPulseAndSpace == 2) {
		if ((iPrintPulseAndSpace == 2)  && (settings.Mode == omNodoDueRkr)) {
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
					PrintNum(PulseSpaceMicros(x) + PulseSpaceMicros(x-1), 0, 4);
					PrintChar(']');
				}
			}
		}
		PrintTermRaw();
	}
	/*
	 * P+S, pulse/Space timings equal length, manchester coding. Sync, SyncAfterPreamble P;S, P/S, P+S, Gap...
	 * Do this in RkrTimeRangeAnalysis
	 * 0, 1, 2, 3 (00, 01, 10, 11)
	 * Sync
	 * PreAmble
	 * Manchester
	 *
	 * More than 4: use shortest...long
	 * Then on index: S, IntermediateSync, Gap/Trailer, Pause whatever...
	 * newkaku 1,2,2  sync
	 * oldkaku 2,2,1 no sync
	 * x10 1,2,2  sync
	 * oregonv2 2,2,3 no sync
	 */
	// Preamble experiment 2,2,3
	if ((OokNrTimeRanges(ixPulseSpace)==3) && ((OokNrTimeRanges(ixPulse)==2) && (OokNrTimeRanges(ixSpace)==2))) { // preamble/manchester like
#if 0
		RkrPreAmbleSyncManchesterAnalysis(true, false, false);
		if (PulseSpaceMicros(iTime) <= 180) {
			RkrPreAmbleSyncManchesterAnalysis(true, true, false);
		}
		else {// oregon scientific v2 protocol?
			RkrPreAmbleSyncManchesterAnalysis(true, false, true);
		}
#else
		if (RkrPreAmbleSyncManchesterAnalysis(false, false, true) >= 4) {
			// oregon scientific v2 protocol?
			RkrPreAmbleSyncManchesterAnalysis(true, false, true);
		}
		else if (RkrPreAmbleSyncManchesterAnalysis(false, true, false) >= 4) {
			RkrPreAmbleSyncManchesterAnalysis(true, true, false);
		}
		else {
			RkrPreAmbleSyncManchesterAnalysis(true, false, false);
		}
#endif
	}
	else { // Sync or just long/short experiment
		// 1,2,2 or 2,1,2 signals: double min/max pairs...
		boolean fSync = (OokNrTimeRanges(ixPulseSpace)==2) && ((OokNrTimeRanges(ixPulse)==1) || (OokNrTimeRanges(ixSpace)==1)); // x10 like
#if 0
		if (fSync) {
			RkrPulseDistanceAnalysis(false, false); // seems ok for 44 X10...
		}
#endif
		RkrPulseDistanceAnalysis(false, fSync); // seems ok for 44 X10...
		RkrPulseDistanceAnalysis(true, fSync); // seems ok for 44 X10...
	}
}


void PrintPulseSpaceTimingAvrLirc(uint iTime) {
	uint x, xEnd;
	uint i;

	Ook.iTime = iTime;
	Ook.iTimeEnd = PsCount(iTime) + iTime;
	xEnd = Ook.iTimeEnd;
	int iTimeRange = RAW_BUFFER_TIMERANGE_START;
	PulseSpaceMicrosSet(iTimeRange, 0);
	Ook.iTimeRange[ixPulse] = iTimeRange;
	Ook.iTimeRange[ixSpace] = iTimeRange;
	Ook.iTimeRange[ixPulseSpace] = iTimeRange;
	i = 0;
	for(int x=1+Ook.iTime;x<=xEnd;x++) {
		i += PulseSpaceMicros(x);
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
	Ook.iTimeEnd = PulseSpaceMicros(iTime) + iTime;
	xEnd = Ook.iTimeEnd;
	int iTimeRange = RAW_BUFFER_TIMERANGE_START;
	PulseSpaceMicrosSet(iTimeRange, 0);
	Ook.iTimeRange[ixPulse] = iTimeRange;
	Ook.iTimeRange[ixSpace] = iTimeRange;
	Ook.iTimeRange[ixPulseSpace] = iTimeRange;
	i = 0;
	for(int x=1+Ook.iTime;x<=xEnd;x++) {
		i += PulseSpaceMicros(x);
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
	Ook.iTimeEnd = PulseSpaceMicros(iTime) + iTime;
	xEnd = Ook.iTimeEnd;
	int iTimeRange = RAW_BUFFER_TIMERANGE_START;
	PulseSpaceMicrosSet(iTimeRange, 0);
	Ook.iTimeRange[ixPulse] = iTimeRange;
	Ook.iTimeRange[ixSpace] = iTimeRange;
	Ook.iTimeRange[ixPulseSpace] = iTimeRange;
	i = 0;
	for(int x=1+Ook.iTime;x<=xEnd;x++) {
		i += PulseSpaceMicros(x);
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
	if (settings.Mode == omAnalysIR) {
		PrintLnStartRaw(F("AnalysIr"));
		PrintPulseSpaceTimingAnalysIr(iTime);
	}
	if (settings.Mode == omLirc) {
		// PrintLnStartRaw(F("Avr"));
		PrintPulseSpaceTimingAvrLirc(iTime);
		PrintTerm();
	}
}

void PrintPulseSpaceTimingEnd() {
	PrintLnStartRaw(F("End"));
}


 /**********************************************************************************************\
 * Voert alle relevante acties in de eventlist uit die horen bij het binnengekomen event
 * Doorlopen van een volledig gevulde eventlist duurt ongeveer 15ms inclusief printen naar serial
 * maar exclusief verwerking n.a.v. een 'hit'.
 \*********************************************************************************************/
void PrintPulseSpaceTimings(ulong Hash, boolean fIsRf) {
	int psmStartRepeatedPackage = 0;
	boolean fDoneSome = false;
	digitalWrite(MonitorLedPin,HIGH);           // LED aan als er iets verwerkt wordt
	// check for repeated signals and report just second package + intra message timing
	for(int psmStart = 0; (PsCount(psmStart) != 0) && (psmStart < RAW_BUFFER_SIZE); psmStart = psmStart + PsCount(psmStart) + 2) {
		// check all packages before a conclusion
		// first package may be incomplete
		if ((RkrCheckRepeatedPackage(psmStart, fIsRf) > 0) && (psmStartRepeatedPackage == 0)) {
			psmStartRepeatedPackage = psmStart;
		}
	}
	if ((psmStartRepeatedPackage != 0) && (Ook.nRepeats > 0)) {
		PrintPulseSpaceTiming(psmStartRepeatedPackage, fIsRf);
		PrintPulseSpaceTimingEnd();
	}
	else { // OOK/RF: do not report? Or only very long signal. IR is less noisy so do report?
		// todo check missed gaps/timeouts 48 or 64 then check?
		Ook.nRepeats = 0;
		// RAWSIGNAL_MULTI
		for(int psmStart = 0; (PsCount(psmStart) != 0) && (psmStart < RAW_BUFFER_SIZE); psmStart = psmStart + PsCount(psmStart) + 2) {
			if (RkrCheckSignalOrNoise(psmStart, fIsRf) > 0) {
				PrintPulseSpaceTiming(psmStart, fIsRf);
				fDoneSome = true;
			}
		}
		if (fDoneSome) {
			PrintPulseSpaceTimingEnd();
		}
#if 0
		else {
		for(int psmStart = 0; (PsCount(psmStart) != 0) && (psmStart < RAW_BUFFER_SIZE); psmStart = psmStart + PsCount(psmStart) + 2) {
				PrintPulseSpaceTiming(psmStart, fIsRf);
				fDoneSome = true;
		}
		if (fDoneSome) {
			PrintPulseSpaceTimingEnd();
		}
	}
#endif
	}
}
