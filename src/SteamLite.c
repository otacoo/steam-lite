#include <windows.h>
#include <wtsapi32.h>
#include <winternl.h>
#include <shlobj.h>
#include <objbase.h>

#define STEAMLITE_VERSION L"v1.0.1"
#define INI_SECTION   L"SteamLite"
#define IDC_SAVE      1
#define IDC_QUIT      2
#define IDC_FRIENDS_SIGNIN    100
#define IDC_LAUNCH_MINIMIZED  101
#define IDC_NO_JOYSTICK       102
#define IDC_NO_SHADERS        103
#define IDC_NO_GPU            104
#define IDC_ANIMATED_AVATARS  105
#define IDC_SHOW_GAME_ICONS   106
#define IDC_CEF_AUTOMATIC     107
#define IDC_DONT_SHOW_LAUNCH  108
#define IDC_DONT_SHOW_TRAY    109
#define IDC_ENABLE_CONSOLE   110
#define WM_STEAMLITE_REMOVE_TRAY  (WM_APP + 0)

static HINSTANCE g_hModule;
static DWORD WINAPI ThreadProc(LPVOID lpParameter);
static void ShowOptionsDialog(HWND hParent);

static unsigned int strlen_w(const WCHAR *s)
{
    unsigned int n = 0;
    while (s[n]) n++;
    return n;
}
static WCHAR *strcpy_w(WCHAR *dst, const WCHAR *src)
{
    WCHAR *r = dst;
    while ((*dst++ = *src++) != 0);
    return r;
}
static WCHAR *strcat_w(WCHAR *dst, const WCHAR *src)
{
    strcpy_w(dst + strlen_w(dst), src);
    return dst;
}

static void get_ini_path(WCHAR *path)
{
    if (!GetModuleFileNameW(g_hModule, path, MAX_PATH)) { path[0] = 0; return; }
    unsigned int i = strlen_w(path);
    while (i && path[i - 1] != L'\\') i--;
    path[i] = 0;
    strcat_w(path, L"SteamLite.ini");
}
static DWORD LoadOptionIni(const WCHAR *path, const WCHAR *key, DWORD def)
{
    return (DWORD)GetPrivateProfileIntW(INI_SECTION, key, (INT)def, path);
}
static void SaveOptionIni(const WCHAR *path, const WCHAR *key, DWORD val)
{
    WCHAR buf[4];
    buf[0] = (WCHAR)(L'0' + (val % 10));
    buf[1] = 0;
    WritePrivateProfileStringW(INI_SECTION, key, buf, path);
}

static const IID IID_IShellLinkW_local = {0x000214F9, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
static const IID IID_IPersistFile_local = {0x0000010b, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
static const CLSID CLSID_ShellLink_local = {0x00021401, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

static void CreateSteamShortcut(void)
{
    WCHAR steamPath[MAX_PATH] = {}, args[512] = L"";
    DWORD cb = sizeof(steamPath);
    if (RegGetValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Valve\\Steam", L"SteamPath", RRF_RT_REG_SZ, NULL, steamPath,
                     &cb) != ERROR_SUCCESS || !steamPath[0])
        return;
    if (strlen_w(steamPath) + 12 >= MAX_PATH) return;
    strcat_w(steamPath, L"\\steam.exe");

    WCHAR iniPath[MAX_PATH];
    get_ini_path(iniPath);
    DWORD launchMin = LoadOptionIni(iniPath, L"LaunchMinimized", 1);
    DWORD noJoy = LoadOptionIni(iniPath, L"NoJoystick", 0);
    DWORD noShaders = LoadOptionIni(iniPath, L"NoShaders", 1);
    DWORD noGpu = LoadOptionIni(iniPath, L"NoGPU", 1);
    DWORD console = LoadOptionIni(iniPath, L"Console", 1);
    strcpy_w(args, launchMin ? L"-silent " : L"");
    strcat_w(args, L"-quicklogin -forceservice -vrdisable -oldtraymenu -nofriendsui -no-dwrite ");
    if (noJoy) strcat_w(args, L"-nojoy ");
    if (noShaders) strcat_w(args, L"-noshaders ");
    if (noGpu) strcat_w(args, L"-nodirectcomp -cef-disable-gpu -cef-disable-gpu-sandbox ");
    strcat_w(args, L"-cef-force-occlusion -cef-disable-hang-timeouts");
    if (console) strcat_w(args, L" -console");

    WCHAR shortcutPath[MAX_PATH] = {};
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, 0, shortcutPath)))
        return;
    if (strlen_w(shortcutPath) + 20 >= MAX_PATH) return;
    strcat_w(shortcutPath, L"\\Steam Lite.lnk");

    if (FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)))
        return;
    IShellLinkW *pSl = NULL;
    if (FAILED(CoCreateInstance((CLSID *)&CLSID_ShellLink_local, NULL, CLSCTX_INPROC_SERVER, (IID *)&IID_IShellLinkW_local, (void **)&pSl)))
    {
        CoUninitialize();
        return;
    }
    pSl->lpVtbl->SetPath(pSl, steamPath);
    pSl->lpVtbl->SetArguments(pSl, args);
    pSl->lpVtbl->SetDescription(pSl, L"Steam with Steam Lite options");
    IPersistFile *pPf = NULL;
    if (SUCCEEDED(pSl->lpVtbl->QueryInterface(pSl, (IID *)&IID_IPersistFile_local, (void **)&pPf)))
    {
        pPf->lpVtbl->Save(pPf, shortcutPath, TRUE);
        pPf->lpVtbl->Release(pPf);
    }
    pSl->lpVtbl->Release(pSl);
    CoUninitialize();
}

