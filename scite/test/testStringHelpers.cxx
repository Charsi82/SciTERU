/** @file testStringHelpers.cxx
 ** Unit Tests for SciTE internal data structures
 **/

#define _CRT_SECURE_NO_WARNINGS

#include <cstddef>
#include <cstring>
#include <cstdio>

#include <stdexcept>
#include <string_view>
#include <vector>
#include <set>
#include <optional>
#include <algorithm>
#include <memory>
#include <chrono>

#include "GUI.h"
#include "StringHelpers.h"
#include "Cookie.h"

#include "catch.hpp"

using namespace std::literals;

TEST_CASE("String Functions") {

	SECTION("Contains") {
		const std::string budgerigar("budgerigar");

		REQUIRE(Contains(budgerigar, 'g'));
		REQUIRE(!Contains(budgerigar, 'x'));
	}

	SECTION("Substitute") {
		std::string budgerigar("budgerigar");

		Substitute(budgerigar, "g", "j");
		REQUIRE(budgerigar == "budjerijar");

		Substitute(budgerigar, "ud", "D");
		REQUIRE(budgerigar == "bDjerijar");

		bool removed = RemoveStringOnce(budgerigar, "K");
		// Not present so not removed
		REQUIRE(!removed);
		REQUIRE(budgerigar == "bDjerijar");

		removed = RemoveStringOnce(budgerigar, "j");
		REQUIRE(removed);
		REQUIRE(budgerigar == "bDerijar");

		Remove(budgerigar, std::string("r"));
		REQUIRE(budgerigar == "bDeija");

		std::wstring wbudgerigar(L"budgerigar");
		const std::wstring wbudjerijar(L"budjerijar");

		Substitute(wbudgerigar, L"g", L"j");
		REQUIRE(wbudgerigar == wbudjerijar);
	}

	SECTION("Trim") {
		std::string budgerigar("  budgerigar   ");
		Trim(budgerigar);
		REQUIRE(budgerigar == "budgerigar");
		Trim(budgerigar);
		REQUIRE(budgerigar == "budgerigar");
	}

	SECTION("StripEOL") {
		{
			std::string wombat("wombat\n");
			StripEOL(wombat);
			REQUIRE(wombat == "wombat");
		}
		{
			std::string wombat("wombat\r");
			StripEOL(wombat);
			REQUIRE(wombat == "wombat");
		}
		{
			std::string wombat("wombat\r\n");
			StripEOL(wombat);
			REQUIRE(wombat == "wombat");
		}
		{
			std::string wombat("wombat");
			StripEOL(wombat);
			REQUIRE(wombat == "wombat");
		}
		{
			std::string wombat("wombat ");
			StripEOL(wombat);
			REQUIRE(wombat == "wombat ");
		}
	}

	SECTION("LowerCase") {
		std::string numbat("NumBat");
		LowerCaseAZ(numbat);
		REQUIRE(numbat == "numbat");
		LowerCaseAZ(numbat);
		REQUIRE(numbat == "numbat");

		REQUIRE(MakeUpperCase('a') == 'A');
		REQUIRE(MakeUpperCase('A') == 'A');
		REQUIRE(MakeUpperCase('1') == '1');

		REQUIRE(MakeLowerCase('A') == 'a');
		REQUIRE(MakeLowerCase('a') == 'a');
		REQUIRE(MakeLowerCase('1') == '1');
	}
}

TEST_CASE("Number Functions") {

	SECTION("String based") {
		REQUIRE(StdStringFromInteger(0) == "0");
		REQUIRE(StdStringFromInteger(1) == "1");
		REQUIRE(StdStringFromInteger(-1) == "-1");
		REQUIRE(StdStringFromInteger(5678) == "5678");

		REQUIRE(StdStringFromSizeT(0) == "0");
		REQUIRE(StdStringFromSizeT(1) == "1");
		REQUIRE(StdStringFromSizeT(1400) == "1400");
		REQUIRE(StdStringFromSizeT(12'345'678'901'234LLU) == "12345678901234");

		REQUIRE(StdStringFromDouble(0, 0) == "0");
		REQUIRE(StdStringFromDouble(1, 0) == "1");
		REQUIRE(StdStringFromDouble(-1, 0) == "-1");
		REQUIRE(StdStringFromDouble(5678, 0) == "5678");

		REQUIRE(StdStringFromDouble(1.23, 0) == "1");
		REQUIRE(StdStringFromDouble(1.23, 1) == "1.2");
		REQUIRE(StdStringFromDouble(1.23, 2) == "1.23");
		REQUIRE(StdStringFromDouble(-1.23, 2) == "-1.23");

		REQUIRE(IntegerFromString("0", -1) == 0);
		REQUIRE(IntegerFromString("1", -1) == 1);
		REQUIRE(IntegerFromString("-2", -1) == -2);
		REQUIRE(IntegerFromString("", -1) == -1);
		// Too big
		REQUIRE(IntegerFromString("12345678901234", -1) == -1);

		REQUIRE(IntPtrFromString("0", -1) == 0);
		REQUIRE(IntPtrFromString("1", -1) == 1);
		REQUIRE(IntPtrFromString("-2", -1) == -2);
		REQUIRE(IntPtrFromString("", -1) == -1);
		// Only for 64-bit
		if constexpr (sizeof(intptr_t) == 8) {
			REQUIRE(IntPtrFromString("12345678901234", -1) == 12'345'678'901'234LL);
		}

		REQUIRE(LongLongFromString("0", -1) == 0);
		REQUIRE(LongLongFromString("1", -1) == 1);
		REQUIRE(LongLongFromString("", -1) == -1);
		REQUIRE(LongLongFromString("-2", -1) == -2);
		REQUIRE(LongLongFromString("12345678901234", -1) == 12'345'678'901'234LL);
	}

	SECTION("IntegerFromText") {
		REQUIRE(IntegerFromText("0") == 0);
		REQUIRE(IntegerFromText("1") == 1);
		REQUIRE(IntegerFromText("-2") == -2);
		REQUIRE(IntegerFromText("") == 0);
		// Only for 64-bit
		if constexpr (sizeof(intptr_t) == 8) {
			REQUIRE(IntegerFromText("12345678901234") == 12'345'678'901'234LL);
		}
	}

}

