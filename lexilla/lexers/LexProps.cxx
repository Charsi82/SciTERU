// Scintilla source code edit control
/** @file LexProps.cxx
 ** Lexer for properties files.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstdlib>
#include <cassert>

#include <string>
#include <string_view>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"

#include "WordList.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"

using namespace Lexilla;

namespace {

bool AtEOL(Accessor &styler, Sci_PositionU i) {
	return (styler[i] == '\n') ||
	       ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n'));
}

constexpr bool isAssignChar(char ch) noexcept {
	return (ch == '=') || (ch == ':');
}

#ifdef RB_PKW
	//!-start-[PropsKeywords]
	inline bool isprefix(std::string_view target, std::string_view prefix) noexcept {
		return target.starts_with(prefix);
	}
	//!-end-[PropsKeywords]
#endif

#ifdef RB_PCF
	char ColourisePropsLine(
#else
void ColourisePropsLine(
#endif // RB_PCF

#ifdef RB_PKS
		char* lineBuffer, //!- const removed [PropsKeysSets]
#else
	const char *lineBuffer,
#endif
	Sci_PositionU lengthLine,
	Sci_PositionU startLine,
	Sci_PositionU endPos,
#ifdef RB_PKS
		WordList* keywordlists[], //!-add-[PropsKeysSets]
#endif
	Accessor &styler,
	bool allowInitialSpaces) {

	Sci_PositionU i = 0;
	if (allowInitialSpaces) {
		while ((i < lengthLine) && isspacechar(lineBuffer[i]))	// Skip initial spaces
			i++;
	} else {
		if (isspacechar(lineBuffer[i])) // don't allow initial spaces
			i = lengthLine;
	}

	if (i < lengthLine) {
#ifdef RB_PCF
			if ((lineBuffer[i] == '#') && (lineBuffer[i + 1] == ' ' || lineBuffer[i + 1] == '#' || lineBuffer[i + 1] == '~')
				|| (lineBuffer[i] == '!'
					|| lineBuffer[i] == ';')) {
#else
		if (lineBuffer[i] == '#' || lineBuffer[i] == '!' || lineBuffer[i] == ';') {
#endif
			styler.ColourTo(endPos, SCE_PROPS_COMMENT);
#ifdef RB_PCF
				return SCE_PROPS_COMMENT; //!-add-[PropsColouriseFix]
#endif // RB_PCF
		} else if (lineBuffer[i] == '[') {
			styler.ColourTo(endPos, SCE_PROPS_SECTION);
#ifdef RB_PCF
				return SCE_PROPS_SECTION; //!-add-[PropsColouriseFix]
#endif // RB_PCF
		} else if (lineBuffer[i] == '@') {
			styler.ColourTo(startLine + i, SCE_PROPS_DEFVAL);
			if (isAssignChar(lineBuffer[i++]))
				styler.ColourTo(startLine + i, SCE_PROPS_ASSIGNMENT);
			styler.ColourTo(endPos, SCE_PROPS_DEFAULT);
			
#ifdef RB_PKW
			}
			//!-start-[PropsKeywords]
			else if (isprefix(lineBuffer, "import ")) {
				styler.ColourTo(startLine + 6, SCE_PROPS_KEYWORD);
				styler.ColourTo(endPos, SCE_PROPS_DEFAULT);
			}
			else if (isprefix(lineBuffer, "if ")) {
				styler.ColourTo(startLine + 2, SCE_PROPS_KEYWORD);
				styler.ColourTo(endPos, SCE_PROPS_DEFAULT);
			}
			else if (isprefix(lineBuffer, "match ")) {
				styler.ColourTo(startLine + 5, SCE_PROPS_KEYWORD);
				styler.ColourTo(endPos, SCE_PROPS_DEFAULT);
			//!-end-[PropsKeywords]
#endif // RB_PKW

			} else {
			// Search for the '=' character
			while ((i < lengthLine) && !isAssignChar(lineBuffer[i]))
				i++;
			if ((i < lengthLine) && isAssignChar(lineBuffer[i])) {
#ifdef RB_PKS
					//!-start-[PropsKeysSets]
					if (i > 0) {
						int chAttr = SCE_PROPS_KEY;
						lineBuffer[i] = '\0';
						// remove trailing spaces
						int indent = 0;
						while (lineBuffer[0] == ' ' || lineBuffer[0] == '\t') {
							lineBuffer++;
							indent++;
						}
						int len = 0, fin = 0;
						if ((*keywordlists[0]).InListPartly(lineBuffer, '~', len, fin)) {
							chAttr = SCE_PROPS_KEYSSET0;
						}
						else if ((*keywordlists[1]).InListPartly(lineBuffer, '~', len, fin)) {
							chAttr = SCE_PROPS_KEYSSET1;
						}
						else if ((*keywordlists[2]).InListPartly(lineBuffer, '~', len, fin)) {
							chAttr = SCE_PROPS_KEYSSET2;
						}
						else if ((*keywordlists[3]).InListPartly(lineBuffer, '~', len, fin)) {
							chAttr = SCE_PROPS_KEYSSET3;
						}
						styler.ColourTo(startLine + indent + len, chAttr);
						styler.ColourTo(startLine + i - 1 - fin, SCE_PROPS_KEY);
						styler.ColourTo(startLine + i - 1, chAttr);
					}
					//!-end-[PropsKeysSets]
#else
				styler.ColourTo(startLine + i - 1, SCE_PROPS_KEY);
#endif
				styler.ColourTo(startLine + i, SCE_PROPS_ASSIGNMENT);
				styler.ColourTo(endPos, SCE_PROPS_DEFAULT);
			} else {
				styler.ColourTo(endPos, SCE_PROPS_DEFAULT);
			}
		}
	} else {
		styler.ColourTo(endPos, SCE_PROPS_DEFAULT);
	}
#ifdef RB_PCF
		return SCE_PROPS_DEFAULT; //!-add-[PropsColouriseFix]
#endif // RB_PCF

}

#ifdef RB_PKS
	void ColourisePropsDoc(Sci_PositionU startPos, Sci_Position length, int, WordList * keywordlists[], Accessor & styler) {
#else
void ColourisePropsDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler) {
#endif // RB_PKS
	std::string lineBuffer;
	styler.StartAt(startPos);
	styler.StartSegment(startPos);
	Sci_PositionU startLine = startPos;

	// property lexer.props.allow.initial.spaces
	//	For properties files, set to 0 to style all lines that start with whitespace in the default style.
	//	This is not suitable for SciTE .properties files which use indentation for flow control but
	//	can be used for RFC2822 text where indentation is used for continuation lines.
	const bool allowInitialSpaces = styler.GetPropertyInt("lexer.props.allow.initial.spaces", 1) != 0;

#ifdef RB_PCF
		//!-start-[PropsColouriseFix]
		char style = SCE_PROPS_DEFAULT;
		bool continuation = false;
		if (startPos >= 3)
			continuation = styler.StyleAt(startPos - 2) != SCE_PROPS_COMMENT && ((styler[startPos - 2] == '\\')
				|| (styler[startPos - 3] == '\\' && styler[startPos - 2] == '\r'));
		//!-end-[PropsColouriseFix]
#endif // RB_PCF

	for (Sci_PositionU i = startPos; i < startPos + length; i++) {
		lineBuffer.push_back(styler[i]);
		if (AtEOL(styler, i)) {
			// End of line (or of line buffer) met, colourise it
#ifdef RB_PCF
				Sci_PositionU start = lineBuffer.find_first_of("#;!");
				bool bfindcomment = start != std::string::npos;
				while (bfindcomment && start)
					if (!isspacechar(lineBuffer[--start])) bfindcomment = false;
				if (bfindcomment) // comment detected
					// colorize line with comment and skip check continuation
					style = ColourisePropsLine(lineBuffer.data(), lineBuffer.length(), startLine, i, keywordlists, styler, allowInitialSpaces);
				else
				{
					if (continuation)
						styler.ColourTo(i, SCE_PROPS_DEFAULT);
					else
						style = ColourisePropsLine(lineBuffer.data(), lineBuffer.length(), startLine, i, keywordlists, styler, allowInitialSpaces);

					// test: is next a continuation of line
					continuation = (style != SCE_PROPS_COMMENT) && (lineBuffer.length() > 2) && (
						(lineBuffer[lineBuffer.length() - 2] == '\\') || // '\\\r'  or '\\\n'
						(lineBuffer[lineBuffer.length() - 3] == '\\' && lineBuffer[lineBuffer.length() - 2] == '\r')); // '\\\r\n'
				}

#else

#ifdef RB_PKS
			//if (styler.SafeGetCharAt(i - lineBuffer.size() - 2) == '\\' && styler.StyleAt(i - lineBuffer.size() - 2) != SCE_PROPS_COMMENT)
				//styler.ColourTo(i, SCE_PROPS_DEFAULT);
			//else
				ColourisePropsLine(lineBuffer.data(), lineBuffer.length(), startLine, i, keywordlists, styler, allowInitialSpaces);
#else
			ColourisePropsLine(lineBuffer.c_str(), lineBuffer.length(), startLine, i, styler, allowInitialSpaces);
#endif // RB_PKS

#endif
			lineBuffer.clear();
			startLine = i + 1;
		}
	}
	if (lineBuffer.length() > 0) {	// Last line does not have ending characters
#ifdef RB_PCF
			//!-start-[PropsColouriseFix]
			if (continuation)
				styler.ColourTo(startPos + length - 1, SCE_PROPS_DEFAULT);
			else
				//!-end-[PropsColouriseFix]
#endif // RB_PCF
#ifdef RB_PKS
				ColourisePropsLine(lineBuffer.data(), lineBuffer.length(), startLine, startPos + length - 1, keywordlists, styler, allowInitialSpaces);
#else
		ColourisePropsLine(lineBuffer.c_str(), lineBuffer.length(), startLine, startPos + length - 1, styler, allowInitialSpaces);
#endif // RB_PKS
	}
}

// adaption by ksc, using the "} else {" trick of 1.53
// 030721
void FoldPropsDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler) {
	const bool foldCompact = styler.GetPropertyInt("fold.compact", 1) != 0;

	const Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	Sci_Position lineCurrent = styler.GetLine(startPos);

	char chNext = styler[startPos];
	bool headerPoint = false;
	int levelPrevious = (lineCurrent > 0) ? styler.LevelAt(lineCurrent - 1) : SC_FOLDLEVELBASE;

	for (Sci_PositionU i = startPos; i < endPos; i++) {
		const char ch = chNext;
		chNext = styler[i+1];

		const int style = styler.StyleIndexAt(i);
		const bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');

		if (style == SCE_PROPS_SECTION) {
			headerPoint = true;
		}

		if (atEOL) {
			int lev = levelPrevious & SC_FOLDLEVELNUMBERMASK;
			if (headerPoint) {
				lev = SC_FOLDLEVELBASE | SC_FOLDLEVELHEADERFLAG;
				if (levelPrevious & SC_FOLDLEVELHEADERFLAG) {
					// previous section is empty
					styler.SetLevel(lineCurrent - 1, SC_FOLDLEVELBASE);
				}
			} else if (levelPrevious & SC_FOLDLEVELHEADERFLAG) {
				lev += 1;
			}

			if (visibleChars == 0 && foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}

			lineCurrent++;
			visibleChars = 0;
			headerPoint = false;
			levelPrevious = lev;
		}
		if (!isspacechar(ch))
			visibleChars++;
	}

	int level = levelPrevious & SC_FOLDLEVELNUMBERMASK;
	if (levelPrevious & SC_FOLDLEVELHEADERFLAG) {
		level += 1;
	}
	const int flagsNext = styler.LevelAt(lineCurrent);
	styler.SetLevel(lineCurrent, level | (flagsNext & ~SC_FOLDLEVELNUMBERMASK));
}

#ifdef RB_PKS
	//!-start-[PropsKeysSets]
	const char* const propsWordListDesc[] = {
		"Keys set 0",
		"Keys set 1",
		"Keys set 2",
		"Keys set 3",
		nullptr
	};
	//!-end-[PropsKeysSets]
#else
const char *const emptyWordListDesc[] = {
	nullptr
};
#endif // RB_PKS

}

#ifdef RB_PKS
extern const LexerModule lmProps(SCLEX_PROPERTIES, ColourisePropsDoc, "props", FoldPropsDoc, propsWordListDesc);
#else
extern const LexerModule lmProps(SCLEX_PROPERTIES, ColourisePropsDoc, "props", FoldPropsDoc, emptyWordListDesc);
#endif // RB_PKS