static LRESULT CALLBACK OptionsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_SAVE)
        {
            WCHAR iniPath[MAX_PATH];
            get_ini_path(iniPath);
            DWORD v = (SendMessageW(GetDlgItem(hWnd, IDC_FRIENDS_SIGNIN), BM_GETCHECK, 0, 0) == BST_CHECKED);
            SaveOptionIni(iniPath, L"FriendsSignIn", v);
            DWORD launchMin = (SendMessageW(GetDlgItem(hWnd, IDC_LAUNCH_MINIMIZED), BM_GETCHECK, 0, 0) == BST_CHECKED);
            SaveOptionIni(iniPath, L"LaunchMinimized", launchMin);
            DWORD noJoy = (SendMessageW(GetDlgItem(hWnd, IDC_NO_JOYSTICK), BM_GETCHECK, 0, 0) == BST_CHECKED);
            SaveOptionIni(iniPath, L"NoJoystick", noJoy);
            DWORD noShaders = (SendMessageW(GetDlgItem(hWnd, IDC_NO_SHADERS), BM_GETCHECK, 0, 0) == BST_CHECKED);
            SaveOptionIni(iniPath, L"NoShaders", noShaders);
            DWORD noGpu = (SendMessageW(GetDlgItem(hWnd, IDC_NO_GPU), BM_GETCHECK, 0, 0) == BST_CHECKED);
            SaveOptionIni(iniPath, L"NoGPU", noGpu);
            DWORD console = (SendMessageW(GetDlgItem(hWnd, IDC_ENABLE_CONSOLE), BM_GETCHECK, 0, 0) == BST_CHECKED);
            SaveOptionIni(iniPath, L"Console", console);
            v = (SendMessageW(GetDlgItem(hWnd, IDC_ANIMATED_AVATARS), BM_GETCHECK, 0, 0) == BST_CHECKED);
            SaveOptionIni(iniPath, L"FriendsAnimed", v);
            v = (SendMessageW(GetDlgItem(hWnd, IDC_SHOW_GAME_ICONS), BM_GETCHECK, 0, 0) == BST_CHECKED);
            SaveOptionIni(iniPath, L"ShowGameIcons", v);
            v = (SendMessageW(GetDlgItem(hWnd, IDC_CEF_AUTOMATIC), BM_GETCHECK, 0, 0) == BST_CHECKED);
            SaveOptionIni(iniPath, L"CEFAutomatic", v);
            SaveOptionIni(iniPath, L"Mode", v ? 3u : 1u);
            v = (SendMessageW(GetDlgItem(hWnd, IDC_DONT_SHOW_LAUNCH), BM_GETCHECK, 0, 0) == BST_CHECKED);
            SaveOptionIni(iniPath, L"DontShowOnLaunch", v);
            v = (SendMessageW(GetDlgItem(hWnd, IDC_DONT_SHOW_TRAY), BM_GETCHECK, 0, 0) == BST_CHECKED);
            SaveOptionIni(iniPath, L"DontShowSystray", v);
            if (v)
            {
                HWND hTray = FindWindowW(L" ", NULL);
                if (hTray) PostMessageW(hTray, WM_STEAMLITE_REMOVE_TRAY, 0, 0);
            }
            if (launchMin || noJoy || noShaders || noGpu || console)
                CreateSteamShortcut();
            RegSetKeyValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Valve\\Steam", L"RunningAppID", REG_DWORD,
                            &(DWORD){0u}, sizeof(DWORD));
            DestroyWindow(hWnd);
        }
        else if (LOWORD(wParam) == IDC_QUIT)
            DestroyWindow(hWnd);
        break;
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        break;
    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

