#include "windows.h"
#include "Resource.h"
#include "commctrl.h"
#include "Shlwapi.h"
#include "tchar.h"

#define RUN_ON_LOGON			L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define TIMEZONE_WINDOW_NAME	_T("93D57D48-D027-4431-94FF-74AA6969486F")
#define WM_TRAY_ICON			WM_APP 

#define SHELL_TRAY_WND			L"Shell_TrayWnd"

#define IDC_BTN_SINGAPORE		102
#define IDC_BTN_IST				103
#define IDC_BTN_PST				104
#define IDC_BTN_CST				105
#define IDC_BTN_MST				106
#define IDC_BTN_EST				107

#define SINGAPORE_TZ_ID			L"Singapore Standard Time"
#define INDIA_TZ_ID				L"India Standard Time" 
#define CENTRAL_TZ_ID			L"Central Standard Time"
#define EASTERN_TZ_ID			L"Eastern Standard Time"
#define MOUNTAIN_TZ_ID			L"Mountain Standard Time"
#define PACEFIC_TZ_ID			L"Pacific Standard Time"

#define SET_SINGAPORE_TZ		L"Set Singapore Standard Time"
#define SET_INDIA_TZ			L"Set India Standard Time" 
#define SET_CENTRAL_TZ			L"Set Central Standard Time"
#define SET_EASTERN_TZ			L"Set Eastern Standard Time"
#define SET_MOUNTAIN_TZ			L"Set Mountain Standard Time"
#define SET_PACEFIC_TZ			L"Set Pacific Standard Time"

#define TIME_ZONE				L"TimeZone"
#define WM_EXIT					(WM_USER + 100)
#define WM_ONREGIESTER			(WM_USER + 101)

HINSTANCE  g_hInst;
WCHAR g_szTZUtilPath[MAX_PATH] = {0};
BOOL g_bAddToStartup = false;
WCHAR g_szAppPath[MAX_PATH] = {0};
int g_iColorStatus = 0;

LRESULT _stdcall WndProc(HWND, UINT, WPARAM, LPARAM);
//Set the key in the registry
BOOL SetRegKey(LPCWSTR i_wstrSubKeyName, LPCWSTR i_wstrValueName, DWORD i_dwType, LPCWSTR i_wstrPathName);

//Remove the Value from the regiestry
BOOL UnSetRegKey(LPCWSTR i_wstrSubKeyName, LPCWSTR i_wstrValueName);

//Set the Color to Button
BOOL SetColor(LPARAM i_lParam, LPCWSTR i_wstrButtonName, BOOL i_bIsSelected);

//Checks whether the Application is already running
BOOL IsAppAlreadyRunning();

//Add the Icon to system Tray
void AddTrayIcon( HINSTANCE i_hInstance, DWORD dwMessage, HWND hwnd);

//Set the position for window
BOOL SetWndPosition(HWND i_hWnd);

//show the menu items.
BOOL ShowTrayContextMenu(HWND i_hWnd);

//Create the tool tip
HWND CreateToolTip(int i_iBtnId, HWND i_hwnd, PTSTR pszToolTipText);

//Create the buttons
BOOL CreateButtonWindow(HWND i_hwnd, LPCWSTR i_wstrButtonName, int i_iXcord, int i_iYcord, int i_iXend, int i_iYend, int i_iBtnId);

//Opens the application
BOOL ExecuteApp(LPCWSTR i_lpAppPath, LPWSTR i_lpCmdLine);

//Changes the time zone
BOOL ChangeTimeZone(LPCWSTR i_lpcTimeZoneID);

//Controls and Set the time zone 
BOOL HandleChangeZoneRequest(HWND hwnd, int i_iButtonID);

//Draws the Custom Button
BOOL DrawCustomButton(LPCWSTR i_pszButtonName, LPARAM lParam, int i_iBtnId);

