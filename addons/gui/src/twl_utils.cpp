#include "twl_utils.h"
#include "log.h"
#include <format>

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

int THasIconWin::load_icons(const wchar_t* path, bool bSmallIcon)
{
	TImageList iList;
	const int icons_loaded = iList.load_icons_from_module(path);
	if (icons_loaded) set_image_list(bSmallIcon, iList.handle());
	if (icons_loaded) _has_image = true;
	return icons_loaded;
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
TImageList::TImageList(bool s) : m_small_icons(s), m_handle(NULL)
{
	log_add("TImageList::TImageList");
}

int TImageList::add(const wchar_t* bitmapfile, COLORREF mask_clr)
{
	create();
	int res = -1;
	if (HBITMAP hBitmap = load_bitmap(bitmapfile))
	{
		if (mask_clr != -1)
			res = ImageList_AddMasked(m_handle, hBitmap, mask_clr);
		else
			res = ImageList_Add(m_handle, hBitmap, NULL);
		DeleteObject(hBitmap);
	}
	return res;
}

TImageList::~TImageList()
{
	log_add("TImageList::~TImageList");
	//destroy();
}

void TImageList::create()
{
	destroy();
	m_handle = ImageList_Create(m_small_icons ? 16 : 32, m_small_icons ? 16 : 32, ILC_COLOR32 | ILC_MASK, 0, 32);
}

void TImageList::destroy()
{
	if (m_handle)
	{
		ImageList_Destroy(m_handle);
		m_handle = NULL;
	}
}

int TImageList::load_icons_from_module(const wchar_t* mod)
{
	create();
	const int icon_cnt = ExtractIconEx(mod, -1, NULL, NULL, 1);
	for (int i = 0; i < icon_cnt; ++i)
		if (HICON hIcon = load_icon(mod, i, m_small_icons))
		{
			ImageList_AddIcon(m_handle, hIcon);
			DeleteObject(hIcon);
		}
	set_back_colour(CLR_NONE);
	return icon_cnt;
}

void TImageList::set_back_colour(COLORREF clrRef) const
{
	if (m_handle) ImageList_SetBkColor(m_handle, clrRef);
}