static HWND CreateOptionsDialog(HWND hParent)
{
    WNDCLASSW wc = {.style = CS_HREDRAW | CS_VREDRAW,
                    .lpfnWndProc = OptionsDialogProc,
                    .hInstance = GetModuleHandleW(NULL),
                    .lpszClassName = L"SteamLiteOptions"};
    RegisterClassW(&wc);

    HWND hDlg = CreateWindowExW(0, wc.lpszClassName, L"Steam Lite - Options",
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
                                100, 100, 380, 404, hParent, NULL, wc.hInstance, NULL);
    if (!hDlg) return NULL;

    WCHAR iniPath[MAX_PATH];
    get_ini_path(iniPath);
    DWORD def[11] = {0, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1};
    def[0] = LoadOptionIni(iniPath, L"FriendsSignIn", 0);
    def[1] = LoadOptionIni(iniPath, L"LaunchMinimized", 1);
    def[2] = LoadOptionIni(iniPath, L"NoJoystick", 0);
    def[3] = LoadOptionIni(iniPath, L"NoShaders", 1);
    def[4] = LoadOptionIni(iniPath, L"NoGPU", 1);
    def[5] = LoadOptionIni(iniPath, L"FriendsAnimed", 0);
    def[6] = LoadOptionIni(iniPath, L"ShowGameIcons", 0);
    def[7] = LoadOptionIni(iniPath, L"CEFAutomatic", 1);
    def[8] = LoadOptionIni(iniPath, L"DontShowOnLaunch", 0);
    def[9] = LoadOptionIni(iniPath, L"DontShowSystray", 0);
    def[10] = LoadOptionIni(iniPath, L"Console", 1);

    CreateWindowExW(0, L"Static", L"Shortcut Options:",
                    WS_CHILD | WS_VISIBLE, 16, 10, 200, 16, hDlg, NULL, wc.hInstance, NULL);
    CreateWindowExW(0, L"Button", L"Sign into friends",
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 16, 32, 320, 18, hDlg, (HMENU)IDC_FRIENDS_SIGNIN,
                    wc.hInstance, NULL);
    CreateWindowExW(0, L"Button", L"Launch minimized",
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 16, 54, 320, 18, hDlg, (HMENU)IDC_LAUNCH_MINIMIZED,
                    wc.hInstance, NULL);
    CreateWindowExW(0, L"Button", L"No joystick (disables gamepads)",
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 16, 76, 320, 18, hDlg, (HMENU)IDC_NO_JOYSTICK,
                    wc.hInstance, NULL);
    CreateWindowExW(0, L"Button", L"No shaders (CEF)",
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 16, 98, 320, 18, hDlg, (HMENU)IDC_NO_SHADERS,
                    wc.hInstance, NULL);
    CreateWindowExW(0, L"Button", L"No GPU (CEF)",
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 16, 120, 320, 18, hDlg, (HMENU)IDC_NO_GPU,
                    wc.hInstance, NULL);
    CreateWindowExW(0, L"Button", L"Animated avatars",
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 16, 142, 320, 18, hDlg, (HMENU)IDC_ANIMATED_AVATARS,
                    wc.hInstance, NULL);
    CreateWindowExW(0, L"Button", L"Show game icons",
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 16, 164, 320, 18, hDlg, (HMENU)IDC_SHOW_GAME_ICONS,
                    wc.hInstance, NULL);
    CreateWindowExW(0, L"Button", L"Enable Steam console",
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 16, 186, 320, 18, hDlg, (HMENU)IDC_ENABLE_CONSOLE,
                    wc.hInstance, NULL);
    CreateWindowExW(0, L"Button", L"Automatic CEF (disable CEF when a game runs)",
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 16, 208, 340, 36, hDlg, (HMENU)IDC_CEF_AUTOMATIC,
                    wc.hInstance, NULL);
    CreateWindowExW(0, L"Button", L"Don't show this dialog on launch again",
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 16, 250, 340, 18, hDlg, (HMENU)IDC_DONT_SHOW_LAUNCH,
                    wc.hInstance, NULL);
    CreateWindowExW(0, L"Button", L"Don't show the systray icon",
                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 16, 272, 340, 18, hDlg, (HMENU)IDC_DONT_SHOW_TRAY,
                    wc.hInstance, NULL);
    CreateWindowExW(0, L"Button", L"Save", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 16, 304, 100, 28, hDlg, (HMENU)IDC_SAVE,
                    wc.hInstance, NULL);
    CreateWindowExW(0, L"Button", L"Close", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 260, 304, 100, 28, hDlg, (HMENU)IDC_QUIT,
                    wc.hInstance, NULL);
    CreateWindowExW(0, L"Static", STEAMLITE_VERSION,
                    WS_CHILD | WS_VISIBLE | SS_RIGHT, 16, 340, 344, 14, hDlg, NULL, wc.hInstance, NULL);

    for (int i = 0; i < 11; i++)
        SendMessageW(GetDlgItem(hDlg, IDC_FRIENDS_SIGNIN + i), BM_SETCHECK, def[i] ? BST_CHECKED : BST_UNCHECKED, 0);
    return hDlg;
}