BOOL SetRegKey(LPCWSTR i_wstrSubKeyName, LPCWSTR i_wstrValueName, DWORD i_dwType, LPCWSTR i_wstrPathName)
{
	if(!i_wstrSubKeyName || !i_wstrValueName || !i_wstrPathName)
		return FALSE;

	HKEY hKey = NULL;
	DWORD RetCode = 0;
	DWORD dwsize = 2*wcslen(i_wstrPathName);

	RetCode = RegCreateKeyEx(HKEY_CURRENT_USER,
		           i_wstrSubKeyName,
				   0,
				   NULL,
				   REG_OPTION_NON_VOLATILE,
				   KEY_WRITE,
				   NULL,
				   &hKey,
				   NULL);

	if (RetCode == ERROR_SUCCESS)
	{
		RetCode = RegSetValueEx(hKey, i_wstrValueName, 0, i_dwType, (const byte *)i_wstrPathName, dwsize);
		if(RetCode != ERROR_SUCCESS )
		{
			RegCloseKey(hKey);
			return FALSE;
		}
		RegCloseKey(hKey);
	}
	return TRUE;
}

BOOL UnSetRegKey(LPCWSTR i_wstrSubKeyName, LPCWSTR i_wstrValueName)
{	
	if(!i_wstrSubKeyName || !i_wstrValueName)
		return FALSE;

	DWORD RetCode = 0;	

	RetCode = RegDeleteKeyValue(HKEY_CURRENT_USER, i_wstrSubKeyName, i_wstrValueName);
	if(RetCode != ERROR_SUCCESS)
		return FALSE;

	return TRUE;
}

BOOL SetColor(LPARAM i_lParam, LPCWSTR i_wstrButtonName, BOOL i_bIsSelected)
{
	if (!i_wstrButtonName)
		return FALSE;

	LPDRAWITEMSTRUCT lpdis = (DRAWITEMSTRUCT*)i_lParam;					
	SIZE size = {0};
	WCHAR szButtonName[MAX_PATH] = {0};
	
	if(i_bIsSelected)
	{
		SetTextColor(lpdis->hDC, RGB(255,0,0));
		wsprintf(szButtonName, L"%s", i_wstrButtonName);
		GetTextExtentPoint32(lpdis->hDC, szButtonName, wcslen(szButtonName), &size);
	}
	else
	{
		SetTextColor(lpdis->hDC, RGB(0, 0, 0));
		wsprintf(szButtonName, L"%s", i_wstrButtonName);
		GetTextExtentPoint32(lpdis->hDC, szButtonName, wcslen(szButtonName), &size);
	}

    ExtTextOut(lpdis->hDC,
    ((lpdis->rcItem.right - lpdis->rcItem.left) - size.cx) / 2,
    ((lpdis->rcItem.bottom - lpdis->rcItem.top) - size.cy) / 2,
	ETO_OPAQUE | ETO_CLIPPED, &lpdis->rcItem, szButtonName, wcslen(szButtonName), NULL);

    DrawEdge(lpdis->hDC, &lpdis->rcItem,
    (lpdis->itemState & ODS_SELECTED ?
    EDGE_SUNKEN : EDGE_RAISED ), BF_RECT);

	return TRUE;
}

