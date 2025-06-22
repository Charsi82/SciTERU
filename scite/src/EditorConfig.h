// SciTE - Scintilla based Text Editor
/** @file EditorConfig.h
 ** Read and interpret settings files in the EditorConfig format.
 **/
// Copyright 2018 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef EDITORCONFIG_H
#define EDITORCONFIG_H

class FilePath;

using StringMap = std::map<std::string, std::string>;

class IEditorConfig {
public:
	virtual ~IEditorConfig() = default;
	virtual void ReadFromDirectory(const FilePath &dirStart) = 0;
	[[nodiscard]] virtual StringMap MapFromAbsolutePath(const FilePath &absolutePath) const = 0;
	virtual void Clear() = 0;
	[[nodiscard]] static std::unique_ptr<IEditorConfig> Create();
};

#endif