static void ShowOptionsDialog(HWND hParent)
{
    HWND hDlg = CreateOptionsDialog(hParent);
    if (!hDlg) return;
    ShowWindow(hDlg, SW_SHOW);
    MSG msg;
    while (IsWindow(hDlg) && GetMessageW(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_QUIT) break;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static NOTIFYICONDATAW _ = {.cbSize = sizeof(NOTIFYICONDATAW),
                                .uCallbackMessage = WM_USER,
                                .uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP,
                                .szTip = L"Steam Lite"};
    static UINT $ = WM_NULL;

    switch (uMsg)
    {
    case WM_CREATE:
        $ = RegisterWindowMessageW(L"TaskbarCreated");
        _.hWnd = hWnd;
        _.hIcon = LoadIconW(NULL, IDI_APPLICATION);
        {
            WCHAR iniPath[MAX_PATH];
            get_ini_path(iniPath);
            if (!LoadOptionIni(iniPath, L"DontShowSystray", 0))
                Shell_NotifyIconW(NIM_ADD, &_);
        }
        break;
    case WM_STEAMLITE_REMOVE_TRAY:
        Shell_NotifyIconW(NIM_DELETE, &_);
        return 0;

    case WM_USER:
        if (lParam == WM_RBUTTONDOWN)
        {
            WCHAR iniPath[MAX_PATH];
            get_ini_path(iniPath);
            DWORD mode = LoadOptionIni(iniPath, L"Mode", 3);

            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING | (mode == 3 ? MF_CHECKED : 0), 3, L"Automatic CEF");
            AppendMenuW(hMenu, MF_STRING | (mode == 1 ? MF_CHECKED : 0), 1, L"Enable CEF");
            AppendMenuW(hMenu, MF_STRING | (mode == 2 ? MF_CHECKED : 0), 2, L"Disable CEF");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, 4, L"Options");
            SetForegroundWindow(hWnd);

            POINT _ = {};
            GetCursorPos(&_);
            DWORD cmd = (DWORD)TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD,
                                              _.x, _.y, 0, hWnd, NULL);
            if (cmd == 4)
            {
                PostMessageW(hWnd, WM_NULL, 0, 0);
                DestroyMenu(hMenu);
                ShowOptionsDialog(hWnd);
                return 0;
            }
            if (cmd != 0)
            {
                get_ini_path(iniPath);
                SaveOptionIni(iniPath, L"Mode", cmd);
                if (cmd != 3)
                    RegSetKeyValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Valve\\Steam", L"RunningAppID", REG_DWORD,
                                    &(DWORD){cmd == 2 ? 1u : 0u}, sizeof(DWORD));
                else
                    RegSetKeyValueW(HKEY_CURRENT_USER, L"SOFTWARE\\Valve\\Steam", L"RunningAppID", REG_DWORD,
                                    &(DWORD){0u}, sizeof(DWORD));
            }
            PostMessageW(hWnd, WM_NULL, 0, 0);
            DestroyMenu(hMenu);
        }
        break;

    default:
        if (uMsg == $)
        {
            WCHAR iniPath[MAX_PATH];
            get_ini_path(iniPath);
            if (!LoadOptionIni(iniPath, L"DontShowSystray", 0))
                Shell_NotifyIconW(NIM_ADD, &_);
        }
        break;
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

