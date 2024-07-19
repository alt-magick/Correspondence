#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <string>
#include <stack>

#define ID_FILE_SAVE 1001
#define ID_FILE_EXIT 1002
#define ID_FILE_OPEN 1003
#define ID_TOGGLE_SUBSTITUTION 1004
#define ID_CODE_ENCODE 1005
#define ID_CODE_DECODE 1006
#define ID_STATUSBAR 1007
#define ID_EDIT_CUT 1008
#define ID_EDIT_COPY 1009
#define ID_EDIT_PASTE 1010
#define ID_EDIT_DELETE 1011
#define ID_EDIT_ALL 1012
#define ID_EDIT_UNDO 1013
#define ID_EDIT_REDO 1014
#define ID_CTRL_E_ENCODE 1015
#define ID_CTRL_D_DECODE 1016
#define ID_CTRL_T 1017
#define ID_CTRL_S 1018
#define ID_CTRL_O 1019
#define ID_CTRL_Q 1020
#define ID_CTRL_A 1021
#define ID_CTRL_X 1022
#define ID_CTRL_V 1023
#define ID_CTRL_C 1024

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:WinMainCRTStartup")

const wchar_t CLASS_NAME[] = L"Correspondence";
const wchar_t WINDOW_TITLE[] = L"Correspondence";

HWND hEdit;
HWND hStatusBar;
WNDPROC OldEditProc;
HFONT hFont;
OPENFILENAMEW ofn;
bool enableSubstitution = false;

// Stack for undo functionality
std::stack<std::wstring> undoStack;
std::stack<std::wstring> redoStack;

void SaveFile(HWND hwnd);
void OpenFile(HWND hwnd);
void EncodeText();
void DecodeText();

void SaveCurrentStateForUndo() {
    std::wstring currentState;
    int length = GetWindowTextLengthW(hEdit) + 1;
    currentState.resize(length);
    GetWindowTextW(hEdit, &currentState[0], length);
    undoStack.push(currentState);
    while (!redoStack.empty()) {
        redoStack.pop();
    }
}

void SelectAllText(HWND hwndEdit) {

    int textLength = GetWindowTextLengthW(hwndEdit);
    SendMessageW(hwndEdit, EM_SETSEL, 0, textLength);
}

void UpdateStatusBar() {
    int index = 0;
    const int bufferSize = 256;
    wchar_t statusText[bufferSize];
    swprintf(statusText, bufferSize, L"Code %ls", enableSubstitution ? L"Enabled" : L"Disabled");
    SendMessageW(hStatusBar, SB_SETTEXTW, index, (LPARAM)statusText);
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

    if (GetSaveFileNameW(&ofn) == TRUE) {
        HANDLE hFile = CreateFileW(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            MessageBoxW(hwnd, L"Failed to create file!", L"Error", MB_OK | MB_ICONERROR);
            return;
        }

        int len = GetWindowTextLengthW(hEdit);
        if (len > 0) {
            WCHAR* buffer = new WCHAR[len + 1];
            GetWindowTextW(hEdit, buffer, len + 1);

            const WCHAR bom = 0xFEFF;
            DWORD bytesWritten;
            WriteFile(hFile, &bom, sizeof(WCHAR), &bytesWritten, NULL);

            WriteFile(hFile, buffer, len * sizeof(WCHAR), &bytesWritten, NULL);

            delete[] buffer;
        }

        CloseHandle(hFile);

        MessageBoxW(hwnd, L"File saved successfully!", L"Correspondence", MB_OK | MB_ICONINFORMATION);
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

    if (GetOpenFileNameW(&ofn) == TRUE) {
        HANDLE hFile = CreateFileW(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            MessageBoxW(hwnd, L"Failed to open file!", L"Error", MB_OK | MB_ICONERROR);
            return;
        }

        DWORD dwFileSize = GetFileSize(hFile, NULL);
        if (dwFileSize == INVALID_FILE_SIZE) {
            CloseHandle(hFile);
            MessageBoxW(hwnd, L"Failed to get file size!", L"Error", MB_OK | MB_ICONERROR);
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

            SetWindowTextW(hEdit, buffer);
        }
        else {
            MessageBoxW(hwnd, L"Failed to read file!", L"Error", MB_OK | MB_ICONERROR);
        }

        delete[] buffer;
        CloseHandle(hFile);
    }
}

void EncodeText() {
    int len = GetWindowTextLengthW(hEdit);
    if (len > 0) {
        WCHAR* buffer = new WCHAR[len + 1];
        GetWindowTextW(hEdit, buffer, len + 1);

        for (int i = 0; i < len; ++i) {
            WCHAR originalChar = buffer[i];
            WCHAR newChar = originalChar;

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
            default:
                newChar = originalChar;
                break;
            }

            buffer[i] = newChar;
        }

        SetWindowTextW(hEdit, buffer);
        delete[] buffer;
    }

    UpdateStatusBar();
}

