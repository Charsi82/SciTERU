// SciTE - Scintilla based Text Editor
/** @file UnicodeConstants.h
 ** Define constants used for Unicode.
 **/
// Copyright 1998-2025 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef UNICODECONSTANTS_H
#define UNICODECONSTANTS_H

constexpr unsigned int first2Byte = 0x80;
constexpr unsigned int first3Byte = 0x800;
constexpr unsigned int first4Byte = 0x10000;

constexpr unsigned int maskSurrogate = 0x3FF;
constexpr unsigned int shiftSurrogate = 10;

constexpr unsigned int trailByteFlag = 0b1000'0000;
constexpr unsigned int trailByteMask = 0b0011'1111;

constexpr unsigned int leadByte2 = 0b1100'0000;
constexpr unsigned int leadBits2 = 0b0001'1111;

constexpr unsigned int leadByte3 = 0b1110'0000;
constexpr unsigned int leadBits3 = 0b0000'1111;

constexpr unsigned int leadByte4 = 0b1111'0000;
constexpr unsigned int leadBits4 = 0b0000'0111;

constexpr unsigned int shiftUTF8 = 6;
constexpr unsigned int shiftByte2 = shiftUTF8;
constexpr unsigned int shiftByte3 = shiftUTF8 * 2;
constexpr unsigned int shiftByte4 = shiftUTF8 * 3;

constexpr size_t LengthFromLeadByte(unsigned char c) noexcept {
	size_t lenChar = 1;
	if (c >= leadByte4) {
		lenChar = 4;
	} else if (c >= leadByte3) {
		lenChar = 3;
	} else if (c >= leadByte2) {
		lenChar = 2;
	}
	// Values from 0x80 to 0xBF are invalid as lead bytes but treated as length 1 here
	return lenChar;
}

constexpr unsigned char TrailByteValue(unsigned char c) noexcept {
	return c & trailByteMask;
}

// Helper for UTF8FromUTF32 processes 6 bits of input and isolates bit twiddling and cast.
constexpr char SixBits(unsigned int uch, unsigned int shift, unsigned int mark = trailByteFlag) noexcept {
	return static_cast<char>(mark | ((uch >> (shift * shiftUTF8)) & trailByteMask));
}

#endif
