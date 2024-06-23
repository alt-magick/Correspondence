#include <windows.h>
#include <commdlg.h>

#define ID_FILE_SAVE 1001
#define ID_FILE_EXIT 1002
#define ID_FILE_OPEN 1003
#define ID_TOGGLE_SUBSTITUTION 1004

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")

#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup")

const wchar_t CLASS_NAME[] = L"Correspondence";
const wchar_t WINDOW_TITLE[] = L"Correspondence";

HWND hEdit;
WNDPROC OldEditProc;
HFONT hFont;
OPENFILENAME ofn;
bool enableSubstitution = true;

void SaveFile(HWND hwnd) {
    WCHAR szFileName[MAX_PATH] = L"";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFileName;
    ofn.lpstrFile[0] = L'\0';
    ofn.nMaxFile = sizeof(szFileName);
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;

    if (GetSaveFileName(&ofn) == TRUE) {
        HANDLE hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            MessageBox(hwnd, L"Failed to create file!", L"Error", MB_OK | MB_ICONERROR);
            return;
        }

        int len = GetWindowTextLength(hEdit);
        if (len > 0) {
            WCHAR* buffer = new WCHAR[len + 1];
            GetWindowText(hEdit, buffer, len + 1);

            const WCHAR bom = 0xFEFF;
            DWORD bytesWritten;
            WriteFile(hFile, &bom, sizeof(WCHAR), &bytesWritten, NULL);

            WriteFile(hFile, buffer, len * sizeof(WCHAR), &bytesWritten, NULL);

            delete[] buffer;
        }

        CloseHandle(hFile);

        MessageBox(hwnd, L"File saved successfully!", L"Correspondence", MB_OK | MB_ICONINFORMATION);
    }
}

void OpenFile(HWND hwnd) {
    WCHAR szFileName[MAX_PATH] = L"";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFileName;
    ofn.lpstrFile[0] = L'\0';
    ofn.nMaxFile = sizeof(szFileName);
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;

    if (GetOpenFileName(&ofn) == TRUE) {
        HANDLE hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            MessageBox(hwnd, L"Failed to open file!", L"Error", MB_OK | MB_ICONERROR);
            return;
        }

        DWORD dwFileSize = GetFileSize(hFile, NULL);
        if (dwFileSize == INVALID_FILE_SIZE) {
            CloseHandle(hFile);
            MessageBox(hwnd, L"Failed to get file size!", L"Error", MB_OK | MB_ICONERROR);
            return;
        }

        WCHAR* buffer = new WCHAR[dwFileSize / sizeof(WCHAR) + 1];

        DWORD bytesRead;
        if (ReadFile(hFile, buffer, dwFileSize, &bytesRead, NULL)) {
            if (bytesRead >= sizeof(WCHAR) && buffer[0] == 0xFEFF) {
                memmove(buffer, buffer + 1, (bytesRead - sizeof(WCHAR)));
                bytesRead -= sizeof(WCHAR);
            }

            buffer[bytesRead / sizeof(WCHAR)] = L'\0';

            SetWindowText(hEdit, buffer);
        }
        else {
            MessageBox(hwnd, L"Failed to read file!", L"Error", MB_OK | MB_ICONERROR);
        }

        delete[] buffer;
        CloseHandle(hFile);
    }
}

LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CHAR: {
        if (enableSubstitution) {
            switch (wParam) {
            case 'A': case 'a': wParam = L'☉'; break;
            case 'B': case 'b': wParam = L'●'; break;
            case 'C': case 'c': wParam = L'☾'; break;
            case 'D': case 'd': wParam = L'☽'; break;
            case 'E': case 'e': wParam = L'○'; break;
            case 'F': case 'f': wParam = L'☿'; break;
            case 'G': case 'g': wParam = L'♀'; break;
            case 'H': case 'h': wParam = L'♁'; break;
            case 'I': case 'i': wParam = L'♂'; break;
            case 'J': case 'j': wParam = L'♃'; break;
            case 'K': case 'k': wParam = L'♄'; break;
            case 'L': case 'l': wParam = L'♅'; break;
            case 'M': case 'm': wParam = L'♆'; break;
            case 'N': case 'n': wParam = L'♇'; break;
            case 'O': case 'o': wParam = L'♈'; break;
            case 'P': case 'p': wParam = L'♉'; break;
            case 'Q': case 'q': wParam = L'♊'; break;
            case 'R': case 'r': wParam = L'♋'; break;
            case 'S': case 's': wParam = L'♌'; break;
            case 'T': case 't': wParam = L'♍'; break;
            case 'U': case 'u': wParam = L'♎'; break;
            case 'V': case 'v': wParam = L'♏'; break;
            case 'W': case 'w': wParam = L'♐'; break;
            case 'X': case 'x': wParam = L'♑'; break;
            case 'Y': case 'y': wParam = L'♒'; break;
            case 'Z': case 'z': wParam = L'♓'; break;
            }
        }

        break;
    }
    }
    return CallWindowProc(OldEditProc, hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        hEdit = CreateWindowExW(
            0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
            ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
            0, 0, 0, 0,
            hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);
        if (hEdit == NULL) {
            MessageBox(hwnd, L"Could not create edit box.", L"Error", MB_OK | MB_ICONERROR);
            return -1;
        }

        hFont = CreateFontW(
            32, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS, L"Segoe UI Symbol");
        SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

        OldEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);
        break;
    }
    case WM_SIZE: {
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        SetWindowPos(hEdit, NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOZORDER);
        break;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case ID_FILE_SAVE:
            SaveFile(hwnd);
            break;
        case ID_FILE_OPEN:
            OpenFile(hwnd);
            break;
        case ID_TOGGLE_SUBSTITUTION:
            enableSubstitution = !enableSubstitution;
            if (enableSubstitution) {
                MessageBox(hwnd, L"Code enabled.", L"Correspondence", MB_OK | MB_ICONINFORMATION);
            }
            else {
                MessageBox(hwnd, L"Code disabled.", L"Correspondence", MB_OK | MB_ICONINFORMATION);
            }
            break;
        case ID_FILE_EXIT:
            DestroyWindow(hwnd);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        break;
    }
    case WM_DESTROY: {
        if (hFont) {
            DeleteObject(hFont);
        }
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        return 0;
    }

    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();
    AppendMenu(hSubMenu, MF_STRING, ID_FILE_OPEN, L"&Open");
    AppendMenu(hSubMenu, MF_STRING, ID_FILE_SAVE, L"&Save");
    AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hSubMenu, MF_STRING, ID_TOGGLE_SUBSTITUTION, L"&Toggle Code");
    AppendMenu(hSubMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hSubMenu, MF_STRING, ID_FILE_EXIT, L"E&xit");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, L"&File");
    SetMenu(hwnd, hMenu);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}
