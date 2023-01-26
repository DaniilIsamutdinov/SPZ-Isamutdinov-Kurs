#include "framework.h"
#include "KursovayaApplication.h"
#include <Windows.h>
#include <string>
#include <vector> 

#include "winnt.h"
#include "fileapi.h"
#include "shobjidl_core.h"
#include "winbase.h"


#define MAX_LOADSTRING 100

// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна

// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Разместите код здесь.

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_KURSOVAYAAPPLICATION, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Выполнить инициализацию приложения:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_KURSOVAYAAPPLICATION));

    MSG msg;

    // Цикл основного сообщения:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KURSOVAYAAPPLICATION));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDR_MENU1);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
       100, 50, 1200, 800, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

HWND HWNDlogs;
std::vector<LPWSTR> pathList;
DWORD WINAPI WriteFolderChanges(LPVOID);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
    case WM_CREATE:
    {
        RECT rt;
        GetClientRect(hWnd, &rt);
        HWNDlogs = CreateWindowA("listbox", "Логи", WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_DISABLENOSCROLL | WS_VSCROLL,
            0, 0, rt.right, rt.bottom, hWnd, HMENU(0), NULL, NULL);
        break;
    }
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case 100:
        {
            IFileDialog* fileDialog; 
            CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fileDialog));

            DWORD dwOptions;
            fileDialog->GetOptions(&dwOptions);
            fileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS); 
            if (SUCCEEDED(fileDialog->Show(NULL))) 
            {
                LPWSTR path;
                IShellItem* selectedItem;
                fileDialog->GetResult(&selectedItem);
                selectedItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &path);
                selectedItem->Release();
                fileDialog->Release();
                pathList.push_back(path);
                CreateThread(NULL, 0, WriteFolderChanges, NULL, 0, 0);
            }
            else { MessageBoxA(NULL, "Выберите папку для отслеживания", "Ошибка", MB_ICONERROR); }   
            break;
        }
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

DWORD WINAPI WriteFolderChanges(LPVOID)
{
    LPWSTR path = pathList[pathList.size() - 1];
    HANDLE FileHandle = CreateFile(path, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE, 
        NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
    if (FileHandle == INVALID_HANDLE_VALUE) { MessageBoxA(NULL, "INVALID_HANDLE_VALUE", NULL, NULL); }
    OVERLAPPED overlapped;
    overlapped.hEvent = CreateEvent(NULL, FALSE, 0, NULL);
    uint8_t Data[1024];
    ReadDirectoryChangesW(FileHandle, Data, 1024,
        TRUE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE, NULL, &overlapped, NULL);
    while (true) { 
        DWORD result = WaitForSingleObject(overlapped.hEvent, 0);
        if (result == WAIT_OBJECT_0) { 
            DWORD dataUsage;
            GetOverlappedResult(path, &overlapped, &dataUsage, FALSE);
            FILE_NOTIFY_INFORMATION* event = (FILE_NOTIFY_INFORMATION*)Data; 
            std::wstring OperationType;
            while (true) { 
                DWORD folderNameLength = event->FileNameLength / sizeof(wchar_t); 
                std::wstring FileName;
                FileName.assign(event->FileName, folderNameLength); 
                std::wstring text = L"В папке "; 
                text += path;
                switch (event->Action) { 
                case FILE_ACTION_ADDED: { text += L" создан файл с именем \"" + FileName + L"\""; break; }
                case FILE_ACTION_REMOVED: { text += L" файл \"" + FileName + L"\" был удален";  break; }
                case FILE_ACTION_MODIFIED: { text += L" в этом файле произошли изменения \"" + FileName + L"\""; break; }
                case FILE_ACTION_RENAMED_OLD_NAME: { text += L" имя файла \"" + FileName + L"\" было изменино"; break; }
                case FILE_ACTION_RENAMED_NEW_NAME: { text += L" новое имя - \"" + FileName + L"\""; break; }
                }
                SendMessage(HWNDlogs, LB_ADDSTRING, 0, (LPARAM)text.c_str()); 
                if (event->NextEntryOffset) { *((uint8_t**)&event) += event->NextEntryOffset; }
                else { break; } 
            }
            ReadDirectoryChangesW(FileHandle, Data, 1024, TRUE, FILE_NOTIFY_CHANGE_FILE_NAME | 
                FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE, NULL, &overlapped, NULL);
        }
    }
    ExitThread(0);
}