BOOL IsAppAlreadyRunning()
{
	BOOL bRetVal = FALSE;
	HANDLE hAppwnd = FindWindow(NULL, TIMEZONE_WINDOW_NAME);

	if(hAppwnd != NULL)		
	{
		CloseHandle( hAppwnd );
		bRetVal = TRUE;
	}
	
	return bRetVal;
}
void AddTrayIcon(HINSTANCE i_hInstance, DWORD dwMessage, HWND i_hwnd)
{
	WCHAR szTip[MAX_PATH]= L"Tip";
	NOTIFYICONDATA notifyIconData = {0};

	wcscpy_s(notifyIconData.szTip, 20 , L"Change Time Zone");
	notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
	notifyIconData.szTip[19]= L'\0';
	notifyIconData.hWnd = i_hwnd;
	notifyIconData.uID = 100;
	notifyIconData.hIcon = (HICON)LoadImage( i_hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	notifyIconData.uCallbackMessage = WM_TRAY_ICON;
	notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP  ;
	
	Shell_NotifyIcon(dwMessage, &notifyIconData);
}

BOOL SetWndPosition(HWND i_hWnd)
{	
	if(!i_hWnd)
		return FALSE;

	DWORD dwRetCode = 0;
	RECT  rectCurrWnd = {0};
	RECT  rectShellTrayWnd = {0};

	int xFull = GetSystemMetrics(SM_CXSCREEN);
	int yFull = GetSystemMetrics(SM_CYSCREEN);
	HWND hTrayWnd = FindWindow(SHELL_TRAY_WND, NULL);
	if(hTrayWnd == NULL)
		return FALSE;

	GetWindowRect(hTrayWnd, &rectShellTrayWnd);
	GetWindowRect(i_hWnd, &rectCurrWnd);
	
	yFull = yFull - (rectShellTrayWnd.bottom - rectShellTrayWnd.top);
	dwRetCode = SetWindowPos(i_hWnd, HWND_TOP, (xFull - (rectCurrWnd.right - rectCurrWnd.left) - 10), (yFull - (rectCurrWnd.bottom - rectCurrWnd.top) - 3), 0, 0, SWP_NOSIZE);
	if(0 == dwRetCode)
		return FALSE;

	return TRUE;
}

BOOL ShowTrayContextMenu(HWND i_hWnd)
{
	POINT pt={0};
	DWORD dwAddToStartUp = 0;
	HMENU hMenu = 0 ;

	GetCursorPos(&pt);

	if(g_bAddToStartup)
		dwAddToStartUp = MF_CHECKED;
	else
		dwAddToStartUp = MF_UNCHECKED;

	hMenu = CreatePopupMenu();
	if(hMenu)
	{ 	
		InsertMenu(hMenu, -1, MF_BYPOSITION | MF_STRING , WM_EXIT, L"Exit");
		InsertMenu(hMenu, 0,  dwAddToStartUp | MF_BYPOSITION | MF_STRING , WM_ONREGIESTER, L"About"/*Add To Start Up*/);	
		SetForegroundWindow(i_hWnd);
		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, i_hWnd, NULL);
		PostMessage(i_hWnd, WM_NULL, 0, 0);
		DestroyMenu(hMenu);
	}
	return TRUE;
}

HWND CreateToolTip(int i_iBtnId, HWND i_hwnd, PTSTR pszToolTipText)
{
    if (!i_iBtnId || !i_hwnd || !pszToolTipText)
    {
        return FALSE;
    }
    HWND hwndBtn = GetDlgItem(i_hwnd, i_iBtnId);
    
    HWND hwndTip = CreateWindowEx(NULL, TOOLTIPS_CLASS, NULL,
                              WS_POPUP |TTS_ALWAYSTIP,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              i_hwnd, NULL, 
                              g_hInst, NULL);
    
   if (!hwndBtn || !hwndTip)
   {
       return (HWND)NULL;
   }                              
                              
	TOOLINFO toolInfo = {0};
    toolInfo.cbSize = sizeof(toolInfo);
    toolInfo.hwnd = i_hwnd;
    toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    toolInfo.uId = (UINT_PTR)hwndBtn;
    toolInfo.lpszText = pszToolTipText;
	
    SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);

    return hwndTip;
}

BOOL CreateButtonWindow(HWND i_hwnd,LPCWSTR i_wstrButtonName, int i_iXcord, int i_iYcord, int i_iXend, int i_iYend, int i_iBtnId)
{
	if(!i_wstrButtonName || !i_hwnd || !i_iBtnId)
		return FALSE;

	HWND hButton = CreateWindowEx(NULL, 
						L"BUTTON",
						i_wstrButtonName,
						WS_VISIBLE|WS_CHILD|BS_OWNERDRAW,
						i_iXcord, 
						i_iYcord,
						i_iXend, 
						i_iYend,
						i_hwnd,
						(HMENU)i_iBtnId,
						NULL,
						NULL);
	if(!hButton)
		return FALSE;

	return TRUE;
}

