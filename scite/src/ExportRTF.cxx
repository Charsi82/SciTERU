// SciTE - Scintilla based Text Editor
/** @file ExportRTF.cxx
 ** Export the current document to RTF.
 **/
// Copyright 1998-2006 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <ctime>

#include <compare>
#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <memory>
#include <chrono>
#include <sstream>
#include <atomic>
#include <mutex>

#include <fcntl.h>
#include <sys/stat.h>

#include "ILexer.h"

#include "ScintillaTypes.h"
#include "ScintillaCall.h"

#include "GUI.h"
#include "ScintillaWindow.h"

#include "StringList.h"
#include "StringHelpers.h"
#include "FilePath.h"
#include "StyleDefinition.h"
#include "PropSetFile.h"
#include "StyleWriter.h"
#include "Extender.h"
#include "SciTE.h"
#include "JobQueue.h"
#include "Cookie.h"
#include "Worker.h"
#include "MatchMarker.h"
#include "Searcher.h"
#include "SciTEBase.h"


//---------- Save to RTF ----------

#define RTF_HEADEROPEN "{\\rtf1\\ansi\\deff0\\deftab720"
#define RTF_FONTDEFOPEN "{\\fonttbl"
#define RTF_FONTDEFCLOSE "}"
#define RTF_COLORDEFOPEN "{\\colortbl"
#define RTF_COLORDEFCLOSE "}"
#define RTF_HEADERCLOSE "\n"
#define RTF_BODYOPEN ""
#define RTF_BODYCLOSE "}"

#define RTF_SETFONTFACE "\\f"
#define RTF_SETFONTSIZE "\\fs"
#define RTF_SETCOLOR "\\cf"
#define RTF_SETBACKGROUND "\\highlight"
#define RTF_BOLD_ON "\\b"
#define RTF_BOLD_OFF "\\b0"
#define RTF_ITALIC_ON "\\i"
#define RTF_ITALIC_OFF "\\i0"
#define RTF_UNDERLINE_ON "\\ul"
#define RTF_UNDERLINE_OFF "\\ulnone"
#define RTF_STRIKE_ON "\\i"
#define RTF_STRIKE_OFF "\\strike0"

#define RTF_EOLN "\\par\n"
#define RTF_TAB "\\tab "

#define RTF_FONTFACE "Courier New"
#define RTF_COLOR "#000000"

namespace {

size_t FindCaseInsensitive(const std::vector<std::string> &values, const std::string &s) noexcept {
	for (size_t i = 0; i < values.size(); i++)
		if (EqualCaseInsensitive(s, values[i]))
			return i;
	return values.size();
}

// extract the next RTF control word from *style
std::string_view GetRTFNextControl(const char **style) noexcept {
	const char *pos = *style;
	if ('\0' == *pos) return {};
	pos++; // implicit skip over leading '\'
	while ('\0' != *pos && '\\' != *pos) { pos++; }
	const std::string_view ret(*style, pos - *style);
	*style = pos;
	return ret;
}

// extracts control words that are different between two styles
std::string GetRTFStyleChange(const std::string &last, const std::string &current) { // \f0\fs20\cf0\highlight0\b0\i0
	const char *lastPos = last.c_str();
	const char *currentPos = current.c_str();
	std::string delta;
	// font face, size, color, background, bold, italic
	for (int i = 0; i < 6; i++) {
		const std::string_view lastControl = GetRTFNextControl(&lastPos);
		const std::string_view currentControl = GetRTFNextControl(&currentPos);
		if (lastControl != currentControl) {	// changed
			delta += currentControl;
		}
	}
	if (!delta.empty()) { delta += " "; }
	return delta;
}

}

