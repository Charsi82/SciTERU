#pragma once

// dialogs
bool run_color_dlg(HWND win, COLORREF& cl);
bool run_open_file_dialog(HWND win, wchar_t* result, const std::wstring& caption, std::wstring filter, bool multi = false);
bool run_selelect_dir_dialog(HWND win, wchar_t* result, const wchar_t* descr, const wchar_t* initial_dir);

// fs
HRESULT CreateShellLink(LPCWSTR pszShortcutFile, LPCWSTR pszLink, LPCWSTR pszWorkingDir, LPCWSTR pszDesc);
std::wstring GetKnownFolder(int folder_id);

DWORD shell_execute(const wchar_t* _command, const wchar_t* _args = nullptr, const wchar_t* _curDir = nullptr);

// html codes
std::string getAscii(unsigned char value);
std::string getHtmlName(unsigned char value);
int getHtmlNumber(unsigned char value);

class THasIconWin
{
	bool _has_image = false;
	virtual void set_image_list(bool small_size, HIMAGELIST hImageList) = 0;

public:
	int load_icons(const wchar_t* path, bool small_size);

protected:
	bool has_image() const { return _has_image; }
};
