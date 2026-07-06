// SciTE - Scintilla based Text Editor
/** @file StringList.h
 ** Definition of class holding a list of strings.
 **/
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef STRINGLIST_H
#define STRINGLIST_H

using StringVector = std::vector<std::string>;

class StringList {
	// Text pointed into by words and wordsNoCase
	std::string listText;
	// Each word contains at least one character.
	std::vector<char *> words;
	std::vector<char *> wordsNoCase;
	bool onlyLineEnds;	///< Delimited by any white space or only line ends
	bool sorted = false;
	bool sortedNoCase = false;
	void SetFromListText();
	void SortIfNeeded(bool ignoreCase);
public:
	explicit StringList(bool onlyLineEnds_ = false) : onlyLineEnds(onlyLineEnds_) {
	}
	[[nodiscard]] size_t Length() const noexcept { return words.size(); }
	[[nodiscard]] operator bool() const noexcept { return !words.empty(); }
	char *operator[](size_t ind) noexcept { return words[ind]; }
	void Clear() noexcept;
	void Set(std::string_view data);
	std::string GetNearestWord(std::string_view word, bool ignoreCase, const std::string &wordCharacters, ptrdiff_t wordIndex);
	StringVector GetNearestWords(std::string_view word, bool ignoreCase, char otherSeparator='\0', bool exactLen=false);
};

class AutoCompleteWordList {
	std::set<std::string> words;
	size_t totalLength = 0;
	size_t minWordLength = SIZE_MAX;
public:
	[[nodiscard]] size_t Count() const noexcept {
		return words.size();
	}
	[[nodiscard]] size_t MinWordLength() const noexcept {
		return minWordLength;
	}
	bool Add(const std::string& word);
	[[nodiscard]] std::string Sorted(bool ignoreCase) const;
};

#endif
