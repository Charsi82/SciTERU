// SciTE - Scintilla based Text Editor
/** @file EditorConfig.cxx
 ** Read and interpret settings files in the EditorConfig format.
 ** http://editorconfig.org/
 **/
// Copyright 2018 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cassert>

#include <compare>
#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <memory>
#include <chrono>

#include "GUI.h"

#include "StringHelpers.h"
#include "FilePath.h"
#include "PathMatch.h"
#include "EditorConfig.h"

namespace {

struct ECForDirectory {
	bool isRoot = false;
	std::string directory;
	std::vector<std::string> lines;
	void ReadOneDirectory(const FilePath &dir);
};

class EditorConfig : public IEditorConfig {
	std::vector<ECForDirectory> config;
public:
	void ReadFromDirectory(const FilePath &dirStart) override;
	[[nodiscard]] StringMap MapFromAbsolutePath(const FilePath &absolutePath) const override;
	void Clear() noexcept override;
};

const GUI::gui_string_view editorConfigName = GUI_TEXT(".editorconfig");

// Install defaults for indentation/tab
void InstallDefaults(StringMap &ret) {
	// if indent_style == "tab" and !indent_size: indent_size = "tab"
	if (ret.contains("indent_style") && ret["indent_style"] == "tab" && !ret.contains("indent_size")) {
		ret["indent_size"] = "tab";
	}

	// if indent_size != "tab" and !tab_width: tab_width = indent_size
	if (ret.contains("indent_size") && ret["indent_size"] != "tab" && !ret.contains("tab_width")) {
		ret["tab_width"] = ret["indent_size"];
	}

	// if indent_size == "tab": indent_size = tab_width
	if (ret.contains("indent_size") && ret["indent_size"] == "tab" && ret.contains("tab_width")) {
		ret["indent_size"] = ret["tab_width"];
	}
}

}

void ECForDirectory::ReadOneDirectory(const FilePath &dir) {
	directory = dir.AsUTF8();
	directory.append("/");
	FilePath path(dir, editorConfigName);
	std::string configString = path.Read();
	if (!configString.empty()) {
		const std::string_view svUtf8BOM(UTF8BOM);
		if (configString.starts_with(svUtf8BOM)) {
			configString.erase(0, svUtf8BOM.length());
		}
		// Carriage returns aren't wanted
		Remove(configString, std::string("\r"));
		std::vector<std::string> configLines = StringSplit(configString, '\n');
		for (std::string &line : configLines) {
			Trim(line);
			if (line.empty() || line.starts_with("#") || line.starts_with(";")) {
				// Drop comments
			} else if (line.starts_with("[")) {
				// Pattern
				lines.push_back(line);
			} else if (Contains(line, '=')) {
				LowerCaseAZ(line);
				lines.push_back(line);
				std::vector<std::string> nameVal = StringSplit(line, '=');
				if (nameVal.size() == 2) {
					Trim(nameVal[0]);
					Trim(nameVal[1]);
					if ((nameVal[0] == "root") && nameVal[1] == "true") {
						isRoot = true;
					}
				}
			}
		}
	}
}

void EditorConfig::ReadFromDirectory(const FilePath &dirStart) {
	FilePath dir = dirStart;
	while (true) {
		ECForDirectory ecfd;
		ecfd.ReadOneDirectory(dir);
		config.insert(config.begin(), ecfd);
		if (ecfd.isRoot || !dir.IsSet() || dir.IsRoot()) {
			break;
		}
		// Up a level
		dir = dir.Directory();
	}
}

StringMap EditorConfig::MapFromAbsolutePath(const FilePath &absolutePath) const {
	StringMap ret;
	std::string fullPath = absolutePath.AsUTF8();
#if defined(_WIN32)
	// Convert Windows path separators to Unix
	std::ranges::replace(fullPath, '\\', '/');
#endif
	for (const ECForDirectory &level : config) {
		std::string relPath;
		if (level.directory.length() <= fullPath.length()) {
			relPath = fullPath.substr(level.directory.length());
		}
		bool inActiveSection = false;
		for (const std::string &line : level.lines) {
			if (line.starts_with("[")) {
				const std::string pattern = line.substr(1, line.size() - 2);
				inActiveSection = PathMatch(pattern, relPath);
				// PatternMatch only works with literal filenames, '?', '*', '**', '[]', '[!]', '{,}', '{..}', '\x'.
			} else if (inActiveSection && Contains(line, '=')) {
				std::vector<std::string> nameVal = StringSplit(line, '=');
				if (nameVal.size() == 2) {
					Trim(nameVal[0]);
					Trim(nameVal[1]);
					if (nameVal[1] == "unset") {
						ret.erase(nameVal[0]);
					} else {
						ret[nameVal[0]] = nameVal[1];
					}
				}
			}
		}
	}

	InstallDefaults(ret);

	return ret;
}

void EditorConfig::Clear() noexcept {
	config.clear();
}

std::unique_ptr<IEditorConfig> IEditorConfig::Create() {
	return std::make_unique<EditorConfig>();
}