BOOL ExecuteApp(LPCWSTR i_lpAppPath, LPWSTR i_lpCmdLine)
{
	if(!i_lpAppPath && !i_lpCmdLine)
		return FALSE;
	
	BOOL bRetVal = FALSE;	 
	STARTUPINFO structStartupInfo = {0};
	PROCESS_INFORMATION structProcessInfo = {0} ;

	ZeroMemory(&structStartupInfo, sizeof(structStartupInfo));
    structStartupInfo.cb = sizeof(structStartupInfo);
    ZeroMemory(&structProcessInfo, sizeof(structProcessInfo));
	
	if(CreateProcess( i_lpAppPath, i_lpCmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &structStartupInfo, &structProcessInfo))
	{		
		bRetVal = TRUE;
	}		
	return bRetVal;
}

BOOL ChangeTimeZone(LPCWSTR i_lpcTimeZoneID)
{
	if(!i_lpcTimeZoneID)
		return FALSE;
	
	TCHAR szCommandLine[MAX_PATH] = {0};	
	
	wsprintf(szCommandLine, L"%ls /s \"%ls\"", g_szTZUtilPath, i_lpcTimeZoneID);
	return ExecuteApp(NULL, szCommandLine);
}

BOOL HandleChangeZoneRequest(HWND hwnd, int i_iButtonID)
{
	if(0 == i_iButtonID)
		return FALSE;
	if(!hwnd)
		return FALSE;

	WCHAR szCountryId[MAX_PATH] = {0};
	g_iColorStatus = i_iButtonID;
	
	switch(i_iButtonID)
	{
	case IDC_BTN_SINGAPORE:
		wcscpy_s(szCountryId, MAX_PATH, SINGAPORE_TZ_ID);
		break;
	
	case IDC_BTN_IST:
		wcscpy_s(szCountryId, MAX_PATH,INDIA_TZ_ID);
		break;
	
	case IDC_BTN_PST:
		wcscpy_s(szCountryId, MAX_PATH,PACEFIC_TZ_ID);
		break;
	
	case IDC_BTN_CST:
		wcscpy_s(szCountryId,MAX_PATH,CENTRAL_TZ_ID);
		break;
	
	case IDC_BTN_MST:
		wcscpy_s(szCountryId,MAX_PATH,MOUNTAIN_TZ_ID);
		break;

	case IDC_BTN_EST:
		wcscpy_s(szCountryId,MAX_PATH,EASTERN_TZ_ID);
		break;
	}

	ChangeTimeZone(szCountryId);
	if(g_iColorStatus)
	{	
		InvalidateRect(hwnd,NULL,TRUE);
		UpdateWindow(hwnd);					
	}	
	ShowWindow(hwnd, SW_HIDE);
	SetFocus(hwnd);

	return TRUE;
}

BOOL DrawCustomButton(LPCWSTR i_pszButtonName, LPARAM lParam, int i_iBottonId)
{
	if(!i_pszButtonName || !i_iBottonId)
		return FALSE;
	
	if(i_iBottonId == g_iColorStatus)
		SetColor(lParam, i_pszButtonName, TRUE);
	else
		SetColor(lParam, i_pszButtonName, FALSE);

	return TRUE;
}

