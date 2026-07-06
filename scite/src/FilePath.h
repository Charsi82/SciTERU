// SciTE - Scintilla based Text Editor
/** @file FilePath.h
 ** Definition of platform independent base class of editor.
 **/
// Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef FILEPATH_H
#define FILEPATH_H

extern const GUI::gui_char pathSepString[];
extern const GUI::gui_char listSepString[];
extern const GUI::gui_char configFileVisibilityString[];

constexpr const GUI::gui_char *fileRead = GUI_TEXT("rb");
constexpr const GUI::gui_char *fileWrite = GUI_TEXT("wb");

class FilePath;

using FilePathSet = std::vector<FilePath>;

struct FileCloser {
	// Called by unique_ptr to close the file
	void operator()(FILE *file) noexcept {
		if (file) {
			fclose(file);
		}
	}
};

using FileHolder = std::unique_ptr<FILE, FileCloser>;

class FilePath {
	GUI::gui_string fileName;
public:
	FilePath() noexcept;
	FilePath(const GUI::gui_char *fileName_);
	FilePath(GUI::gui_string_view fileName_);
	FilePath(GUI::gui_string fileName_) noexcept;
	FilePath(FilePath const &directory, FilePath const &name);
	FilePath(FilePath const &) = default;
	FilePath(FilePath &&) noexcept = default;
	FilePath &operator=(FilePath const &) = default;
	FilePath &operator=(FilePath &&) noexcept = default;
	virtual ~FilePath() = default;

	void Set(const GUI::gui_char *fileName_);
	void Set(FilePath const &other);
	void Set(FilePath const &directory, FilePath const &name);
	void SetDirectory(FilePath const &directory);
	virtual void Init() noexcept;
	[[nodiscard]] bool SameNameAs(const FilePath &other) const noexcept;
	bool operator==(const FilePath &other) const noexcept = default;
	std::weak_ordering operator<=>(const FilePath &other) const noexcept;
	[[nodiscard]] bool IsSet() const noexcept;
	[[nodiscard]] bool IsUntitled() const noexcept;
	[[nodiscard]] bool IsAbsolute() const noexcept;
	[[nodiscard]] bool IsRoot() const noexcept;
	[[nodiscard]] static size_t RootLength() noexcept;
	[[nodiscard]] const GUI::gui_char *AsInternal() const noexcept;
	[[nodiscard]] const GUI::gui_string &AsText() const noexcept;
	[[nodiscard]] std::string AsUTF8() const;
	[[nodiscard]] FilePath Name() const;
	[[nodiscard]] FilePath BaseName() const;
	[[nodiscard]] FilePath Extension() const;
	[[nodiscard]] FilePath Directory() const;
	void FixName();
	[[nodiscard]] FilePath AbsolutePath() const;
	[[nodiscard]] FilePath NormalizePath() const;
	[[nodiscard]] GUI::gui_string RelativePathTo(const FilePath &filePath) const;
	[[nodiscard]] static FilePath GetWorkingDirectory();
	bool SetWorkingDirectory() const noexcept;
	[[nodiscard]] static FilePath UserHomeDirectory();
	void List(FilePathSet &directories, FilePathSet &files) const;
	[[nodiscard]] FILE *Open(const GUI::gui_char *mode) const noexcept;
	[[nodiscard]] std::string Read() const;
	void Remove() const noexcept;
	[[nodiscard]] time_t ModifiedTime() const noexcept;
	[[nodiscard]] long long GetFileLength() const noexcept;
	[[nodiscard]] bool Exists() const noexcept;
	[[nodiscard]] bool IsDirectory() const noexcept;
	[[nodiscard]] bool Matches(GUI::gui_string_view pattern) const;
	[[nodiscard]] static bool CaseSensitive() noexcept;
};

std::optional<FilePath> FindPath(GUI::gui_string_view path, const FilePath &dir);

std::string CommandExecute(const GUI::gui_char *command, const GUI::gui_char *directoryForRun);

#endif
