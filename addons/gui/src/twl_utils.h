#pragma once
#include <Windows.h>
#include <commctrl.h>
#include "utf.h"

// dialogs
bool run_colordlg(HWND win, COLORREF& cl);
bool run_ofd(HWND win, TCHAR* result, const std::wstring& caption, std::wstring filter, bool multi = false);
bool run_seldirdlg(HWND win, TCHAR* result, const wchar_t* descr, const wchar_t* initial_dir);

// fs
HRESULT CreateShellLink(LPCWSTR pszShortcutFile, LPCWSTR pszLink, LPCWSTR pszWorkingDir, LPCWSTR pszDesc);
std::wstring GetKnownFolder(int folder_id);

//DWORD shell_execute_sync(const wchar_t* _command, const wchar_t* _args = nullptr, const wchar_t* _curDir = nullptr);
DWORD shell_execute(const wchar_t* _command, const wchar_t* _args = nullptr, const wchar_t* _curDir = nullptr);

// Loading icons from .ico and .dll
HICON load_icon(const wchar_t* file, int idx = 0, bool small_icon = true);

// Loading icons from .ico and .dll, bitmaps from .bmp files
HBITMAP load_bitmap(const wchar_t* file);

// html codes
std::string getAscii(unsigned char value);
std::string getHtmlName(unsigned char value);
int getHtmlNumber(unsigned char value);

// icons and images list
class TImageList
{
	bool m_small_icons;
	HIMAGELIST m_handle;

public:
	explicit TImageList(bool s = true);
	HIMAGELIST handle() const { return m_handle; }
	int add(const wchar_t* bitmapfile, COLORREF mask_clr = -1);
	int load_icons_from_module(const wchar_t* mod);
	void set_back_colour(COLORREF clrRef);
};

class THasIconWin
{
	HIMAGELIST hImgList;

public:
	THasIconWin() : hImgList(NULL) {}
	virtual ~THasIconWin() { destroy(); }
	HIMAGELIST get_image_list() const { return hImgList; }
	bool has_image() const { return hImgList != NULL; }
	int load_icons(const wchar_t* path, bool small_size);

private:
	virtual void set_image_list(bool normal = true) = 0;
	void destroy();
};