int _stdcall WinMain(HINSTANCE hInstance, 
				   HINSTANCE hPrevInstance,
				   LPSTR szCmdLine, 
				   int iCmdShow) 
{	
	if(IsAppAlreadyRunning())
		return 1;

	GetSystemDirectory(g_szTZUtilPath, MAX_PATH); 
	wcscat_s(g_szTZUtilPath, L"\\tzutil.exe");
	if(!PathFileExists(g_szTZUtilPath))
	{
		MessageBox(NULL, L"Failed to locate \"TZUtil.exe\"", L"Error", MB_OK|MB_ICONERROR);
	}

    TCHAR szAppName[] = L"Time Zone";
    HWND        hwnd;
    MSG         msg;
    WNDCLASSEX  wndclass;
	DWORD ErrorCode;
	g_hInst = hInstance;

    wndclass.cbSize         = sizeof(wndclass);
	wndclass.style          = NULL ;
    wndclass.lpfnWndProc    = WndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = hInstance;
	wndclass.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
	wndclass.hIconSm        = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground	= (HBRUSH) CreateSolidBrush(RGB(94,190,227));
    wndclass.lpszClassName  = szAppName;
    wndclass.lpszMenuName   = NULL;
	
    if(!RegisterClassEx(&wndclass))
	{
		MessageBox(NULL,L"Register Class Fail",L"Error",MB_OK);
		ErrorCode = GetLastError();
		return 1;
	}

	hwnd = CreateWindowEx(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,
						szAppName, 
						TIMEZONE_WINDOW_NAME,
						WS_POPUPWINDOW,
						0,
						0,
						203,
						37,
						NULL,
						NULL, 
						hInstance,
						NULL);

	if(!hwnd)
	{
		MessageBox(NULL,L"Create Window Fail",L"Error",MB_OK);
		ErrorCode = GetLastError();
		return 1;
	}

	RegisterHotKey(hwnd,1,MOD_ALT|MOD_CONTROL,0x54);
	
	AddTrayIcon( hInstance ,NIM_ADD ,hwnd);
	
	ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
	
    while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);    
		DispatchMessage(&msg);    
    }
    return msg.wParam;
}

