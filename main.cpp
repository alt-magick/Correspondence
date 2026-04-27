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

bool HasSelection(HWND hEdit, DWORD& start, DWORD& end)
{
    SendMessageW(hEdit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
    return start != end;
}

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

std::wstring GetAllText(HWND hEdit)
{
    int len = GetWindowTextLengthW(hEdit);
    std::wstring text(len, L'\0');
    GetWindowTextW(hEdit, &text[0], len + 1);
    return text;
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
    DWORD start = 0, end = 0;
    bool hasSelection = HasSelection(hEdit, start, end);

    std::wstring text = GetAllText(hEdit);

    if (!hasSelection) {
        start = 0;
        end = (DWORD)text.size();
    }

    for (DWORD i = start; i < end; ++i) {
        switch (text[i]) {
        case L'A': case L'a': text[i] = L'☉'; break;
        case L'B': case L'b': text[i] = L'●'; break;
        case L'C': case L'c': text[i] = L'☾'; break;
        case L'D': case L'd': text[i] = L'☽'; break;
        case L'E': case L'e': text[i] = L'○'; break;
        case L'F': case L'f': text[i] = L'☿'; break;
        case L'G': case L'g': text[i] = L'♀'; break;
        case L'H': case L'h': text[i] = L'♁'; break;
        case L'I': case L'i': text[i] = L'♂'; break;
        case L'J': case L'j': text[i] = L'♃'; break;
        case L'K': case L'k': text[i] = L'♄'; break;
        case L'L': case L'l': text[i] = L'♅'; break;
        case L'M': case L'm': text[i] = L'♆'; break;
        case L'N': case L'n': text[i] = L'♇'; break;
        case L'O': case L'o': text[i] = L'♈'; break;
        case L'P': case L'p': text[i] = L'♉'; break;
        case L'Q': case L'q': text[i] = L'♊'; break;
        case L'R': case L'r': text[i] = L'♋'; break;
        case L'S': case L's': text[i] = L'♌'; break;
        case L'T': case L't': text[i] = L'♍'; break;
        case L'U': case L'u': text[i] = L'♎'; break;
        case L'V': case L'v': text[i] = L'♏'; break;
        case L'W': case L'w': text[i] = L'♐'; break;
        case L'X': case L'x': text[i] = L'♑'; break;
        case L'Y': case L'y': text[i] = L'♒'; break;
        case L'Z': case L'z': text[i] = L'♓'; break;
        }
    }

    SetWindowTextW(hEdit, text.c_str());
    UpdateStatusBar();
}

void DecodeText() {
    DWORD start = 0, end = 0;
    bool hasSelection = HasSelection(hEdit, start, end);

    std::wstring text = GetAllText(hEdit);

    if (!hasSelection) {
        start = 0;
        end = (DWORD)text.size();
    }

    for (DWORD i = start; i < end; ++i) {
        switch (text[i]) {
        case L'☉': text[i] = L'a'; break;
        case L'●': text[i] = L'b'; break;
        case L'☾': text[i] = L'c'; break;
        case L'☽': text[i] = L'd'; break;
        case L'○': text[i] = L'e'; break;
        case L'☿': text[i] = L'f'; break;
        case L'♀': text[i] = L'g'; break;
        case L'♁': text[i] = L'h'; break;
        case L'♂': text[i] = L'i'; break;
        case L'♃': text[i] = L'j'; break;
        case L'♄': text[i] = L'k'; break;
        case L'♅': text[i] = L'l'; break;
        case L'♆': text[i] = L'm'; break;
        case L'♇': text[i] = L'n'; break;
        case L'♈': text[i] = L'o'; break;
        case L'♉': text[i] = L'p'; break;
        case L'♊': text[i] = L'q'; break;
        case L'♋': text[i] = L'r'; break;
        case L'♌': text[i] = L's'; break;
        case L'♍': text[i] = L't'; break;
        case L'♎': text[i] = L'u'; break;
        case L'♏': text[i] = L'v'; break;
        case L'♐': text[i] = L'w'; break;
        case L'♑': text[i] = L'x'; break;
        case L'♒': text[i] = L'y'; break;
        case L'♓': text[i] = L'z'; break;
        }
    }

    SetWindowTextW(hEdit, text.c_str());
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

case ID_CODE_ENCODE: {

    EncodeText();
    break;
}

case ID_CODE_DECODE: {

    DecodeText();
    break;
}
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
