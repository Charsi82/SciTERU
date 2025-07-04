// Scintilla source code edit control
/** @file LexBatch.cxx
 ** Lexer for batch files.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <cstdarg>

#include <string>
#include <string_view>
#include <initializer_list>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "InList.h"
#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

using namespace Lexilla;

namespace {

constexpr bool Is0To9(char ch) noexcept {
	return (ch >= '0') && (ch <= '9');
}

bool IsAlphabetic(int ch) noexcept {
	return IsASCII(ch) && isalpha(ch);
}

inline bool AtEOL(Accessor &styler, Sci_PositionU i) {
	return (styler[i] == '\n') ||
	       ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}

// Tests for BATCH Operators
constexpr bool IsBOperator(char ch) noexcept {
	return (ch == '=') || (ch == '+') || (ch == '>') || (ch == '<') ||
		(ch == '|') || (ch == '?') || (ch == '*')||
		(ch == '&') || (ch == '(') || (ch == ')');
}

// Tests for BATCH Separators
constexpr bool IsBSeparator(char ch) noexcept {
	return (ch == '\\') || (ch == '.') || (ch == ';') ||
		(ch == '\"') || (ch == '\'') || (ch == '/');
}

// Tests for escape character
constexpr bool IsEscaped(const char* wordStr, Sci_PositionU pos) noexcept {
	bool isQoted=false;
	while (pos>0){
		pos--;
		if (wordStr[pos]=='^')
			isQoted=!isQoted;
		else
			break;
	}
	return isQoted;
}

constexpr bool IsQuotedBy(std::string_view svBuffer, char quote) noexcept {
	bool CurrentStatus = false;
	size_t pQuote = svBuffer.find(quote);
	while (pQuote != std::string_view::npos) {
		if (!IsEscaped(svBuffer.data(), pQuote)) {
			CurrentStatus = !CurrentStatus;
		}
		pQuote = svBuffer.find(quote, pQuote + 1);
	}
	return CurrentStatus;
}

// Tests for quote character
constexpr bool textQuoted(const char *lineBuffer, Sci_PositionU endPos) noexcept {
	const std::string_view svBuffer(lineBuffer, endPos);
	return IsQuotedBy(svBuffer, '\"') || IsQuotedBy(svBuffer, '\'');
}

#ifdef RB_BALI
//!-start-[BatchLexerImprovement]
// Tests for Environment Variable simbol
inline bool IsEnvironmentVar(char ch)
{
	return isalpha(ch) || Is0To9(ch) || (ch == '_');
}

// Tests for BATCH Variable simbol
inline bool IsBatchVar(char ch)
{
	return isalpha(ch) || Is0To9(ch);
}

// Find length of BATCH Variable with modifier (%~...) or return 0
Sci_PositionU GetBatchVarLen(char* wordBuffer, Sci_PositionU wbl)
{
	if (wbl > 2 && wordBuffer[0] == '%' && wordBuffer[1] == '~') {
		wordBuffer += 2;
		if (wbl > 5 && wordBuffer[0] == '$' && isalpha(wordBuffer[1])) {
			unsigned int l = 2;
			while (IsEnvironmentVar(wordBuffer[l])) l++;
			if (wordBuffer[l] == ':' && IsBatchVar(wordBuffer[l + 1]))
				return l + 4;
		}
		else
			if (wbl > 7 && 0 == CompareNCaseInsensitive(wordBuffer, "dp$", 3) &&
				isalpha(wordBuffer[3])) {
				unsigned int l = 4;
				while (IsEnvironmentVar(wordBuffer[l])) l++;
				if (wordBuffer[l] == ':' && IsBatchVar(wordBuffer[l + 1]))
					return l + 4;
			}
			else
				if (wbl > 6 && 0 == CompareNCaseInsensitive(wordBuffer, "ftza", 4) &&
					IsBatchVar(wordBuffer[4])) {
					return 7;
				}
				else
					if (wbl > 4 &&
						(0 == CompareNCaseInsensitive(wordBuffer, "dp", 2) ||
							0 == CompareNCaseInsensitive(wordBuffer, "nx", 2) ||
							0 == CompareNCaseInsensitive(wordBuffer, "fs", 2)) &&
						IsBatchVar(wordBuffer[2])) {
						return 5;
					}
					else
						if (wbl > 3 &&
							(wordBuffer[0] == 'f' || wordBuffer[0] == 'F' ||
							wordBuffer[0] == 'd' || wordBuffer[0] == 'D' ||
							wordBuffer[0] == 'p' || wordBuffer[0] == 'P' ||
							wordBuffer[0] == 'n' || wordBuffer[0] == 'N' ||
							wordBuffer[0] == 'x' || wordBuffer[0] == 'X' ||
							wordBuffer[0] == 's' || wordBuffer[0] == 'S' ||
							wordBuffer[0] == 'a' || wordBuffer[0] == 'A' ||
							wordBuffer[0] == 't' || wordBuffer[0] == 'T' ||
							wordBuffer[0] == 'z' || wordBuffer[0] == 'Z')
							&& IsBatchVar(wordBuffer[1])) {
							return 4;
						}
						else
							if (IsBatchVar(wordBuffer[0])) {
								return 3;
							}
	}
	return 0;
}
//!-end-[BatchLexerImprovement]
#endif // RB_BALI

void ColouriseBatchDoc(
    Sci_PositionU startPos,
    Sci_Position length,
    int /*initStyle*/,
    WordList *keywordlists[],
    Accessor &styler) {
	// Always backtracks to the start of a line that is not a continuation
	// of the previous line
	if (startPos > 0) {
		Sci_Position ln = styler.GetLine(startPos); // Current line number
		while (startPos > 0) {
			ln--;
			if ((styler.SafeGetCharAt(startPos-3) == '^' && styler.SafeGetCharAt(startPos-2) == '\r' && styler.SafeGetCharAt(startPos-1) == '\n')
			|| styler.SafeGetCharAt(startPos-2) == '^') {	// handle '^' line continuation
				// When the line continuation is found,
				// set the Start Position to the Start of the previous line
				length+=startPos-styler.LineStart(ln);
				startPos=styler.LineStart(ln);
			}
			else
				break;
		}
	}

	char lineBuffer[1024] {};

	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	Sci_PositionU linePos = 0;
	Sci_PositionU startLine = startPos;
	bool continueProcessing = true;	// Used to toggle Regular Keyword Checking
	bool isNotAssigned=false; // Used to flag Assignment in Set operation

	for (Sci_PositionU i = startPos; i < startPos + length; i++) {
		lineBuffer[linePos++] = styler[i];
		if (AtEOL(styler, i) || (linePos >= sizeof(lineBuffer) - 1) || (i==startPos + length-1)) {
			// End of line (or of line buffer) (or End of Last Line) met, colourise it
			lineBuffer[linePos] = '\0';
			const Sci_PositionU lengthLine=linePos;
			const Sci_PositionU endPos=i;
			const WordList &keywords = *keywordlists[0];      // Internal Commands
			const WordList &keywords2 = *keywordlists[1];     // External Commands (optional)
#ifdef RB_BALI
			bool isDelayedExpansion = styler.GetPropertyInt("lexer.batch.enabledelayedexpansion") != 0; //!-add-[BatchLexerImprovement]
			bool inString = false; // Used for processing while "" //!-add-[BatchLexerImprovement]
#endif // RB_BALI

			// CHOICE, ECHO, GOTO, PROMPT and SET have Default Text that may contain Regular Keywords
			//   Toggling Regular Keyword Checking off improves readability
			// Other Regular Keywords and External Commands / Programs might also benefit from toggling
			//   Need a more robust algorithm to properly toggle Regular Keyword Checking
			bool stopLineProcessing=false;  // Used to stop line processing if Comment or Drive Change found

			Sci_PositionU offset = 0;	// Line Buffer Offset
			// Skip initial spaces
			while ((offset < lengthLine) && (isspacechar(lineBuffer[offset]))) {
				offset++;
			}
			// Colorize Default Text
			styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
			// Set External Command / Program Location
			Sci_PositionU cmdLoc = offset;

			// Check for Fake Label (Comment) or Real Label - return if found
			if (lineBuffer[offset] == ':') {
				if (lineBuffer[offset + 1] == ':') {
					// Colorize Fake Label (Comment) - :: is similar to REM, see http://content.techweb.com/winmag/columns/explorer/2000/21.htm
					styler.ColourTo(endPos, SCE_BAT_COMMENT);
				} else {
					// Colorize Real Label
					// :[\t ]*[^\t &+:<>|]+
					const char *startLabelName = lineBuffer + offset + 1;
					const size_t whitespaceLength = strspn(startLabelName, "\t ");
					// Set of label-terminating characters determined experimentally
					const char *endLabel = strpbrk(startLabelName + whitespaceLength, "\t &+:<>|");
					if (endLabel) {
						styler.ColourTo(startLine + offset + endLabel - startLabelName, SCE_BAT_LABEL);
						styler.ColourTo(endPos, SCE_BAT_AFTER_LABEL);	// New style
					} else {
						styler.ColourTo(endPos, SCE_BAT_LABEL);
					}
				}
				stopLineProcessing=true;
#ifdef RB_BALI
			//!-start-[BatchLexerImprovement]
			// Check for Comment - return if found
			} else if (CompareNCaseInsensitive(lineBuffer + offset, "rem", 3) == 0 &&
				isspacechar(lineBuffer[offset + 3])) {
				styler.ColourTo(endPos, SCE_BAT_COMMENT);
				stopLineProcessing = true;  //return;
			//!-end-[BatchLexerImprovement]
#endif // RB_BALI

			// Check for Drive Change (Drive Change is internal command) - return if found
			} else if ((IsAlphabetic(lineBuffer[offset])) &&
				(lineBuffer[offset + 1] == ':') &&
				((isspacechar(lineBuffer[offset + 2])) ||
				(((lineBuffer[offset + 2] == '\\')) &&
				(isspacechar(lineBuffer[offset + 3]))))) {
				// Colorize Regular Keyword
				styler.ColourTo(endPos, SCE_BAT_WORD);
				stopLineProcessing=true;
			}

			// Check for Hide Command (@ECHO OFF/ON)
			if (lineBuffer[offset] == '@') {
				styler.ColourTo(startLine + offset, SCE_BAT_HIDE);
				offset++;
			}
			// Skip next spaces
			while ((offset < lengthLine) && (isspacechar(lineBuffer[offset]))) {
				offset++;
			}

			// Read remainder of line word-at-a-time or remainder-of-word-at-a-time
			while (offset < lengthLine  && !stopLineProcessing) {
				if (offset > startLine) {
					// Colorize Default Text
					styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
				}
				char wordBuffer[81]{};		// Word Buffer - large to catch long paths
				// Copy word from Line Buffer into Word Buffer and convert to lower case
				Sci_PositionU wbl = 0;		// Word Buffer Length
				for (; offset < lengthLine && wbl < 80 &&
						!isspacechar(lineBuffer[offset]); wbl++, offset++) {
					wordBuffer[wbl] = MakeLowerCase(lineBuffer[offset]);
				}
				wordBuffer[wbl] = '\0';
				const std::string_view wordView(wordBuffer);
				Sci_PositionU wbo = 0;		// Word Buffer Offset - also Special Keyword Buffer Length

				// Check for Comment - return if found
#ifndef RB_BALI
				if (continueProcessing) {
					if ((wordView == "rem") || (wordBuffer[0] == ':' && wordBuffer[1] == ':')) {
						if ((offset == wbl) || !textQuoted(lineBuffer, offset - wbl)) {
							styler.ColourTo(startLine + offset - wbl - 1, SCE_BAT_DEFAULT);
							styler.ColourTo(endPos, SCE_BAT_COMMENT);
							break;
						}
					}
				}
#endif
				// Check for Separator
				if (IsBSeparator(wordBuffer[0])) {
					// Check for External Command / Program
					if ((cmdLoc == offset - wbl) &&
						((wordBuffer[0] == ':') ||
						(wordBuffer[0] == '\\') ||
						(wordBuffer[0] == '.'))) {
						// Reset Offset to re-process remainder of word
						offset -= (wbl - 1);
						// Colorize External Command / Program
						if (!keywords2) {
							styler.ColourTo(startLine + offset - 1, SCE_BAT_COMMAND);
						} else if (keywords2.InList(wordBuffer)) {
							styler.ColourTo(startLine + offset - 1, SCE_BAT_COMMAND);
						} else {
							styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
						}
						// Reset External Command / Program Location
						cmdLoc = offset;
					} else {
						// Reset Offset to re-process remainder of word
						offset -= (wbl - 1);
						// Colorize Default Text
						styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
#ifdef RB_BALI
						//!-start-[BatchLexerImprovement]
						if (wordBuffer[0] == '"')
							inString = !inString;
						//!-end-[BatchLexerImprovement]
#endif // RB_BALI

					}
#ifdef RB_BALI
//!-start-[BatchLexerImprovement]
				// Check for Labels in text (... :label)
					}
				else if (wordBuffer[0] == ':' && isspacechar(lineBuffer[offset - wbl - 1])) {
					// Colorize Default Text
					styler.ColourTo(startLine + offset - 1 - wbl, SCE_BAT_DEFAULT);
					// Colorize Label
					styler.ColourTo(startLine + offset - 1, SCE_BAT_CLABEL);
					// No need to Reset Offset
				// Check for SetLocal Variable (!x...!)
				}
				else if (isDelayedExpansion && wordBuffer[0] == '!') {
					// Colorize Default Text
					styler.ColourTo(startLine + offset - 1 - wbl, SCE_BAT_DEFAULT);
					wbo++;
					// Search to end of word for second !
					while ((wbo < wbl) &&
						(wordBuffer[wbo] != '!') &&
						(!IsBOperator(wordBuffer[wbo])) &&
						(!IsBSeparator(wordBuffer[wbo]))) {
						wbo++;
					}
					if (wordBuffer[wbo] == '!') {
						wbo++;
						// Colorize Environment Variable
						styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_EXPANSION);
					}
					else {
						wbo = 1;
						// Colorize Simbol
						styler.ColourTo(startLine + offset - 1 - (wbl - 1), SCE_BAT_DEFAULT);
					}
					// Check for External Command / Program
					if (cmdLoc == offset - wbl) {
						cmdLoc = offset - (wbl - wbo);
					}
					// Reset Offset to re-process remainder of word
					offset -= (wbl - wbo);
//!-end-[BatchLexerImprovement]
#endif // RB_BALI
				// Check for Regular Keyword in list
				} else if ((keywords.InList(wordBuffer)) &&
#ifdef RB_BALI
					(!inString) && //!-add-[BatchLexerImprovement]
#endif // RB_BALI
					(continueProcessing)) {
					// ECHO, GOTO, PROMPT and SET require no further Regular Keyword Checking
					if (InList(wordView, {"echo", "goto", "prompt"})) {
						continueProcessing = false;
					}
					// SET requires additional processing for the assignment operator
					if (wordView == "set") {
						continueProcessing = false;
						isNotAssigned=true;
					}
					// Identify External Command / Program Location for ERRORLEVEL, and EXIST
					if (InList(wordView, {"errorlevel", "exist"})) {
						// Reset External Command / Program Location
						cmdLoc = offset;
						// Skip next spaces
						while ((cmdLoc < lengthLine) &&
							(isspacechar(lineBuffer[cmdLoc]))) {
							cmdLoc++;
						}
						// Skip comparison
						while ((cmdLoc < lengthLine) &&
							(!isspacechar(lineBuffer[cmdLoc]))) {
							cmdLoc++;
						}
						// Skip next spaces
						while ((cmdLoc < lengthLine) &&
							(isspacechar(lineBuffer[cmdLoc]))) {
							cmdLoc++;
						}
					// Identify External Command / Program Location for CALL, DO, LOADHIGH and LH
					} else if (InList(wordView, {"call", "do", "loadhigh", "lh"})) {
						// Reset External Command / Program Location
						cmdLoc = offset;
						// Skip next spaces
						while ((cmdLoc < lengthLine) &&
							(isspacechar(lineBuffer[cmdLoc]))) {
							cmdLoc++;
						}
						// Check if call is followed by a label
						if ((lineBuffer[cmdLoc] == ':') &&
							(wordView == "call")) {
							continueProcessing = false;
						}
					}
					// Colorize Regular keyword
					styler.ColourTo(startLine + offset - 1, SCE_BAT_WORD);
					// No need to Reset Offset
				// Check for Special Keyword in list, External Command / Program, or Default Text
				} else if ((wordBuffer[0] != '%') &&
						   (wordBuffer[0] != '!') &&
#ifdef RB_BALI
						   (!inString) && //!-add-[BatchLexerImprovement]
#endif // RB_BALI
					(!IsBOperator(wordBuffer[0])) &&
					(continueProcessing)) {
					// Check for Special Keyword
					//     Affected Commands are in Length range 2-6
					//     Good that ERRORLEVEL, EXIST, CALL, DO, LOADHIGH, and LH are unaffected
					bool sKeywordFound = false;		// Exit Special Keyword for-loop if found
					for (Sci_PositionU keywordLength = 2; keywordLength < wbl && keywordLength < 7 && !sKeywordFound; keywordLength++) {
						// Special Keywords are those that allow certain characters without whitespace after the command
						// Examples are: cd. cd\ md. rd. dir| dir> echo: echo. path=
						// Special Keyword Buffer used to determine if the first n characters is a Keyword
						char sKeywordBuffer[10]{};	// Special Keyword Buffer
						wbo = 0;
						// Copy Keyword Length from Word Buffer into Special Keyword Buffer
						for (; wbo < keywordLength; wbo++) {
							sKeywordBuffer[wbo] = wordBuffer[wbo];
						}
						sKeywordBuffer[wbo] = '\0';
						// Check for Special Keyword in list
						if ((keywords.InList(sKeywordBuffer)) &&
							((IsBOperator(wordBuffer[wbo])) ||
							(IsBSeparator(wordBuffer[wbo])) ||
							(wordBuffer[wbo] == ':' &&
							(InList(sKeywordBuffer, {"call", "echo", "goto"}) )))) {
							sKeywordFound = true;
							// ECHO requires no further Regular Keyword Checking
							if (std::string_view(sKeywordBuffer) == "echo") {
								continueProcessing = false;
							}
							// Colorize Special Keyword as Regular Keyword
							styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_WORD);
							// Reset Offset to re-process remainder of word
							offset -= (wbl - wbo);
						}
					}
					// Check for External Command / Program or Default Text
					if (!sKeywordFound) {
						wbo = 0;
						// Check for External Command / Program
						if (cmdLoc == offset - wbl) {
							// Read up to %, Operator or Separator
							while ((wbo < wbl) &&
								(((wordBuffer[wbo] != '%') &&
#ifdef RB_BALI
								(!isDelayedExpansion || wordBuffer[wbo] != '!') && //!-change-[BatchLexerImprovement]
#else
								(wordBuffer[wbo] != '!') &&
#endif // RB_BALI
								(!IsBOperator(wordBuffer[wbo])) &&
								(!IsBSeparator(wordBuffer[wbo]))))) {
								wbo++;
							}
							// Reset External Command / Program Location
							cmdLoc = offset - (wbl - wbo);
							// Reset Offset to re-process remainder of word
							offset -= (wbl - wbo);
							// CHOICE requires no further Regular Keyword Checking
							if (wordView == "choice") {
								continueProcessing = false;
							}
							// Check for START (and its switches) - What follows is External Command \ Program
							if (wordView == "start") {
								// Reset External Command / Program Location
								cmdLoc = offset;
								// Skip next spaces
								while ((cmdLoc < lengthLine) &&
									(isspacechar(lineBuffer[cmdLoc]))) {
									cmdLoc++;
								}
								// Reset External Command / Program Location if command switch detected
								if (lineBuffer[cmdLoc] == '/') {
									// Skip command switch
									while ((cmdLoc < lengthLine) &&
										(!isspacechar(lineBuffer[cmdLoc]))) {
										cmdLoc++;
									}
									// Skip next spaces
									while ((cmdLoc < lengthLine) &&
										(isspacechar(lineBuffer[cmdLoc]))) {
										cmdLoc++;
									}
								}
							}
							// Colorize External Command / Program
							if (!keywords2) {
								styler.ColourTo(startLine + offset - 1, SCE_BAT_COMMAND);
							} else if (keywords2.InList(wordBuffer)) {
								styler.ColourTo(startLine + offset - 1, SCE_BAT_COMMAND);
							} else {
								styler.ColourTo(startLine + offset - 1, SCE_BAT_DEFAULT);
							}
							// No need to Reset Offset
						// Check for Default Text
						} else {
							// Read up to %, Operator or Separator
							while ((wbo < wbl) &&
								(((wordBuffer[wbo] != '%') &&
#ifdef RB_BALI
								(!isDelayedExpansion || wordBuffer[wbo] != '!') && //!-change-[BatchLexerImprovement]
#else
								(wordBuffer[wbo] != '!') &&
#endif // RB_BALI
								(!IsBOperator(wordBuffer[wbo])) &&
								(!IsBSeparator(wordBuffer[wbo]))))) {
								wbo++;
							}
							// Colorize Default Text
							styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_DEFAULT);
							// Reset Offset to re-process remainder of word
							offset -= (wbl - wbo);
						}
					}
				// Check for Argument  (%n), Environment Variable (%x...%) or Local Variable (%%a)
				} else if (wordBuffer[0] == '%') {
#ifdef RB_BALI
					Sci_PositionU varlen; //!-add-[BatchLexerImprovement]
#endif // RB_BALI

					// Colorize Default Text
					styler.ColourTo(startLine + offset - 1 - wbl, SCE_BAT_DEFAULT);
					wbo++;
					// Search to end of word for second % (can be a long path)
					while ((wbo < wbl) &&
						(wordBuffer[wbo] != '%')) {
						wbo++;
					}
					// Check for Argument (%n) or (%*)
					if (((Is0To9(wordBuffer[1])) || (wordBuffer[1] == '*')) &&
						(wordBuffer[wbo] != '%')) {
						// Check for External Command / Program
						if (cmdLoc == offset - wbl) {
							cmdLoc = offset - (wbl - 2);
						}
						// Colorize Argument
						styler.ColourTo(startLine + offset - 1 - (wbl - 2), SCE_BAT_IDENTIFIER);
						// Reset Offset to re-process remainder of word
						offset -= (wbl - 2);
					// Check for Expanded Argument (%~...) / Variable (%%~...)
					// Expanded Argument: %~[<path-operators>]<single digit>
					// Expanded Variable: %%~[<path-operators>]<single identifier character>
					// Path operators are exclusively alphabetic.
					// Expanded arguments have a single digit at the end.
					// Expanded variables have a single identifier character as variable name.
					} else if (((wbl > 1) && (wordBuffer[1] == '~')) ||
						((wbl > 2) && (wordBuffer[1] == '%') && (wordBuffer[2] == '~'))) {
						// Check for External Command / Program
						if (cmdLoc == offset - wbl) {
							cmdLoc = offset - (wbl - wbo);
						}
						const bool isArgument = (wordBuffer[1] == '~');
						if (isArgument) {
							Sci_PositionU expansionStopOffset = 2;
							bool isValid = false;
							for (; expansionStopOffset < wbl; expansionStopOffset++) {
								if (Is0To9(wordBuffer[expansionStopOffset])) {
									expansionStopOffset++;
									isValid = true;
									wbo = expansionStopOffset;
									// Colorize Expanded Argument
									styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_IDENTIFIER);
									break;
								}
							}
							if (!isValid) {
								// not a valid expanded argument or variable
								styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_DEFAULT);
							}
						// Expanded Variable
						} else {
							// start after ~
							wbo = 3;
							// Search to end of word for another % (can be a long path)
							while ((wbo < wbl) &&
								(wordBuffer[wbo] != '%') &&
								(!IsBOperator(wordBuffer[wbo])) &&
								(!IsBSeparator(wordBuffer[wbo]))) {
								wbo++;
							}
							if (wbo > 3) {
								// Colorize Expanded Variable
								styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_IDENTIFIER);
							} else {
								// not a valid expanded argument or variable
								styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_DEFAULT);
							}
						}
						// Reset Offset to re-process remainder of word
						offset -= (wbl - wbo);
					// Check for Environment Variable (%x...%)
					} else if ((wordBuffer[1] != '%') &&
						(wordBuffer[wbo] == '%')) {
						wbo++;
						// Check for External Command / Program
						if (cmdLoc == offset - wbl) {
							cmdLoc = offset - (wbl - wbo);
						}
						// Colorize Environment Variable
#ifdef RB_BALI
						styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_ENVIRONMENT); //!-change-[BatchLexerImprovement]