TEST_CASE("Is") {

	SECTION("IsX") {
		REQUIRE(IsASCII('A'));

		REQUIRE(IsASpace(' '));
		REQUIRE(IsASpace('\t'));
		REQUIRE(IsASpace('\n'));
		REQUIRE(!IsASpace('A'));

		REQUIRE(IsSpaceOrTab(' '));
		REQUIRE(IsSpaceOrTab('\t'));
		REQUIRE(!IsSpaceOrTab('\n'));
		REQUIRE(!IsSpaceOrTab('A'));

		REQUIRE(IsEOLCharacter('\r'));
		REQUIRE(IsEOLCharacter('\r'));
		REQUIRE(!IsEOLCharacter(' '));
		REQUIRE(!IsEOLCharacter('A'));

		REQUIRE(IsADigit('7'));
		REQUIRE(!IsADigit('A'));

		REQUIRE(IsAHexDigit('7'));
		REQUIRE(IsAHexDigit('A'));
		REQUIRE(IsAHexDigit('a'));
		REQUIRE(!IsAHexDigit('G'));

		REQUIRE(IsUpperCase('A'));
		REQUIRE(!IsUpperCase('a'));
		REQUIRE(!IsUpperCase('7'));

		REQUIRE(IsAlphabetic('A'));
		REQUIRE(IsAlphabetic('a'));
		REQUIRE(!IsAlphabetic('7'));
		REQUIRE(!IsAlphabetic('%'));

		REQUIRE(IsAlphaNumeric('A'));
		REQUIRE(IsAlphaNumeric('a'));
		REQUIRE(IsAlphaNumeric('7'));
		REQUIRE(!IsAlphaNumeric('%'));
	}
}

TEST_CASE("Split") {

	SECTION("StringSplit") {
		std::string abc("a,b,c");
		auto v = StringSplit(abc, ',');
		REQUIRE(v.size() == 3);

		auto gv = ListFromString(GUI_TEXT(" a\nb \nc\nd "));
		REQUIRE(gv.size() == 4);
		REQUIRE(gv[0] == GUI_TEXT(" a"));
		REQUIRE(gv[1] == GUI_TEXT("b "));
		REQUIRE(gv[2] == GUI_TEXT("c"));
		REQUIRE(gv[3] == GUI_TEXT("d "));
	}

	SECTION("SetFromString") {
		auto s = SetFromString("a,b,c", ',');
		REQUIRE(s.size() == 3);
		std::set<std::string> ss{ "a", "b", "c" };
		REQUIRE(s == ss);
	}

	SECTION("ViewSplit") {
		auto [a,b] = ViewSplit("a b c", ' ');
		REQUIRE(a == "a");
		REQUIRE(b == "b c");
	}
}

TEST_CASE("Archaic") {

	SECTION("StringCopy") {
		char ar[4]{};
		StringCopy(ar, "abc");
		REQUIRE(strcmp(ar, "abc") == 0);
		REQUIRE(ar[3] == 0);

		// Can't write the 'd' as the last char must remain NUL
		StringCopy(ar, "abcd");
		REQUIRE(strcmp(ar, "abc") == 0);
		REQUIRE(ar[3] == 0);
		StringCopy(ar, "abcde");
		REQUIRE(strcmp(ar, "abc") == 0);
		REQUIRE(ar[3] == 0);
		StringCopy(ar, "ab");
		REQUIRE(strcmp(ar, "ab") == 0);
		REQUIRE(ar[2] == 0);
		REQUIRE(ar[3] == 0);
	}

}

TEST_CASE("Comparison") {

	SECTION("CompareNoCase") {
		// Three way
		REQUIRE(CompareNoCase("aBc", "abc") == 0);
		REQUIRE(CompareNoCase("aBc", "abD") < 0);
		REQUIRE(CompareNoCase("acd", "abD") > 0);

		REQUIRE(EqualCaseInsensitive("aBc", "abc"));
		REQUIRE(!EqualCaseInsensitive("aBcd", "abc"));

		REQUIRE(EqualCaseInsensitive("aBc"sv, "abc"sv));
		REQUIRE(!EqualCaseInsensitive("aBcd"sv, "abc"sv));
	}

	SECTION("Prefix") {
		REQUIRE(isprefix("abc", "ab"));
		REQUIRE(!isprefix("ab", "abc"));
	}

}