LRESULT _stdcall WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) 
{	
	UINT uHitTest = 0;
	switch ( iMsg ) 
	{		
	case WM_CREATE :
		{
			SetWndPosition(hwnd);
			CreateButtonWindow(hwnd, L"S", 4, 3, 30, 30, IDC_BTN_SINGAPORE);
			CreateToolTip(IDC_BTN_SINGAPORE, hwnd, SET_SINGAPORE_TZ);

			CreateButtonWindow(hwnd, L"I", 37, 3, 30, 30, IDC_BTN_IST);
			CreateToolTip(IDC_BTN_IST, hwnd, SET_INDIA_TZ);

			CreateButtonWindow(hwnd, L"P", 70, 3, 30, 30, IDC_BTN_PST);
			CreateToolTip(IDC_BTN_PST, hwnd, SET_PACEFIC_TZ);

			CreateButtonWindow(hwnd, L"C", 136, 3, 30, 30, IDC_BTN_CST);			
			CreateToolTip(IDC_BTN_CST, hwnd, SET_CENTRAL_TZ);

			CreateButtonWindow(hwnd,L"E", 169, 3, 30, 30, IDC_BTN_EST);
			CreateToolTip(IDC_BTN_EST, hwnd, SET_EASTERN_TZ);

			CreateButtonWindow(hwnd,L"M",103,3,30,30, IDC_BTN_MST);			
			CreateToolTip(IDC_BTN_MST, hwnd, SET_MOUNTAIN_TZ);

			GetModuleFileName(NULL, g_szAppPath, MAX_PATH);

			/*if(g_bAddToStartup)
			{	
				SetRegKey(RUN_ON_LOGON, TIME_ZONE, REG_SZ, g_szAppPath);
			}*/
			return 0;
		}
		break;

	case WM_DRAWITEM:
		{
            switch ((UINT)wParam)
            {
			case IDC_BTN_SINGAPORE:
				{   	
					DrawCustomButton(L"S", lParam, IDC_BTN_SINGAPORE);
					return TRUE;
                }				
			case IDC_BTN_IST:
                {
					DrawCustomButton(L"I",lParam, IDC_BTN_IST);
					return TRUE;    
                }
			case IDC_BTN_PST:
                {
					DrawCustomButton(L"P", lParam, IDC_BTN_PST);
					return TRUE;     
                }
			case IDC_BTN_MST:
                {
					DrawCustomButton(L"M", lParam, IDC_BTN_MST);
					return TRUE;	
                }
			case IDC_BTN_CST:
				{
					DrawCustomButton(L"C",lParam,IDC_BTN_CST);
					return TRUE;
				}
			case IDC_BTN_EST:
                {
					DrawCustomButton(L"E",lParam,IDC_BTN_EST);
					return TRUE;
				}
                break;
            }
        }
        break;

	case WM_TRAY_ICON:
		switch(lParam)
		{
		case WM_LBUTTONDOWN:
			SetForegroundWindow(hwnd);
			ShowWindow(hwnd, SW_SHOW);
			break;

		case WM_RBUTTONDOWN:

		case WM_CONTEXTMENU:
			ShowTrayContextMenu(hwnd);
			break;
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case WM_ONREGIESTER:
        {
            MessageBox(NULL, L"This Utility is developed by Mahadev Bichewar.\r\n For any query or suggestion please contact on below details.\r\nContact Number:9890930180\r\nEmail:mbichewar@gmail.com", L"The Great Indian Developers", MB_OK);
            /*g_bAddToStartup = !g_bAddToStartup;

            if (g_bAddToStartup)
            {
                if (!SetRegKey(RUN_ON_LOGON, TIME_ZONE, REG_SZ, g_szAppPath))
                {
                    MessageBox(NULL, L"Failed to add application to startup", L"Error", MB_OK | MB_ICONERROR);
                }
            }
            if (!g_bAddToStartup)
            {
                UnSetRegKey(RUN_ON_LOGON, TIME_ZONE);
            }*/
        }
			break;

		case WM_EXIT:
			UnregisterHotKey(hwnd, 1);
			PostQuitMessage(0);
			break;

		case IDC_BTN_SINGAPORE:
			HandleChangeZoneRequest(hwnd, IDC_BTN_SINGAPORE);
			break ;

		case IDC_BTN_IST:
			HandleChangeZoneRequest(hwnd, IDC_BTN_IST);
			break ;

		case IDC_BTN_PST:
			HandleChangeZoneRequest(hwnd, IDC_BTN_PST);
			break ;

		case IDC_BTN_CST:
			HandleChangeZoneRequest(hwnd, IDC_BTN_CST);
			break ;

		case IDC_BTN_EST:
			HandleChangeZoneRequest(hwnd, IDC_BTN_EST);
			break ;

		case IDC_BTN_MST:
			HandleChangeZoneRequest(hwnd, IDC_BTN_MST);
			break ;
		}
		break;

	case WM_KEYDOWN:
		if(wParam == VK_ESCAPE)
		{
			ShowWindow(hwnd, SW_HIDE);
			return 0;
		}
		else if(wParam == 0x53) //S key press
		{
			HandleChangeZoneRequest(hwnd, IDC_BTN_SINGAPORE);
			return 0;
		}
		else if(wParam == 0x49) //I key press
		{
			HandleChangeZoneRequest(hwnd, IDC_BTN_IST);
			return 0;
		}
		else if(wParam == 0x50) //P key press
		{
			HandleChangeZoneRequest(hwnd, IDC_BTN_PST);
			return 0;
		}
		else if(wParam == 0x43) //C key press
		{
			HandleChangeZoneRequest(hwnd, IDC_BTN_CST);
			return 0;
		}
		else if(wParam == 0x45) //E key press
		{
			HandleChangeZoneRequest(hwnd, IDC_BTN_EST);
			return 0;
		}
		else if(wParam == 0x4D) //M key press
		{
			HandleChangeZoneRequest(hwnd, IDC_BTN_MST);
			return 0;
		}
		break;

	case WM_NCHITTEST:
		uHitTest = DefWindowProc(hwnd, WM_NCHITTEST, wParam, lParam);
		if(uHitTest == HTCLIENT)
			return HTCAPTION;
		else
			return uHitTest;

	case WM_HOTKEY:
		ShowWindow(hwnd, SW_SHOW);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
    return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