#else
						styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_IDENTIFIER);
#endif // RB_BALI
						// Reset Offset to re-process remainder of word
						offset -= (wbl - wbo);
#ifdef RB_BALI
					//!-start-[BatchLexerImprovement]
					// Check for Variable with modifiers (%~...)
					}
					else if ((varlen = GetBatchVarLen(wordBuffer, wbl)) != 0) {
						// Check for External Command / Program
						if (cmdLoc == offset - wbl) {
							cmdLoc = offset - (wbl - varlen);
						}
						// Colorize Variable
						styler.ColourTo(startLine + offset - 1 - (wbl - varlen), SCE_BAT_IDENTIFIER);
						// Reset Offset to re-process remainder of word
						offset -= (wbl - varlen);
						// Check for Local Variable with modifiers (%%~...)
					}
					else if ((wordBuffer[1] == '%') &&
						((varlen = GetBatchVarLen(wordBuffer + 1, wbl - 1)) != 0)) {
						// Check for External Command / Program
						if (cmdLoc == offset - wbl) {
							cmdLoc = offset - (wbl - varlen - 1);
						}
						// Colorize Local Variable
						styler.ColourTo(startLine + offset - 1 - (wbl - varlen - 1), SCE_BAT_IDENTIFIER);
						// Reset Offset to re-process remainder of word
						offset -= (wbl - varlen - 1);
					//!-end-[BatchLexerImprovement]
#endif // RB_BALI

					// Check for Local Variable (%%a)
					} else if (
						(wbl > 2) &&
						(wordBuffer[1] == '%') &&
						(wordBuffer[2] != '%') &&
						(!IsBOperator(wordBuffer[2])) &&
						(!IsBSeparator(wordBuffer[2]))) {
						// Check for External Command / Program
						if (cmdLoc == offset - wbl) {
							cmdLoc = offset - (wbl - 3);
						}
						// Colorize Local Variable
						styler.ColourTo(startLine + offset - 1 - (wbl - 3), SCE_BAT_IDENTIFIER);
						// Reset Offset to re-process remainder of word
						offset -= (wbl - 3);
#ifdef RB_BALI
						//!-start-[BatchLexerImprovement]
					// Check for %%
					} else if (
							(wbl > 1) &&
							(wordBuffer[1] == '%')) {
							// Check for External Command / Program
							if (cmdLoc == offset - wbl) {
								cmdLoc = offset - (wbl - 2);
							}
							// Colorize Simbols
							styler.ColourTo(startLine + offset - 1 - (wbl - 2), SCE_BAT_DEFAULT);
							// Reset Offset to re-process remainder of word
							offset -= (wbl - 2);
					} else {
						// Check for External Command / Program
						if (cmdLoc == offset - wbl) {
							cmdLoc = offset - (wbl - 1);
						}
						// Colorize Simbol
						styler.ColourTo(startLine + offset - 1 - (wbl - 1), SCE_BAT_DEFAULT);
						// Reset Offset to re-process remainder of word
						offset -= (wbl - 1);
					//!-end-[BatchLexerImprovement]
#else
					// escaped %
					} else if (
						(wbl > 1) &&
						(wordBuffer[1] == '%')) {

						// Reset Offset to re-process remainder of word
						styler.ColourTo(startLine + offset - 1 - (wbl - 2), SCE_BAT_DEFAULT);
						offset -= (wbl - 2);
#endif // RB_BALI
					}
				// Check for Environment Variable (!x...!)
				} else if (wordBuffer[0] == '!') {
					// Colorize Default Text
					styler.ColourTo(startLine + offset - 1 - wbl, SCE_BAT_DEFAULT);
					wbo++;
					// Search to end of word for second ! (can be a long path)
					while ((wbo < wbl) &&
						(wordBuffer[wbo] != '!')) {
						wbo++;
					}
					if (wordBuffer[wbo] == '!') {
						wbo++;
						// Check for External Command / Program
						if (cmdLoc == offset - wbl) {
							cmdLoc = offset - (wbl - wbo);
						}
						// Colorize Environment Variable
						styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_IDENTIFIER);
						// Reset Offset to re-process remainder of word
						offset -= (wbl - wbo);
					}
				// Check for Operator
				} else if (IsBOperator(wordBuffer[0])) {
					// Colorize Default Text
					styler.ColourTo(startLine + offset - 1 - wbl, SCE_BAT_DEFAULT);
					// Check for Comparison Operator
					if ((wordBuffer[0] == '=') && (wordBuffer[1] == '=')) {
						// Identify External Command / Program Location for IF
						cmdLoc = offset;
						// Skip next spaces
						while ((cmdLoc < lengthLine) &&
							(isspacechar(lineBuffer[cmdLoc]))) {
							cmdLoc++;
						}
						// Colorize Comparison Operator
						if (continueProcessing)
							styler.ColourTo(startLine + offset - 1 - (wbl - 2), SCE_BAT_OPERATOR);
						else
							styler.ColourTo(startLine + offset - 1 - (wbl - 2), SCE_BAT_DEFAULT);
						// Reset Offset to re-process remainder of word
						offset -= (wbl - 2);
					// Check for Pipe Operator
					} else if ((wordBuffer[0] == '|') &&
								!(IsEscaped(lineBuffer,offset - wbl + wbo) || textQuoted(lineBuffer, offset - wbl) )) {
						// Reset External Command / Program Location
						cmdLoc = offset - wbl + 1;
						// Skip next spaces
						while ((cmdLoc < lengthLine) &&
							(isspacechar(lineBuffer[cmdLoc]))) {
							cmdLoc++;
						}
						// Colorize Pipe Operator
						styler.ColourTo(startLine + offset - 1 - (wbl - 1), SCE_BAT_OPERATOR);
						// Reset Offset to re-process remainder of word
						offset -= (wbl - 1);
						continueProcessing = true;
					// Check for Other Operator
					} else {
						// Check for Operators: >, |, &
						if (((wordBuffer[0] == '>')||
						   (wordBuffer[0] == ')')||
						   (wordBuffer[0] == '(')||
						   (wordBuffer[0] == '&' )) &&
						   !(!continueProcessing && (IsEscaped(lineBuffer,offset - wbl + wbo)
						   || textQuoted(lineBuffer, offset - wbl) ))){
							// Turn Keyword and External Command / Program checking back on
							continueProcessing = true;
							isNotAssigned=false;
						}
						// Colorize Other Operators
						// Do not Colorize Parenthesis, quoted text and escaped operators
						if (((wordBuffer[0] != ')') && (wordBuffer[0] != '(')
						&& !textQuoted(lineBuffer, offset - wbl)  && !IsEscaped(lineBuffer,offset - wbl + wbo))
						&& !((wordBuffer[0] == '=') && !isNotAssigned ))
#ifdef RB_BALI
						if (!inString || !(wordBuffer[0] == '(' || wordBuffer[0] == ')')) //!-add-[BatchLexerImprovement]
#endif // RB_BALI
							styler.ColourTo(startLine + offset - 1 - (wbl - 1), SCE_BAT_OPERATOR);
						else
							styler.ColourTo(startLine + offset - 1 - (wbl - 1), SCE_BAT_DEFAULT);
						// Reset Offset to re-process remainder of word
						offset -= (wbl - 1);

						if ((wordBuffer[0] == '=') && isNotAssigned ){
							isNotAssigned=false;
						}
					}
				// Check for Default Text
				} else {
					// Read up to %, Operator or Separator
					while ((wbo < wbl) &&
						((wordBuffer[wbo] != '%') &&
#ifdef RB_BALI
						(!isDelayedExpansion || wordBuffer[wbo] != '!') && //!-change-[BatchLexerImprovement]
#else
						(wordBuffer[wbo] != '!') &&
#endif // RB_BALI
						(!IsBOperator(wordBuffer[wbo])) &&
						(!IsBSeparator(wordBuffer[wbo])))) {
						wbo++;
					}
					// Colorize Default Text
					styler.ColourTo(startLine + offset - 1 - (wbl - wbo), SCE_BAT_DEFAULT);
					// Reset Offset to re-process remainder of word
					offset -= (wbl - wbo);
				}
				// Skip next spaces - nothing happens if Offset was Reset
				while ((offset < lengthLine) && (isspacechar(lineBuffer[offset]))) {
					offset++;
				}
			}
			// Colorize Default Text for remainder of line - currently not lexed
			styler.ColourTo(endPos, SCE_BAT_DEFAULT);

			// handle line continuation for SET and ECHO commands except the last line
			if (!continueProcessing && (i<startPos + length-1)) {
				if (linePos==1 || (linePos==2 && lineBuffer[1]=='\r')) // empty line on Unix and Mac or on Windows
					continueProcessing=true;
				else {
					Sci_PositionU lineContinuationPos;
					if ((linePos>2) && lineBuffer[linePos-2]=='\r') // Windows EOL
						lineContinuationPos=linePos-3;
					else
						lineContinuationPos=linePos-2; // Unix or Mac EOL
					// Reset continueProcessing	if line continuation was not found
					if ((lineBuffer[lineContinuationPos]!='^')
							|| IsEscaped(lineBuffer, lineContinuationPos)
							|| textQuoted(lineBuffer, lineContinuationPos))
						continueProcessing=true;
				}
			}

			linePos = 0;
			startLine = i + 1;
		}
	}
}

