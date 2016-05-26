#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib,"wininet")

#include <windows.h>
#include <wininet.h>
#include <string>
#include "picojson.h"
#include "resource.h"

TCHAR szClassName[] = TEXT("Window");

LPBYTE DownloadToMemory(IN LPCTSTR lpszURL, OUT PDWORD_PTR lpSize)
{
	LPBYTE lpszReturn = 0;
	*lpSize = 0;
	const HINTERNET hSession = InternetOpen(TEXT("GetGitHubRepositoryList"), INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, INTERNET_FLAG_NO_COOKIES);
	if (hSession)
	{
		URL_COMPONENTS uc = { 0 };
		TCHAR HostName[MAX_PATH];
		TCHAR UrlPath[MAX_PATH];
		uc.dwStructSize = sizeof(uc);
		uc.lpszHostName = HostName;
		uc.lpszUrlPath = UrlPath;
		uc.dwHostNameLength = MAX_PATH;
		uc.dwUrlPathLength = MAX_PATH;
		InternetCrackUrl(lpszURL, 0, 0, &uc);
		const HINTERNET hConnection = InternetConnect(hSession, HostName, INTERNET_DEFAULT_HTTPS_PORT, 0, 0, INTERNET_SERVICE_HTTP, 0, 0);
		if (hConnection)
		{
			const HINTERNET hRequest = HttpOpenRequest(hConnection, TEXT("GET"), UrlPath, 0, 0, 0, INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 0);
			if (hRequest)
			{
				HttpSendRequest(hRequest, 0, 0, 0, 0);
				lpszReturn = (LPBYTE)GlobalAlloc(GMEM_FIXED, 1);
				DWORD dwRead;
				static BYTE szBuf[1024 * 4];
				LPBYTE lpTmp;
				for (;;)
				{
					if (!InternetReadFile(hRequest, szBuf, (DWORD)sizeof(szBuf), &dwRead) || !dwRead) break;
					lpTmp = (LPBYTE)GlobalReAlloc(lpszReturn, (SIZE_T)(*lpSize + dwRead), GMEM_MOVEABLE);
					if (lpTmp == NULL) break;
					lpszReturn = lpTmp;
					CopyMemory(lpszReturn + *lpSize, szBuf, dwRead);
					*lpSize += dwRead;
				}
				InternetCloseHandle(hRequest);
			}
			InternetCloseHandle(hConnection);
		}
		InternetCloseHandle(hSession);
	}
	return lpszReturn;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hList;
	static HWND hButton;
	switch (msg)
	{
	case WM_CREATE:
		hButton = CreateWindow(TEXT("BUTTON"), TEXT("取得"), WS_VISIBLE | WS_CHILD | WS_DISABLED, 0, 0, 0, 0, hWnd, (HMENU)IDOK, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hList = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("LISTBOX"), 0, WS_VISIBLE | WS_CHILD | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_HASSTRINGS | LBS_NOTIFY, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		PostMessage(hWnd, WM_COMMAND, IDOK, 0);
		break;
	case WM_SIZE:
		MoveWindow(hButton, 10, 10, 256, 32, TRUE);
		MoveWindow(hList, 10, 50, LOWORD(lParam) - 20, HIWORD(lParam) - 60, TRUE);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			EnableWindow(hButton, FALSE);
			DWORD_PTR dwSize;
			LPBYTE lpByte = DownloadToMemory(TEXT("https://api.github.com/users/kenjinote/repos"), &dwSize);
			{
				picojson::value v;
				std::string err;
				picojson::parse(v, lpByte, lpByte + dwSize, &err);
				if (err.empty())
				{
					picojson::array arr = v.get<picojson::array>();
					for (auto v : arr)
					{
						picojson::object obj = v.get<picojson::object>();
						const INT_PTR nIndex = SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)(obj["name"].to_str().c_str()));
						std::string strCloneUrl = obj["clone_url"].to_str();
						LPSTR lpszCloneUrl = (LPSTR)GlobalAlloc(0, strCloneUrl.size() + 1);
						lstrcpyA(lpszCloneUrl, strCloneUrl.c_str());
						SendMessage(hList, LB_SETITEMDATA, nIndex, (LPARAM)lpszCloneUrl);
					}
				}
			}
			GlobalFree(lpByte);
			EnableWindow(hButton, TRUE);
		}
		else if ((HWND)lParam == hList && HIWORD(wParam) == LBN_SELCHANGE)
		{
			const INT_PTR nIndex = SendMessage(hList, LB_GETCURSEL, 0, 0);
			if (nIndex != LB_ERR)
			{
				LPSTR lpData = (LPSTR)SendMessage(hList, LB_GETITEMDATA, nIndex, 0);
				HGLOBAL hMem = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, lstrlenA(lpData) + 1);
				LPSTR lpszBuf = (LPSTR)GlobalLock(hMem);
				lstrcpyA(lpszBuf, lpData);
				GlobalUnlock(hMem);
				OpenClipboard(0);
				EmptyClipboard();
				SetClipboardData(CF_TEXT, hMem);
				CloseClipboard();
			}
		}
		break;
	case WM_DESTROY:
		{
			const INT_PTR nSize = SendMessage(hList, LB_GETCOUNT, 0, 0);
			for (int i = 0; i < nSize; ++i)
			{
				LPSTR lpData = (LPSTR)SendMessage(hList, LB_GETITEMDATA, i, 0);
				GlobalFree(lpData);
			}
		}
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		LoadIcon(hInstance,(LPCTSTR)IDI_ICON1),
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("GitHubからリポジトリー一覧を取得"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
