#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string>

#define ID_FILE_SAVE 1001
#define ID_FILE_EXIT 1002
#define ID_FILE_OPEN 1003
#define ID_TOGGLE_SUBSTITUTION 1004
#define ID_CODE_ENCODE 1005
#define ID_CODE_DECODE 1006
#define ID_STATUSBAR 1007

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup")

const wchar_t CLASS_NAME[] = L"Correspondence";
const wchar_t WINDOW_TITLE[] = L"Correspondence";

HWND hEdit;
HWND hStatusBar;
WNDPROC OldEditProc;
HFONT hFont;
OPENFILENAME ofn;
bool enableSubstitution = true;

void SaveFile(HWND hwnd);
void OpenFile(HWND hwnd);
void EncodeText();
void DecodeText();

void UpdateStatusBar() {
    int index = 0;
    const int bufferSize = 256;
    wchar_t statusText[bufferSize];
    swprintf_s(statusText, bufferSize, L"Code %s", enableSubstitution ? L"Enabled" : L"Disabled");
    SendMessage(hStatusBar, SB_SETTEXT, index, (LPARAM)statusText);
}

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

void EncodeText() {
    int len = GetWindowTextLength(hEdit);
    if (len > 0) {
        WCHAR* buffer = new WCHAR[len + 1];
        GetWindowText(hEdit, buffer, len + 1);

        for (int i = 0; i < len; ++i) {
            WCHAR originalChar = buffer[i];
            WCHAR newChar = originalChar; // Initialize with the original character

            // Perform custom character substitution based on originalChar
            switch (originalChar) {
            case L'A': case L'a': newChar = L'☉'; break;
            case L'B': case L'b': newChar = L'●'; break;
            case L'C': case L'c': newChar = L'☾'; break;
            case L'D': case L'd': newChar = L'☽'; break;
            case L'E': case L'e': newChar = L'○'; break;
            case L'F': case L'f': newChar = L'☿'; break;
            case L'G': case L'g': newChar = L'♀'; break;
            case L'H': case L'h': newChar = L'♁'; break;
            case L'I': case L'i': newChar = L'♂'; break;
            case L'J': case L'j': newChar = L'♃'; break;
            case L'K': case L'k': newChar = L'♄'; break;
            case L'L': case L'l': newChar = L'♅'; break;
            case L'M': case L'm': newChar = L'♆'; break;
            case L'N': case L'n': newChar = L'♇'; break;
            case L'O': case L'o': newChar = L'♈'; break;
            case L'P': case L'p': newChar = L'♉'; break;
            case L'Q': case L'q': newChar = L'♊'; break;
            case L'R': case L'r': newChar = L'♋'; break;
            case L'S': case L's': newChar = L'♌'; break;
            case L'T': case L't': newChar = L'♍'; break;
            case L'U': case L'u': newChar = L'♎'; break;
            case L'V': case L'v': newChar = L'♏'; break;
            case L'W': case L'w': newChar = L'♐'; break;
            case L'X': case L'x': newChar = L'♑'; break;
            case L'Y': case L'y': newChar = L'♒'; break;
            case L'Z': case L'z': newChar = L'♓'; break;
            default: // If no substitution, keep the original character
                newChar = originalChar;
                break;
            }

            buffer[i] = newChar; // Replace the character in the buffer
        }

        SetWindowText(hEdit, buffer);
        delete[] buffer;
    }

    UpdateStatusBar();
}

void DecodeText() {
    int len = GetWindowTextLength(hEdit);
    if (len > 0) {
        WCHAR* buffer = new WCHAR[len + 1];
        GetWindowText(hEdit, buffer, len + 1);

        for (int i = 0; i < len; ++i) {
            WCHAR originalChar = buffer[i];
            WCHAR newChar = originalChar; // Initialize with the original character

            // Perform custom character substitution based on originalChar
            switch (originalChar) {
            case L'☉': newChar = L'a'; break;
            case L'●': newChar = L'b'; break;
            case L'☾': newChar = L'c'; break;
            case L'☽': newChar = L'd'; break;
            case L'○': newChar = L'e'; break;
            case L'☿': newChar = L'f'; break;
            case L'♀': newChar = L'g'; break;
            case L'♁': newChar = L'h'; break;
            case L'♂': newChar = L'i'; break;
            case L'♃': newChar = L'j'; break;
            case L'♄': newChar = L'k'; break;
            case L'♅': newChar = L'l'; break;
            case L'♆': newChar = L'm'; break;
            case L'♇': newChar = L'n'; break;
            case L'♈': newChar = L'o'; break;
            case L'♉': newChar = L'p'; break;
            case L'♊': newChar = L'q'; break;
            case L'♋': newChar = L'r'; break;
            case L'♌': newChar = L's'; break;
            case L'♍': newChar = L't'; break;
            case L'♎': newChar = L'u'; break;
            case L'♏': newChar = L'v'; break;
            case L'♐': newChar = L'w'; break;
            case L'♑': newChar = L'x'; break;
            case L'♒': newChar = L'y'; break;
            case L'♓': newChar = L'z'; break;
            default: // If no substitution, keep the original character
                newChar = originalChar;
                break;
            }

            buffer[i] = newChar; // Replace the character in the buffer
        }

        SetWindowText(hEdit, buffer);
        delete[] buffer;
    }

    UpdateStatusBar();
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

        // Create status bar
        hStatusBar = CreateWindowExW(
            0, STATUSCLASSNAME, NULL,
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            hwnd, (HMENU)ID_STATUSBAR, GetModuleHandle(NULL), NULL);

        // Set up the parts of the status bar
        int statusParts[] = { 200, -1 }; // 200 for the status text, -1 for sizing grip
        SendMessage(hStatusBar, SB_SETPARTS, sizeof(statusParts) / sizeof(int), (LPARAM)&statusParts);

        UpdateStatusBar(); // Initialize status bar text
        break;
    }
    case WM_SIZE: {
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        SetWindowPos(hEdit, NULL, 0, 0, rcClient.right, rcClient.bottom - 20, SWP_NOZORDER); // Adjust size for status bar
        SetWindowPos(hStatusBar, NULL, 0, rcClient.bottom - 20, rcClient.right, 20, SWP_NOZORDER); // Position status bar
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
            UpdateStatusBar(); // Update status bar text            
            break;
        case ID_CODE_ENCODE:
            EncodeText();
            break;
        case ID_CODE_DECODE:
            DecodeText();
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
    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(iccex);
    iccex.dwICC = ICC_WIN95_CLASSES; // or ICC_STANDARD_CLASSES
    InitCommonControlsEx(&iccex);

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
    HMENU hSubMenuFile = CreatePopupMenu();
    AppendMenu(hSubMenuFile, MF_STRING, ID_FILE_OPEN, L"&Open");
    AppendMenu(hSubMenuFile, MF_STRING, ID_FILE_SAVE, L"&Save");
    AppendMenu(hSubMenuFile, MF_SEPARATOR, 0, NULL);
    AppendMenu(hSubMenuFile, MF_STRING, ID_TOGGLE_SUBSTITUTION, L"&Toggle Code");
    AppendMenu(hSubMenuFile, MF_SEPARATOR, 0, NULL);
    AppendMenu(hSubMenuFile, MF_STRING, ID_FILE_EXIT, L"E&xit");

    HMENU hSubMenuCode = CreatePopupMenu();
    AppendMenu(hSubMenuCode, MF_STRING, ID_CODE_ENCODE, L"&Encode");
    AppendMenu(hSubMenuCode, MF_STRING, ID_CODE_DECODE, L"&Decode");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenuFile, L"&File");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenuCode, L"&Code");

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