static VOID CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild,
                                  DWORD dwEventThread, DWORD dwmsEventTime)
{
    WCHAR szClassName[16] = {};
    GetClassNameW(hwnd, szClassName, 16);

    if (CompareStringOrdinal(L"vguiPopupWindow", -1, szClassName, -1, FALSE) != CSTR_EQUAL ||
        GetWindowTextLengthW(hwnd) < 1)
        return;

    CloseHandle(CreateThread(NULL, 0, ThreadProc, (LPVOID)FALSE, 0, NULL));

    HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, dwEventThread);
    HKEY hKey = NULL;
    RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Valve\\Steam", REG_OPTION_NON_VOLATILE, KEY_NOTIFY | KEY_QUERY_VALUE,
                  &hKey);

    while (!RegNotifyChangeKeyValue(hKey, FALSE, REG_NOTIFY_CHANGE_LAST_SET, NULL, FALSE))
    {
        BOOL _ = FALSE;
        RegGetValueW(hKey, NULL, L"RunningAppID", RRF_RT_REG_DWORD, NULL, (PVOID)&_, &((DWORD){sizeof(BOOL)}));
        (_ ? SuspendThread : ResumeThread)(hThread);
        if (_)
        {
            WTS_PROCESS_INFOW *pProcessInfo = {};
            DWORD $ = {};

            WTSEnumerateProcessesW(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pProcessInfo, &$);
            for (DWORD _ = {}; _ < $; _++)
                if (CompareStringOrdinal(pProcessInfo[_].pProcessName, -1, L"steamwebhelper.exe", -1, TRUE) ==
                    CSTR_EQUAL)
                {
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE, FALSE,
                                                  pProcessInfo[_].ProcessId);

                    PROCESS_BASIC_INFORMATION _ = {};
                    NtQueryInformationProcess(hProcess, ProcessBasicInformation, &_, sizeof(PROCESS_BASIC_INFORMATION),
                                              NULL);

                    if (_.InheritedFromUniqueProcessId == GetCurrentProcessId())
                        TerminateProcess(hProcess, EXIT_SUCCESS);

                    CloseHandle(hProcess);
                }
            WTSFreeMemory(pProcessInfo);
        }
    }
}

static DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
    if (lpParameter)
    {
        WCHAR iniPath[MAX_PATH];
        get_ini_path(iniPath);
        if (!LoadOptionIni(iniPath, L"DontShowOnLaunch", 0))
            ShowOptionsDialog(NULL);
        SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_CREATE, NULL, WinEventProc, GetCurrentProcessId(), (DWORD){},
                        WINEVENT_OUTOFCONTEXT);
    }
    else
        CreateWindowExW(
            WS_EX_LEFT | WS_EX_LTRREADING,
            (LPCWSTR)(ULONG_PTR)RegisterClassW(&((WNDCLASSW){
                .lpszClassName = L" ", .hInstance = GetModuleHandleW(NULL), .lpfnWndProc = (WNDPROC)WndProc})),
            NULL, WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL, GetModuleHandleW(NULL), NULL);

    MSG _ = {};
    while (GetMessageW(&_, NULL, (UINT){}, (UINT){}))
    {
        TranslateMessage(&_);
        DispatchMessageW(&_);
    }
    return EXIT_SUCCESS;
}

BOOL WINAPI DllMainCRTStartup(HINSTANCE hLibModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hModule = hLibModule;
        DisableThreadLibraryCalls(hLibModule);
        CloseHandle(CreateThread(NULL, 0, ThreadProc, (LPVOID)TRUE, 0, NULL));
    }
    return TRUE;
}
