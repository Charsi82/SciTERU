#pragma once
#include <Windows.h>
#include <commctrl.h>
#include <string>

// dialogs
bool run_colordlg(HWND win, COLORREF& cl);
bool run_ofd(HWND win, wchar_t* result, const std::wstring& caption, std::wstring filter, bool multi = false);
bool run_seldirdlg(HWND win, wchar_t* result, const wchar_t* descr, const wchar_t* initial_dir);

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
	bool m_small_icons{};
	HIMAGELIST m_handle{};

public:
	explicit TImageList(bool s = true);
	~TImageList();
	HIMAGELIST handle() const { return m_handle; }
	int add(const wchar_t* bitmapfile, COLORREF mask_clr = -1);
	int load_icons_from_module(const wchar_t* mod);
	void set_back_colour(COLORREF clrRef) const;
	void create();
	void destroy();
};

class THasIconWin
{
	TImageList iList{};

public:
	int load_icons(const wchar_t* path, bool small_size);

protected:
	HIMAGELIST get_image_list() const { return iList.handle(); }
	bool has_image() const { return iList.handle() != NULL; }

private:
	virtual void set_image_list(bool small_size) = 0;
};