void SciTEBase::SaveToStreamRTF(std::ostream &os, SA::Position start, SA::Position end) {
	const SA::Position lengthDoc = LengthDocument();
	if (end < 0)
		end = lengthDoc;
	RemoveFindMarks();
	wEditor.ColouriseAll();

	StyleDefinition defaultStyle = StyleDefinitionFor(StyleDefault);

	int tabSize = props.GetInt("export.rtf.tabsize", props.GetInt("tabsize"));
	const int wysiwyg = props.GetInt("export.rtf.wysiwyg", 1);
	std::string fontFace = props.GetExpandedString("export.rtf.font.face");
	if (fontFace.length()) {
		defaultStyle.font = fontFace;
	} else if (defaultStyle.font.length() == 0) {
		defaultStyle.font = RTF_FONTFACE;
	}

#ifdef RB_SD
	//!-start-[StyleDefault]
	if (defaultStyle.fore == "") defaultStyle.fore = "#000000";
	if (defaultStyle.back == "") defaultStyle.back = "#FFFFFF";
	//!-end-[StyleDefault]
#endif // RB_SD

	const int fontSize = props.GetInt("export.rtf.font.size", 0);
	if (fontSize > 0) {
		defaultStyle.size = fontSize << 1;
	} else if (defaultStyle.size == 0) {
		defaultStyle.size = 10 << 1;
	} else {
		defaultStyle.size <<= 1;
	}
	const bool isUTF8 = wEditor.CodePage() == SA::CpUtf8;
	const unsigned int characterset = props.GetInt("character.set", static_cast<int>(SA::CharacterSet::Default));
	const int tabs = props.GetInt("export.rtf.tabs", 0);
	if (tabSize == 0)
		tabSize = 4;

	std::vector<std::string> styles;
	std::vector<std::string> fonts;
	std::vector<std::string> colors;
	os << RTF_HEADEROPEN << RTF_FONTDEFOPEN;
	fonts.push_back(defaultStyle.font);
	os << "{\\f" << 0 << "\\fnil\\fcharset" << characterset << " " << defaultStyle.font << ";}";
	colors.push_back(defaultStyle.fore);
	colors.push_back(defaultStyle.back);

	for (int istyle = 0; istyle <= StyleMax; istyle++) {
		std::ostringstream osStyle;

		const StyleDefinition sd = StyleDefinitionFor(istyle);

		if (sd.specified != StyleDefinition::sdNone) {
			size_t iFont = 0;
			if (wysiwyg && sd.font.length()) {
				iFont = FindCaseInsensitive(fonts, sd.font);
				if (iFont >= fonts.size()) {
					fonts.push_back(sd.font);
					os << "{\\f" << iFont << "\\fnil\\fcharset" << characterset << " " << sd.font << ";}";
				}
			}
			osStyle << RTF_SETFONTFACE << iFont;

			osStyle << RTF_SETFONTSIZE << (wysiwyg && sd.size ? sd.size << 1 : defaultStyle.size);

			size_t iFore = 0;
			if (sd.specified & StyleDefinition::sdFore) {
				iFore = FindCaseInsensitive(colors, sd.fore);
				if (iFore >= colors.size())
					colors.push_back(sd.fore);
			}
			osStyle << RTF_SETCOLOR << iFore;

			// PL: highlights doesn't seems to follow a distinct table, at least with WordPad and Word 97
			// Perhaps it is different for Word 6?
			size_t iBack = 1;
			if (sd.specified & StyleDefinition::sdBack) {
				iBack = FindCaseInsensitive(colors, sd.back);
				if (iBack >= colors.size())
					colors.push_back(sd.back);
			}
			osStyle << RTF_SETBACKGROUND << iBack;

			if (sd.specified & StyleDefinition::sdWeight) {
				osStyle << (sd.IsBold() ? RTF_BOLD_ON : RTF_BOLD_OFF);
			} else {
				osStyle << (defaultStyle.IsBold() ? RTF_BOLD_ON : RTF_BOLD_OFF);
			}
			if (sd.specified & StyleDefinition::sdItalics) {
				osStyle << (sd.italics ? RTF_ITALIC_ON : RTF_ITALIC_OFF);
			} else {
				osStyle << (defaultStyle.italics ? RTF_ITALIC_ON : RTF_ITALIC_OFF);
			}
		} else {
			osStyle << RTF_SETFONTFACE "0" RTF_SETFONTSIZE << defaultStyle.size <<
				RTF_SETCOLOR "0" RTF_SETBACKGROUND "1"
				RTF_BOLD_OFF RTF_ITALIC_OFF;
		}
		styles.push_back(osStyle.str());
	}
	os << RTF_FONTDEFCLOSE RTF_COLORDEFOPEN;
	for (std::string color : colors) {
		color.append("0000000");	// Ensure substr safe by providing at least 7 characters
		os << "\\red" << IntFromHexByte(color.substr(1,2)) <<
			"\\green" << IntFromHexByte(color.substr(3,2)) <<
		   "\\blue" << IntFromHexByte(color.substr(5,2)) << ";";
	}
	os << RTF_COLORDEFCLOSE RTF_HEADERCLOSE RTF_BODYOPEN RTF_SETFONTFACE "0"
	   RTF_SETFONTSIZE << defaultStyle.size << RTF_SETCOLOR "0 ";
	std::ostringstream osStyleDefault;
	osStyleDefault << RTF_SETFONTFACE "0" RTF_SETFONTSIZE << defaultStyle.size <<
		       RTF_SETCOLOR "0" RTF_SETBACKGROUND "1"
		       RTF_BOLD_OFF RTF_ITALIC_OFF;
	std::string lastStyle = osStyleDefault.str();
	bool prevCR = false;
	int styleCurrent = -1;
	TextReader acc(wEditor);
	int column = 0;
	for (SA::Position iPos = start; iPos < end; iPos++) {
		const char ch = acc[iPos];
		int style = acc.StyleAt(iPos);
		if (style > StyleMax)
			style = 0;
		if (style != styleCurrent) {
			const std::string deltaStyle = GetRTFStyleChange(lastStyle, styles[style]);
			lastStyle = styles[style];
			if (!deltaStyle.empty())
				os << deltaStyle;
			styleCurrent = style;
		}
		if (ch == '{')
			os << "\\{";
		else if (ch == '}')
			os << "\\}";
		else if (ch == '\\')
			os << "\\\\";
		else if (ch == '\t') {
			if (tabs) {
				os << RTF_TAB;
			} else {
				const int ts = tabSize - (column % tabSize);
				for (int itab = 0; itab < ts; itab++) {
					os << ' ';
				}
				column += ts - 1;
			}
		} else if (ch == '\n') {
			if (!prevCR) {
				os << RTF_EOLN;
				column = -1;
			}
		} else if (ch == '\r') {
			os << RTF_EOLN;
			column = -1;
		} else if (isUTF8 && !IsASCII(ch)) {
			const SA::Position nextPosition = wEditor.PositionAfter(iPos);
			wEditor.SetTarget(SA::Span(iPos, nextPosition));
			char u8Char[5] = "";
			wEditor.TargetAsUTF8(u8Char);
			const unsigned int u32 = UTF32Character(u8Char);
			if (u32 < 0x10000) {
				os << "\\u" << static_cast<short>(u32) << "?";
			} else {
				os << "\\u" << static_cast<short>(((u32 - 0x10000) >> 10) + 0xD800) << "?";
				os << "\\u" << static_cast<short>((u32 & 0x3ff) + 0xDC00) << "?";
			}
			iPos = nextPosition - 1;
		} else {
			os << ch;
		}
		column++;
		prevCR = ch == '\r';
	}
	os << RTF_BODYCLOSE;
}

void SciTEBase::SaveToRTF(const FilePath &saveName, SA::Position start, SA::Position end) {
	FILE *fp = saveName.Open(GUI_TEXT("wt"));
	bool failedWrite = fp == nullptr;
	if (fp) {
		try {
			std::ostringstream oss;
			SaveToStreamRTF(oss, start, end);
			const std::string rtf = oss.str();
			if (fwrite(rtf.c_str(), 1, rtf.length(), fp) != rtf.length()) {
				failedWrite = true;
			}
			if (fclose(fp) != 0) {
				failedWrite = true;
			}
		} catch (std::exception &) {
			failedWrite = true;
		}
	}
	if (failedWrite) {
		FailedSaveMessageBox(saveName);
	}
}

