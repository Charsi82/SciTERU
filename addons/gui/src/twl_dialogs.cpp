// twl_dialogs.cpp

#include <shlobj.h>
#include <algorithm>
#include <string>

#include "twl_utils.hpp"

bool run_open_file_dialog(HWND win, wchar_t* result, const std::wstring& caption, std::wstring filter, bool multi)
{
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

bool run_color_dlg(HWND win, COLORREF& cl)
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

bool run_selelect_dir_dialog(HWND win, wchar_t* result, const wchar_t* descr, const wchar_t* initial_dir)
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
	IShellLink* psl{};

	// Get a pointer to the IShellLink interface.
	HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast<void**>(&psl));
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
	return hres;
}

std::wstring GetKnownFolder(int folder_id)
{
	LPITEMIDLIST pidl;
	std::wstring res{};
	if (SHGetSpecialFolderLocation(NULL, folder_id, &pidl) == S_OK)
	{
		res.resize(MAX_PATH);
		SHGetPathFromIDList(pidl, res.data());
	}
	return res;
}

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
