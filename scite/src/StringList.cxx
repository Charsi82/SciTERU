// SciTE - Scintilla based Text Editor
/** @file StringList.cxx
 ** Implementation of class holding a list of strings.
 **/
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <cstring>

#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <array>
#include <map>
#include <set>
#include <algorithm>
#include <ranges>
#include <chrono>

#include "GUI.h"
#include "StringList.h"
#include "StringHelpers.h"

namespace {

int CompareNCaseInsensitive(const char *a, const char *b, size_t len) noexcept {
	while (*a && *b && len) {
		if (*a != *b) {
			const char upperA = MakeUpperCase(*a);
			const char upperB = MakeUpperCase(*b);
			if (upperA != upperB)
				return upperA - upperB;
		}
		a++;
		b++;
		len--;
	}
	if (len == 0)
		return 0;
	// Either *a or *b is nul
	return *a - *b;
}

/**
 * Creates an array that points into each word in the string and puts \0 terminators
 * after each word.
 */
std::vector<char *> ArrayFromStringList(std::string &stringList, bool onlyLineEnds = false) {
	// For rapid determination of whether a character is a separator, build
	// a look up table.
	constexpr size_t byteValues = 0x100;
	std::array<bool, byteValues> wordSeparator = {};
	wordSeparator[static_cast<unsigned int>('\r')] = true;
	wordSeparator[static_cast<unsigned int>('\n')] = true;
	if (!onlyLineEnds) {
		wordSeparator[static_cast<unsigned int>(' ')] = true;
		wordSeparator[static_cast<unsigned int>('\t')] = true;
	}

	std::vector<char *> keywords;
	int prev = '\0';
	for (char &ch : stringList) {
		if (wordSeparator[static_cast<unsigned char>(ch)]) {
			ch = '\0';
		} else if (!prev) {
			keywords.push_back(&ch);
		}
		prev = ch;
	}
	return keywords;
}

bool CmpString(const char *a, const char *b) noexcept {
	return strcmp(a, b) < 0;
}

bool CmpStringNoCase(const char *a, const char *b) noexcept {
	return CompareNoCase(a, b) < 0;
}

// Functors used to find elements given a prefix

struct CompareString {
	size_t searchLen;
	explicit CompareString(size_t searchLen_) noexcept : searchLen(searchLen_) {}
	bool operator()(const char *a, const char *b) const noexcept {
		return strncmp(a, b, searchLen) < 0;
	}
};

struct CompareStringInsensitive {
	size_t searchLen;
	explicit CompareStringInsensitive(size_t searchLen_) noexcept : searchLen(searchLen_) {}
	bool operator()(const char *a, const char *b) const noexcept {
		return CompareNCaseInsensitive(a, b, searchLen) < 0;
	}
};

template<typename Compare>
std::string GetMatch(const std::vector<char *> &vector,
	const char *wordStart, const std::string &wordCharacters, ptrdiff_t wordIndex, Compare comp) {
	std::vector<char *>::const_iterator elem = std::lower_bound(vector.begin(), vector.end(), wordStart, comp);
	if (!comp(wordStart, *elem) && !comp(*elem, wordStart)) {
		// Found a matching element, now move forward wordIndex matching elements
		for (; elem < vector.end(); ++elem) {
			const char *word = *elem;
			if (!word[comp.searchLen] || !Contains(wordCharacters, word[comp.searchLen])) {
				if (wordIndex <= 0) {
					return {word};
				}
				wordIndex--;
			}
		}
	}
	return {};
}

/**
 * Find the length of a 'word' which is actually an identifier in a string
 * which looks like "identifier(..." or "identifier" and where
 * there may be extra spaces after the identifier that should not be
 * counted in the length.
 */
size_t LengthWord(const char *word, char otherSeparator) noexcept {
	const char *endWord = nullptr;
	// Find an otherSeparator
	if (otherSeparator)
		endWord = strchr(word, otherSeparator);
	// Find a '('. If that fails go to the end of the string.
	if (!endWord)
		endWord = strchr(word, '(');
	if (!endWord)
		endWord = word + strlen(word);
	assert(endWord);
	// Last case always succeeds so endWord != 0

	// Drop any space characters.
	if (endWord > word) {
		endWord--;	// Back from the '(', otherSeparator, or '\0'
		// Move backwards over any spaces
		while ((endWord > word) && (IsASpace(*endWord))) {
			endWord--;
		}
	}
	return endWord - word + 1;
}

template<typename Compare>
StringVector GetMatches(const std::vector<char *> &vector, const char *wordStart, char otherSeparator, bool exactLen, Compare comp) {
	StringVector wordList;
	const size_t wordStartLength = LengthWord(wordStart, otherSeparator);
	std::vector<char *>::const_iterator elem = std::lower_bound(vector.begin(), vector.end(), wordStart, comp);
	// Found a matching element, now accumulate all matches
	for (; elem < vector.end(); ++elem) {
		if (comp(wordStart, *elem) || comp(*elem, wordStart))
			break;	// Not a match so stop
		// length of the word part (before the '(' brace) of the api array element
		const size_t wordlen = LengthWord(*elem, otherSeparator);
		if (!exactLen || (wordlen == wordStartLength)) {
			wordList.emplace_back(*elem, wordlen);
		}
	}
	return wordList;
}

}

