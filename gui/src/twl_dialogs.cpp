// twl_dialogs.cpp

#include "twl_utils.h"
#include <shlobj.h>
#include <algorithm>

bool run_ofd(HWND win, TCHAR* result, const std::wstring& caption, std::wstring filter, bool multi)
{
	//std::wstring filter(fltr);
	filter += L"||";
	std::replace(filter.begin(), filter.end(), L'|', L'\0');
	*result = 0;
	OPENFILENAME ofn{};
	ofn.hwndOwner = win;
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.lpstrTitle = caption.c_str();
	ofn.lpstrFilter = filter.c_str();
	ofn.nMaxFile = 1024;
	ofn.lpstrFile = result; // buffer for result
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	if (multi) ofn.Flags |= OFN_ALLOWMULTISELECT;
	return GetOpenFileName(&ofn);
}

bool run_colordlg(HWND win, COLORREF& cl)
{
	static COLORREF custom_colours[16]{};
	CHOOSECOLOR m_choose_color{};
	m_choose_color.lStructSize = sizeof(CHOOSECOLOR);
	m_choose_color.hwndOwner = win;
	m_choose_color.rgbResult = cl;
	m_choose_color.lpCustColors = custom_colours;
	m_choose_color.Flags = CC_RGBINIT | CC_FULLOPEN;
	if (!ChooseColor(&m_choose_color)) return false;
	cl = m_choose_color.rgbResult;
	return true;
}

bool run_seldirdlg(HWND win, TCHAR* result, const wchar_t* descr, const wchar_t* initial_dir)
{
	BROWSEINFO bi{};
	bi.hwndOwner = win;
	bi.lpszTitle = descr;
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	bi.lpfn = [](HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
	{
		if (uMsg == BFFM_INITIALIZED) SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
		return 0;
	};
	bi.lParam = (LPARAM)initial_dir;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
	bool state = false;
	if (pidl)
	{
		//get the name of the folder and put it in path
		state = SHGetPathFromIDList(pidl, result);

		//free memory used
		IMalloc* imalloc = 0;
		if (SUCCEEDED(SHGetMalloc(&imalloc)))
		{
			imalloc->Free(pidl);
			imalloc->Release();
		}
	}
	return state;
}

HRESULT CreateShellLink(LPCWSTR pszShortcutFile, LPCWSTR pszLink, LPCWSTR pszWorkingDir, LPCWSTR pszDesc)
{
	HRESULT hres{};
	IShellLink* psl{};

	// Get a pointer to the IShellLink interface.
	hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast<void**>(&psl));
	if (SUCCEEDED(hres))
	{
		IPersistFile* ppf;

		// Query IShellLink for the IPersistFile interface for 
		// saving the shell link in persistent storage.
		hres = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf));
		if (SUCCEEDED(hres))
		{
			// Set the path to the shell link target.
			hres = psl->SetPath(pszShortcutFile);

			if (!SUCCEEDED(hres))
				MessageBox(NULL, L"Set Path failed!", L"ERROR", MB_OK);

			// Set workig directory
			hres = psl->SetWorkingDirectory(pszWorkingDir);

			if (!SUCCEEDED(hres))
				MessageBox(NULL, L"Set WorkingDirectory failed!", L"ERROR", MB_OK);

			// Set the description of the shell link.
			hres = psl->SetDescription(pszDesc);

			if (!SUCCEEDED(hres))
				MessageBox(NULL, L"Set Description failed!", L"ERROR", MB_OK);

			// Save the link via the IPersistFile::Save method.
			hres = ppf->Save(pszLink, TRUE);

			// Release pointer to IPersistFile.
			ppf->Release();
		}
		// Release pointer to IShellLink.
		psl->Release();
	}
	return (hres);
}

std::wstring GetKnownFolder(int folder_id)
{
	LPITEMIDLIST pidl;
	std::wstring res(L"", MAX_PATH);

	HRESULT ret = SHGetSpecialFolderLocation(NULL, folder_id, &pidl);
	if (ret == S_OK)
	{
		SHGetPathFromIDList(pidl, res.data());
	}
	return res;
}
