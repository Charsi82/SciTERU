#include "twl_utils.h"
#include "log.h"

/*
DWORD shell_execute_sync(const wchar_t* _command, const wchar_t* _args = nullptr, const wchar_t* _curDir = nullptr)
{
	SHELLEXECUTEINFO ShExecInfo{};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = L"open";
	ShExecInfo.lpFile = _command;
	ShExecInfo.lpParameters = _args;
	ShExecInfo.lpDirectory = _curDir;
	ShExecInfo.nShow = SW_SHOWDEFAULT;
	ShExecInfo.hInstApp = NULL;

	ShellExecuteEx(&ShExecInfo);
	if (!ShExecInfo.hProcess)
	{
		// throw exception
		//throw GetLastErrorAsString(GetLastError());
		return GetLastError();
	}

	WaitForSingleObject(ShExecInfo.hProcess, INFINITE);

	DWORD exitCode = 0;
	if (::GetExitCodeProcess(ShExecInfo.hProcess, &exitCode) == FALSE)
	{
		// throw exception
		//throw GetLastErrorAsString(GetLastError());
		return GetLastError();
	}

	return exitCode;
}
*/

DWORD shell_execute(const wchar_t* _command, const wchar_t* _args, const wchar_t* _curDir)
{
	SHELLEXECUTEINFO ShExecInfo{};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = L"open";
	ShExecInfo.lpFile = _command;
	ShExecInfo.lpParameters = _args;
	ShExecInfo.lpDirectory = _curDir;
	ShExecInfo.nShow = SW_SHOWDEFAULT;
	ShExecInfo.hInstApp = NULL;

	ShellExecuteEx(&ShExecInfo);
	if (!ShExecInfo.hProcess)
	{
		// throw exception
		//throw GetLastErrorAsString(GetLastError());
		return GetLastError();
	}

	WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
	CloseHandle(ShExecInfo.hProcess);
	return 0;
}

int THasIconWin::load_icons(const wchar_t* path, bool small_size)
{
	destroy();
	TImageList il(small_size);
	int icons_loaded = il.load_icons_from_module(path);
	hImgList = il.handle();
	set_image_list();
	return icons_loaded;
}

void THasIconWin::destroy()
{
	if (hImgList) ImageList_Destroy(hImgList);
	hImgList = NULL;
}

HICON load_icon(const wchar_t* file, int idx, bool small_icon)
{
	HICON hIcon = NULL;
	ExtractIconEx(file, idx, small_icon ? NULL : (&hIcon), small_icon ? (&hIcon) : NULL, 1);
	return hIcon;
}

HBITMAP load_bitmap(const wchar_t* file)
{
	return (HBITMAP)LoadImage(NULL, file, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_LOADTRANSPARENT);
}

////////////////////////////
// TImageList class
TImageList::TImageList(bool s) : m_small_icons(s), m_handle(ImageList_Create(s ? 16 : 32, s ? 16 : 32, ILC_COLOR32 | ILC_MASK, 0, 32))
{}

int TImageList::add(const wchar_t* bitmapfile, COLORREF mask_clr)
{
	int res = -1;
	HBITMAP hBitmap = load_bitmap(bitmapfile);
	if (hBitmap)
	{
		if (mask_clr != -1)
			res = ImageList_AddMasked(m_handle, hBitmap, mask_clr);
		else
			res = ImageList_Add(m_handle, hBitmap, NULL);
		DeleteObject(hBitmap);
	}
	return res;
}

int TImageList::load_icons_from_module(const wchar_t* mod)
{
	int icon_cnt = ExtractIconEx(mod, -1, NULL, NULL, 1);
	for (int i = 0; i < icon_cnt; ++i)
		if (HICON hIcon = load_icon(mod, i, m_small_icons))
		{
			ImageList_AddIcon(m_handle, hIcon);
			DeleteObject(hIcon);
		}
	set_back_colour(CLR_NONE);
	return icon_cnt;
}

void TImageList::set_back_colour(COLORREF clrRef)
{
	ImageList_SetBkColor(m_handle, clrRef);
}