void DecodeText() {
    int len = GetWindowTextLengthW(hEdit);
    if (len > 0) {
        WCHAR* buffer = new WCHAR[len + 1];
        GetWindowTextW(hEdit, buffer, len + 1);

        for (int i = 0; i < len; ++i) {
            WCHAR originalChar = buffer[i];
            WCHAR newChar = originalChar;

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
            default:
                newChar = originalChar;
                break;
            }

            buffer[i] = newChar;
        }

        SetWindowTextW(hEdit, buffer);
        delete[] buffer;
    }

    UpdateStatusBar();
}

void Undo() {
    if (!undoStack.empty()) {
        std::wstring currentState;
        int length = GetWindowTextLengthW(hEdit) + 1;
        currentState.resize(length);
        GetWindowTextW(hEdit, &currentState[0], length);

        std::wstring previousState = undoStack.top();
        redoStack.push(currentState); // Push the current state to the redo stack
        undoStack.pop();
        SetWindowTextW(hEdit, previousState.c_str());
    }
}

void Redo() {
    if (!redoStack.empty()) {
        std::wstring currentState;
        int length = GetWindowTextLengthW(hEdit) + 1;
        currentState.resize(length);
        GetWindowTextW(hEdit, &currentState[0], length);

        std::wstring nextState = redoStack.top();
        undoStack.push(currentState); // Push the current state to the undo stack
        redoStack.pop();
        SetWindowTextW(hEdit, nextState.c_str());
    }
}


LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CHAR: {
        bool ctrlPressed = GetKeyState(VK_CONTROL) & 0x8000;

        if (ctrlPressed) {

            return 0;
        }
        if (wParam == VK_RETURN || wParam == VK_DELETE || wParam == VK_BACK || wParam == VK_TAB) {
            SaveCurrentStateForUndo();
        }
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
    case WM_KEYDOWN: {
        if (GetKeyState(VK_CONTROL) & 0x8000 && wParam == 'E') {
            SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(ID_CTRL_E_ENCODE, 0), (LPARAM)hwnd);
            return 0;
        }

        if (GetKeyState(VK_CONTROL) & 0x8000 && wParam == 'D') {
            SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(ID_CTRL_D_DECODE, 0), (LPARAM)hwnd);
            return 0;
        }

        if (GetKeyState(VK_CONTROL) & 0x8000 && wParam == 'T') {
            SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(ID_CTRL_T, 0), (LPARAM)hwnd);
            return 0;
        }
        if (GetKeyState(VK_CONTROL) & 0x8000 && wParam == 'S') {
            SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(ID_FILE_SAVE, 0), (LPARAM)hwnd);
            return 0;
        }
        if (GetKeyState(VK_CONTROL) & 0x8000 && wParam == 'O') {
            SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(ID_FILE_OPEN, 0), (LPARAM)hwnd);
            return 0;
        }
        if (GetKeyState(VK_CONTROL) & 0x8000 && wParam == 'Q') {
            SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(ID_FILE_EXIT, 0), (LPARAM)hwnd);
            return 0;
        }
        if (GetKeyState(VK_CONTROL) & 0x8000 && wParam == 'A') {
            SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(ID_CTRL_A, 0), (LPARAM)hwnd);
            return 0;
        }
        if (GetKeyState(VK_CONTROL) & 0x8000 && wParam == 'X') {
            SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(ID_CTRL_X, 0), (LPARAM)hwnd);
            return 0;
        }
        if (GetKeyState(VK_CONTROL) & 0x8000 && wParam == 'V') {
            SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(ID_CTRL_V, 0), (LPARAM)hwnd);
            return 0;
        }
        if (GetKeyState(VK_CONTROL) & 0x8000 && wParam == 'C') {
            SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(ID_CTRL_C, 0), (LPARAM)hwnd);
            return 0;
        }
        if (GetKeyState(VK_CONTROL) & 0x8000 && wParam == 'Z') {
            Undo();
            return 0;
        }
        if (GetKeyState(VK_CONTROL) & 0x8000 && wParam == 'Y') {
            Redo();
            return 0;
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
            WS_CHILD | WS_VISIBLE | WS_VSCROLL |
            ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
            0, 0, 0, 0,
            hwnd, (HMENU)1, GetModuleHandleW(NULL), NULL);
        if (hEdit == NULL) {
            MessageBoxW(hwnd, L"Could not create edit box.", L"Error", MB_OK | MB_ICONERROR);
            return -1;
        }
        SendMessageW(hEdit, EM_SETWORDBREAKPROC, 0, 0);
        hFont = CreateFontW(
            32, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS, L"Segoe UI Symbol");
        SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

        OldEditProc = (WNDPROC)SetWindowLongPtrW(hEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);

        hStatusBar = CreateWindowExW(
            0, STATUSCLASSNAMEW, NULL,
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0,
            hwnd, (HMENU)ID_STATUSBAR, GetModuleHandleW(NULL), NULL);

        int statusParts[] = { 125, -1 };
        SendMessageW(hStatusBar, SB_SETPARTS, sizeof(statusParts) / sizeof(int), (LPARAM)&statusParts);

        UpdateStatusBar();
        break;
    }
    case WM_SIZE: {
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);
        SetWindowPos(hEdit, NULL, 0, 0, rcClient.right, rcClient.bottom - 20, SWP_NOZORDER);
        SetWindowPos(hStatusBar, NULL, 0, rcClient.bottom - 20, rcClient.right, 20, SWP_NOZORDER);
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
            UpdateStatusBar();
            break;
        case ID_CODE_ENCODE:
            SaveCurrentStateForUndo();
            SendMessageW(hEdit, WM_CUT, 0, 0);
            if (OpenClipboard(hwnd)) {
                HANDLE hClipboardData = GetClipboardData(CF_UNICODETEXT);
                if (hClipboardData != NULL) {
                    WCHAR* pszData = static_cast<WCHAR*>(GlobalLock(hClipboardData));
                    if (pszData != NULL) {

                        size_t length = wcslen(pszData);
                        WCHAR* pszModifiedData = new WCHAR[length + 1];
                        if (pszModifiedData != NULL) {
                            for (size_t i = 0; i < length; ++i) {
                                if (pszData[i] == L'a' || pszData[i] == L'A') {
                                    pszModifiedData[i] = L'☉';
                                }
                                else if (pszData[i] == L'b' || pszData[i] == L'B') {
                                    pszModifiedData[i] = L'●';
                                }
                                else if (pszData[i] == L'c' || pszData[i] == L'C') {
                                    pszModifiedData[i] = L'☾';
                                }
                                else if (pszData[i] == L'd' || pszData[i] == L'D') {
                                    pszModifiedData[i] = L'☽';
                                }
                                else if (pszData[i] == L'e' || pszData[i] == L'E') {
                                    pszModifiedData[i] = L'○';
                                }
                                else if (pszData[i] == L'f' || pszData[i] == L'F') {
                                    pszModifiedData[i] = L'☿';
                                }
                                else if (pszData[i] == L'g' || pszData[i] == L'G') {
                                    pszModifiedData[i] = L'♀';
                                }
                                else if (pszData[i] == L'h' || pszData[i] == L'H') {
                                    pszModifiedData[i] = L'♁';
                                }
                                else if (pszData[i] == L'i' || pszData[i] == L'I') {
                                    pszModifiedData[i] = L'♂';
                                }
                                else if (pszData[i] == L'j' || pszData[i] == L'J') {
                                    pszModifiedData[i] = L'♃';
                                }
                                else if (pszData[i] == L'k' || pszData[i] == L'K') {
                                    pszModifiedData[i] = L'♄';
                                }
                                else if (pszData[i] == L'l' || pszData[i] == L'L') {
                                    pszModifiedData[i] = L'♅';
                                }
                                else if (pszData[i] == L'm' || pszData[i] == L'M') {
                                    pszModifiedData[i] = L'♆';
                                }
                                else if (pszData[i] == L'n' || pszData[i] == L'N') {
                                    pszModifiedData[i] = L'♇';
                                }
                                else if (pszData[i] == L'o' || pszData[i] == L'O') {
                                    pszModifiedData[i] = L'♈';
                                }
                                else if (pszData[i] == L'p' || pszData[i] == L'P') {
                                    pszModifiedData[i] = L'♉';
                                }
                                else if (pszData[i] == L'q' || pszData[i] == L'Q') {
                                    pszModifiedData[i] = L'♊';
                                }
                                else if (pszData[i] == L'r' || pszData[i] == L'R') {
                                    pszModifiedData[i] = L'♋';
                                }
                                else if (pszData[i] == L's' || pszData[i] == L'S') {
                                    pszModifiedData[i] = L'♌';
                                }
                                else if (pszData[i] == L't' || pszData[i] == L'T') {
                                    pszModifiedData[i] = L'♍';
                                }
                                else if (pszData[i] == L'u' || pszData[i] == L'U') {
                                    pszModifiedData[i] = L'♎';
                                }
                                else if (pszData[i] == L'v' || pszData[i] == L'V') {
                                    pszModifiedData[i] = L'♏';
                                }
                                else if (pszData[i] == L'w' || pszData[i] == L'W') {
                                    pszModifiedData[i] = L'♐';
                                }
                                else if (pszData[i] == L'x' || pszData[i] == L'X') {
                                    pszModifiedData[i] = L'♑';
                                }
                                else if (pszData[i] == L'y' || pszData[i] == L'Y') {
                                    pszModifiedData[i] = L'♒';
                                }
                                else if (pszData[i] == L'z' || pszData[i] == L'Z') {
                                    pszModifiedData[i] = L'♓';
                                }
                                else {
                                    pszModifiedData[i] = pszData[i];
                                }
                            }
                            pszModifiedData[length] = L'\0';


                            EmptyClipboard();


                            HANDLE hNewData = GlobalAlloc(GMEM_MOVEABLE, (length + 1) * sizeof(WCHAR));
                            if (hNewData != NULL) {
                                WCHAR* pszNewData = static_cast<WCHAR*>(GlobalLock(hNewData));
                                if (pszNewData != NULL) {
                                    wcscpy_s(pszNewData, length + 1, pszModifiedData);
                                    GlobalUnlock(hNewData);


                                    SetClipboardData(CF_UNICODETEXT, hNewData);
                                }
                                else {
                                    GlobalFree(hNewData);
                                }
                            }

                            delete[] pszModifiedData;
                        }
                        GlobalUnlock(hClipboardData);
                    }
                }

                CloseClipboard();
            }
            SendMessageW(hEdit, WM_PASTE, 0, 0);
            break;

        case ID_CODE_DECODE:
            SaveCurrentStateForUndo();
            SendMessageW(hEdit, WM_CUT, 0, 0);
            if (OpenClipboard(hwnd)) {
                HANDLE hClipboardData = GetClipboardData(CF_UNICODETEXT);
                if (hClipboardData != NULL) {
                    WCHAR* pszData = static_cast<WCHAR*>(GlobalLock(hClipboardData));
                    if (pszData != NULL) {

                        size_t length = wcslen(pszData);
                        WCHAR* pszModifiedData = new WCHAR[length + 1];
                        if (pszModifiedData != NULL) {
                            for (size_t i = 0; i < length; ++i) {
                                if (pszData[i] == L'☉') {
                                    pszModifiedData[i] = L'a';
                                }
                                else if (pszData[i] == L'●') {
                                    pszModifiedData[i] = L'b';
                                }
                                else if (pszData[i] == L'☾') {
                                    pszModifiedData[i] = L'c';
                                }
                                else if (pszData[i] == L'☽') {
                                    pszModifiedData[i] = L'd';
                                }
                                else if (pszData[i] == L'○') {
                                    pszModifiedData[i] = L'e';
                                }
                                else if (pszData[i] == L'☿') {
                                    pszModifiedData[i] = L'f';
                                }
                                else if (pszData[i] == L'♀') {
                                    pszModifiedData[i] = L'g';
                                }
                                else if (pszData[i] == L'♁') {
                                    pszModifiedData[i] = L'h';
                                }
                                else if (pszData[i] == L'♂') {
                                    pszModifiedData[i] = L'i';
                                }
                                else if (pszData[i] == L'♃') {
                                    pszModifiedData[i] = L'j';
                                }
                                else if (pszData[i] == L'♄') {
                                    pszModifiedData[i] = L'k';
                                }
                                else if (pszData[i] == L'♅') {
                                    pszModifiedData[i] = L'l';
                                }
                                else if (pszData[i] == L'♆') {
                                    pszModifiedData[i] = L'm';
                                }
                                else if (pszData[i] == L'♇') {
                                    pszModifiedData[i] = L'n';
                                }
                                else if (pszData[i] == L'♈') {
                                    pszModifiedData[i] = L'o';
                                }
                                else if (pszData[i] == L'♉') {
                                    pszModifiedData[i] = L'p';
                                }
                                else if (pszData[i] == L'♊') {
                                    pszModifiedData[i] = L'q';
                                }
                                else if (pszData[i] == L'♋') {
                                    pszModifiedData[i] = L'r';
                                }
                                else if (pszData[i] == L'♌') {
                                    pszModifiedData[i] = L's';
                                }
                                else if (pszData[i] == L'♍') {
                                    pszModifiedData[i] = L't';
                                }
                                else if (pszData[i] == L'♎') {
                                    pszModifiedData[i] = L'u';
                                }
                                else if (pszData[i] == L'♏') {
                                    pszModifiedData[i] = L'v';
                                }
                                else if (pszData[i] == L'♐') {
                                    pszModifiedData[i] = L'w';
                                }
                                else if (pszData[i] == L'♑') {
                                    pszModifiedData[i] = L'x';
                                }
                                else if (pszData[i] == L'♒') {
                                    pszModifiedData[i] = L'y';
                                }
                                else if (pszData[i] == L'♓') {
                                    pszModifiedData[i] = L'z';
                                }

                                else {
                                    pszModifiedData[i] = pszData[i];
                                }
                            }
                            pszModifiedData[length] = L'\0';

                            EmptyClipboard();

                            HANDLE hNewData = GlobalAlloc(GMEM_MOVEABLE, (length + 1) * sizeof(WCHAR));
                            if (hNewData != NULL) {
                                WCHAR* pszNewData = static_cast<WCHAR*>(GlobalLock(hNewData));
                                if (pszNewData != NULL) {
                                    wcscpy_s(pszNewData, length + 1, pszModifiedData);
                                    GlobalUnlock(hNewData);


                                    SetClipboardData(CF_UNICODETEXT, hNewData);
                                }
                                else {
                                    GlobalFree(hNewData);
                                }
                            }

                            delete[] pszModifiedData;
                        }
                        GlobalUnlock(hClipboardData);
                    }
                }

                CloseClipboard();
            }
            SendMessageW(hEdit, WM_PASTE, 0, 0);
            break;

        case ID_EDIT_CUT: {
            SendMessageW(hEdit, WM_CUT, 0, 0);
            break;
        }
        case ID_EDIT_COPY: {
            SendMessageW(hEdit, WM_COPY, 0, 0);
            break;
        }
        case ID_EDIT_PASTE: {
            SendMessageW(hEdit, WM_PASTE, 0, 0);
            break;
        }
        case ID_EDIT_ALL: {
            SendMessageW(hEdit, EM_SETSEL, 0, -1);
            SendMessageW(hEdit, WM_COPY, 0, 0);
            break;
        }
        case ID_EDIT_UNDO: {
            Undo();
            break;
        }
        case ID_EDIT_REDO: {
            Redo();
            break;
        }
        case ID_EDIT_DELETE: {
            DWORD start, end;
            SendMessageW(hEdit, EM_GETSEL, reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&end));

            if (start != end) {
                SendMessageW(hEdit, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(TEXT("")));
            }
            else {
                SendMessageW(hEdit, EM_SETSEL, start, end + 1);
                SendMessageW(hEdit, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(TEXT("")));
            }
            break;
        }
        case ID_CTRL_D_DECODE: {
            SendMessageW(hwnd, WM_COMMAND, MAKEWPARAM(ID_CODE_DECODE, 0), 0);
            break;
        }
        case ID_CTRL_E_ENCODE: {
            SendMessageW(hwnd, WM_COMMAND, MAKEWPARAM(ID_CODE_ENCODE, 0), 0);
            break;
        }

        case ID_CTRL_T: {
            enableSubstitution = !enableSubstitution;
            UpdateStatusBar();
            break;
        }
        case ID_CTRL_A: {
            int textLength = GetWindowTextLength(hEdit);
            SendMessageW(hEdit, EM_SETSEL, 0, textLength);
            break;
        }
        case ID_CTRL_X: {
            SendMessageW(hEdit, WM_CUT, 0, 0);
            break;
        }
        case ID_CTRL_V: {
            SendMessageW(hEdit, WM_PASTE, 0, 0);
            break;
        }
        case ID_CTRL_C: {
            SendMessageW(hEdit, WM_COPY, 0, 0);
            break;
        }
        case ID_FILE_EXIT:
            DestroyWindow(hwnd);
            break;
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
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
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(iccex);
    iccex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&iccex);

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassW(&wc);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Set window dimensions
    int windowWidth = 480;
    int windowHeight = 640;

    // Calculate position to center the window
    int posX = (screenWidth - windowWidth) - 20;
    int posY = (screenHeight - windowHeight) / 2 - 20;

    // Create the window
    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        posX, posY,           // Use calculated position
        windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    HMENU hMenu = CreateMenu();
    HMENU hSubMenuFile = CreatePopupMenu();
    AppendMenuW(hSubMenuFile, MF_STRING, ID_FILE_OPEN, L"&Open  -  Ctrl + O");
    AppendMenuW(hSubMenuFile, MF_STRING, ID_FILE_SAVE, L"&Save  -  Ctrl + S");
    AppendMenuW(hSubMenuFile, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hSubMenuFile, MF_STRING, ID_FILE_EXIT, L"E&xit  -  Ctrl + Q");

    HMENU hSubMenuCode = CreatePopupMenu();
    AppendMenuW(hSubMenuCode, MF_STRING, ID_CODE_ENCODE, L"&Encode  -  Ctrl + E");
    AppendMenuW(hSubMenuCode, MF_STRING, ID_CODE_DECODE, L"&Decode  -  Ctrl + D");
    AppendMenuW(hSubMenuCode, MF_STRING, ID_TOGGLE_SUBSTITUTION, L"&Toggle  -  Ctrl + T");

    HMENU hSubMenuEdit = CreatePopupMenu();
    AppendMenuW(hSubMenuEdit, MF_STRING, ID_EDIT_UNDO, L"&Undo  -  Ctrl + Z");
    AppendMenuW(hSubMenuEdit, MF_STRING, ID_EDIT_REDO, L"&Redo  -  Ctrl + Y");
    AppendMenuW(hSubMenuEdit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hSubMenuEdit, MF_STRING, ID_EDIT_DELETE, L"&Delete  -  Del");
    AppendMenuW(hSubMenuEdit, MF_STRING, ID_EDIT_CUT, L"&Cut  -  Ctrl + X");
    AppendMenuW(hSubMenuEdit, MF_STRING, ID_EDIT_COPY, L"&Copy  -  Ctrl + C");
    AppendMenuW(hSubMenuEdit, MF_STRING, ID_EDIT_PASTE, L"&Paste - Ctrl + V");
    AppendMenuW(hSubMenuEdit, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hSubMenuEdit, MF_STRING, ID_EDIT_ALL, L"&Select All - Ctrl + A");

    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hSubMenuFile, L"&File");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hSubMenuEdit, L"&Edit");
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hSubMenuCode, L"&Code");


    SetMenu(hwnd, hMenu);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