void StringList::SetFromListText() {
	sorted = false;
	sortedNoCase = false;
	words = ArrayFromStringList(listText, onlyLineEnds);
	wordsNoCase = words;
}

void StringList::SortIfNeeded(bool ignoreCase) {
	// In both cases, the final empty sentinel element is not sorted.
	if (ignoreCase) {
		if (!sortedNoCase) {
			sortedNoCase = true;
			std::ranges::sort(wordsNoCase, CmpStringNoCase);
		}
	} else {
		if (!sorted) {
			sorted = true;
			std::ranges::sort(words, CmpString);
		}
	}
}

void StringList::Clear() noexcept {
	words.clear();
	wordsNoCase.clear();
	listText.clear();
	sorted = false;
	sortedNoCase = false;
}

void StringList::Set(std::string_view data) {
	listText.assign(data);
	SetFromListText();
}

/**
 * Returns an element (complete) of the StringList array which has
 * the same beginning as the passed string.
 * The length of the word to compare is passed too.
 * Letter case can be ignored or preserved.
 */
std::string StringList::GetNearestWord(std::string_view word, bool ignoreCase, const std::string &wordCharacters, ptrdiff_t wordIndex) {
	if (words.empty())
		return {};
	SortIfNeeded(ignoreCase);
	if (ignoreCase) {
		return GetMatch(wordsNoCase, word.data(), wordCharacters, wordIndex, CompareStringInsensitive(word.length()));
	}
	// preserve the letter case
	return GetMatch(words, word.data(), wordCharacters, wordIndex, CompareString(word.length()));
}

/**
 * Returns elements (first words of them) of the StringList array which have
 * the same beginning as the passed string.
 * The length of the word to compare is passed too.
 * Letter case can be ignored or preserved (default).
 * If there are more words meeting the condition they are returned all of
 * them in the ascending order separated with spaces.
 */
StringVector StringList::GetNearestWords(
	std::string_view word,
	bool ignoreCase,
	char otherSeparator /*= '\0'*/,
	bool exactLen /*=false*/) {

	if (words.empty())
		return {};
	SortIfNeeded(ignoreCase);
	if (ignoreCase) {
		return GetMatches(wordsNoCase, word.data(), otherSeparator, exactLen, CompareStringInsensitive(word.length()));
	}
	// Preserve the letter case
	return GetMatches(words, word.data(), otherSeparator, exactLen, CompareString(word.length()));
}

bool AutoCompleteWordList::Add(const std::string& word) {
	const auto [_, ok] = words.insert(word);
	if (ok) {
		const size_t length = word.length();
		totalLength += 1 + length;
		minWordLength = std::min(minWordLength, length);
		return true;
	}
	return ok;
}

std::string AutoCompleteWordList::Sorted(bool ignoreCase) const {
	std::vector<std::string> wv(words.begin(), words.end());
	if (ignoreCase) {
		std::ranges::sort(wv,
			[](const std::string &a, const std::string &b) noexcept -> bool {
				return CmpStringNoCase(a.c_str(), b.c_str());
			});
	}
	std::string result;
	if (totalLength) {
		for (const std::string &word : wv) {
			if (!result.empty())
				result.push_back('\n');
			result.append(word);
		}
	}
	return result;
}