#ifdef RB_BALI
//!-start-[BatchLexerImprovement]
void FoldBatchDoc(Sci_PositionU startPos, Sci_Position length, int,
	WordList* [], Accessor& styler)
{
	int line = styler.GetLine(startPos);
	int level = styler.LevelAt(line);
	int levelIndent = 0;
	Sci_PositionU endPos = startPos + length;
	// Scan for ( and )
	for (Sci_PositionU i = startPos; i < endPos; i++) {
		int c = styler.SafeGetCharAt(i, '\n');
		int style = styler.StyleAt(i);
		if (style == SCE_BAT_OPERATOR) {
			// CheckFoldPoint
			if (c == '(') {
				levelIndent += 1;
			}
			else
				if (c == ')') {
					levelIndent -= 1;
				}
		}
		if (c == '\n') { // line end
			if (levelIndent > 0) {
				level |= SC_FOLDLEVELHEADERFLAG;
			}
			if (level != styler.LevelAt(line))
				styler.SetLevel(line, level);
			level += levelIndent;
			if ((level & SC_FOLDLEVELNUMBERMASK) < SC_FOLDLEVELBASE)
				level = SC_FOLDLEVELBASE;
			line++;
			// reset state
			levelIndent = 0;
			level &= ~SC_FOLDLEVELHEADERFLAG;
			level &= ~SC_FOLDLEVELWHITEFLAG;
		}
	}
}
//!-end-[BatchLexerImprovement]
#endif

const char *const batchWordListDesc[] = {
	"Internal Commands",
	"External Commands",
	nullptr
};

}

#ifdef RB_BALI
extern const LexerModule lmBatch(SCLEX_BATCH, ColouriseBatchDoc, "batch", FoldBatchDoc, batchWordListDesc); //!-change-[BatchLexerImprovement]
#else
extern const LexerModule lmBatch(SCLEX_BATCH, ColouriseBatchDoc, "batch", nullptr, batchWordListDesc);
#endif // RB_BALI