TEST_CASE("Unicode") {
	SECTION("UTF32Character") {
		// a $ Cent Euro
		REQUIRE(UTF32Character("a") == static_cast<unsigned int>('a'));
		REQUIRE(UTF32Character("\x24") == 0x24);
		REQUIRE(UTF32Character("\xC2\xA2") == 0xA2);
		REQUIRE(UTF32Character("\xE2\x82\xAC") == 0x20AC);
	}

	SECTION("UTF32FromUTF8") {
		const std::u32string xs = UTF32FromUTF8("^\xC2\xA2$"); // ^ Cent $
		REQUIRE(xs.length() == 3U);
		REQUIRE(xs[0] == '^');
		REQUIRE(xs[1] == 0xA2);
		REQUIRE(xs[2] == '$');
	}

	SECTION("UTF32FromUTF8 With Reverse") {
		// Hwair
		const char s[] = "\xF0\x90\x8D\x88";
		const std::u32string xs = UTF32FromUTF8(s);
		REQUIRE(xs.length() == 1U);
		REQUIRE(xs[0] == 0x10348);
		REQUIRE(UTF32Character(s) == 0x10348);
		// reverse
		std::string us = UTF8FromUTF32(0x10348);
		REQUIRE(us == s);
	}

}

TEST_CASE("Escape") {

	SECTION("Slash") {
		std::string slashed = Slash("x\n"s, false);
		REQUIRE(slashed == "x\\n");

		std::string unSlashed = UnSlashString(slashed);
		REQUIRE(unSlashed == "x\n");

		std::string unSlashedLO = UnSlashLowOctalString("x\\001\\013y");
		REQUIRE(unSlashedLO == "x\001\013y");

		char s[30] = "a\\1\\r\\n\\xFEx";
		const std::string complexString = UnSlashString(s);
		REQUIRE(complexString.length() == 6);
		REQUIRE(complexString == "a\001\r\n\xFEx");
	}

	SECTION("UnicodeUnEscape") {
		{
			std::string uue = UnicodeUnEscape("abc");
			REQUIRE(uue == "abc");
		}
		{
			// _ Cent _ Euro _ Hwair _
			const char escaped[] = "_\\xA2_\\u20AC_\\U00010348_";
			const char utf[] = "_\xC2\xA2_\xE2\x82\xAC_\xF0\x90\x8D\x88_";
			std::string uue = UnicodeUnEscape(escaped);
			REQUIRE(uue == utf);
		}
	}
}

TEST_CASE("HexDigits") {

	SECTION("IntFromHexDigit") {
		REQUIRE(IntFromHexDigit('1') == 1);
		REQUIRE(IntFromHexDigit('a') == 10);
		REQUIRE(IntFromHexDigit('B') == 11);
		REQUIRE(IntFromHexDigit('x') == 0);

		REQUIRE(AllBytesHex("abc"));
		REQUIRE(AllBytesHex("DE"));
		REQUIRE(AllBytesHex(""));
		REQUIRE(!AllBytesHex("zy"));
		REQUIRE(!AllBytesHex("aBy"));

		REQUIRE(IntFromHexBytes("abc") == 0xABC);
		REQUIRE(IntFromHexBytes("DE") == 0xDE);
		REQUIRE(IntFromHexBytes("") == 0);
		REQUIRE(IntFromHexBytes("zy") == 0);
		REQUIRE(IntFromHexBytes("aBy") == 0xAB0);	// Fails, must be checked before
	}
}

TEST_CASE("ComboMemory") {

	SECTION("ComboMemory") {
		ComboMemory cm(4);

		REQUIRE(cm.Length() == 0);

		cm.Append("a");
		REQUIRE(cm.Length() == 1);
		REQUIRE(cm.At(0) == "a");

		cm.Insert("b");
		REQUIRE(cm.Length() == 2);
		REQUIRE(cm.At(0) == "b");
		REQUIRE(cm.At(1) == "a");

		cm.InsertDeletePrefix("bc");
		REQUIRE(cm.Length() == 2);
		REQUIRE(cm.At(0) == "bc");
		REQUIRE(cm.At(1) == "a");

		cm.Append("d");
		REQUIRE(cm.Length() == 3);

		cm.Append("e");
		REQUIRE(cm.Length() == 4);

		// Bound to allocated length, discarding last
		cm.Append("f");
		REQUIRE(cm.Length() == 4);

		cm.Insert("g");
		REQUIRE(cm.Length() == 4);

		auto v = cm.AsVector();
		REQUIRE(v.size() == 4);
		REQUIRE(v[0] == "g");
		REQUIRE(v[1] == "bc");
		REQUIRE(v[2] == "a");
		REQUIRE(v[3] == "d");
	}
}
