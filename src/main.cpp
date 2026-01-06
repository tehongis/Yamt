// main.cpp
// Win32 app to inject keyboard input at a user-selected interval (1-60s) with jitter (100-900ms).
// Supports tokens like {F10} (default), {ENTER}, {TAB}, arrow keys, etc.
// Compile with Visual Studio (cl), link comctl32.lib

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <cctype>

#pragma comment(lib, "comctl32.lib")

// Control IDs
#define IDC_EDIT_TEXT     101
#define IDC_BTN_INJECT    102
#define IDC_BTN_START     103
#define IDC_BTN_STOP      104
#define IDC_TRACK_SLIDER  105
#define IDC_LABEL_SEC     106
#define IDC_TRACK_VAR     107
#define IDC_LABEL_VAR     108

// Timer ID
#define TIMER_INJECT 1

const wchar_t CLASS_NAME[] = L"YamtWindowClass";

HWND hEdit = NULL;
HWND hBtnInject = NULL;
HWND hBtnStart = NULL;
HWND hBtnStop = NULL;
HWND hSlider = NULL;
HWND hLabel = NULL;
HWND hSliderVar = NULL;
HWND hLabelVar = NULL;

std::mt19937_64 rng;

int GetBaseSeconds() {
    return (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);
}
int GetVariationMs() {
    return (int)SendMessage(hSliderVar, TBM_GETPOS, 0, 0);
}

void UpdateLabelFromSlider() {
    int pos = GetBaseSeconds();
    wchar_t buf[64];
    wsprintf(buf, L"Interval: %d second%s", pos, pos == 1 ? L"" : L"s");
    SetWindowTextW(hLabel, buf);
}

void UpdateLabelFromVarSlider() {
    int pos = GetVariationMs();
    wchar_t buf[64];
    wsprintf(buf, L"Variation: \xB1 %d ms", pos); // ± symbol
    SetWindowTextW(hLabelVar, buf);
}

std::wstring GetEditText() {
    int len = GetWindowTextLengthW(hEdit);
    if (len <= 0) return std::wstring();
    std::wstring txt;
    txt.resize(len + 1);
    int copied = GetWindowTextW(hEdit, &txt[0], len + 1);
    if (copied < 0) copied = 0;
    txt.resize(copied);
    return txt;
}

// Send a sequence of INPUT events
void SendInputs(const std::vector<INPUT>& inputs) {
    if (inputs.empty()) return;
    // SendInput requires non-const pointer
    std::vector<INPUT> tmp = inputs;
    SendInput((UINT)tmp.size(), tmp.data(), sizeof(INPUT));
}

// Helper to append a virtual-key press (down + up)
void AppendVirtualKey(std::vector<INPUT>& out, WORD vk) {
    INPUT down = {};
    down.type = INPUT_KEYBOARD;
    down.ki.wVk = vk;
    down.ki.dwFlags = 0;
    INPUT up = down;
    up.ki.dwFlags = KEYEVENTF_KEYUP;
    out.push_back(down);
    out.push_back(up);
}

// Helper to append a Unicode character (down + up using KEYEVENTF_UNICODE)
void AppendUnicodeChar(std::vector<INPUT>& out, WCHAR ch) {
    INPUT down = {};
    down.type = INPUT_KEYBOARD;
    down.ki.wScan = ch;
    down.ki.dwFlags = KEYEVENTF_UNICODE;
    INPUT up = down;
    up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    out.push_back(down);
    out.push_back(up);
}

// Parse tokens like {F10}, {ENTER}, etc., and build INPUT sequence.
// Supports mixing plain text and tokens, e.g. "abc{ENTER}{F10}def".
void SendParsedInput(const std::wstring& text) {
    if (text.empty()) return;
    std::vector<INPUT> inputs;
    const size_t n = text.size();
    for (size_t i = 0; i < n; ) {
        if (text[i] == L'{') {
            // try to parse token until next '}'
            size_t j = i + 1;
            while (j < n && text[j] != L'}') ++j;
            if (j < n && text[j] == L'}') {
                // token found
                std::wstring token = text.substr(i + 1, j - (i + 1));
                // uppercase trim
                token.erase(token.begin(), std::find_if(token.begin(), token.end(), [](wchar_t c){ return !iswspace(c); }));
                token.erase(std::find_if(token.rbegin(), token.rend(), [](wchar_t c){ return !iswspace(c); }).base(), token.end());
                std::transform(token.begin(), token.end(), token.begin(), towupper);

                bool handled = false;
                // F keys: F1..F24
                if (!token.empty() && token[0] == L'F') {
                    // parse number after F
                    int num = 0;
                    try {
                        num = std::stoi(std::wstring(token.begin() + 1, token.end()));
                    } catch (...) { num = 0; }
                    if (num >= 1 && num <= 24) {
                        WORD vk = (WORD)(VK_F1 + (num - 1)); // VK_F1..VK_F24
                        AppendVirtualKey(inputs, vk);
                        handled = true;
                    }
                }
                if (!handled) {
                    if (token == L"ENTER" || token == L"RETURN") {
                        AppendVirtualKey(inputs, VK_RETURN);
                        handled = true;
                    } else if (token == L"TAB") {
                        AppendVirtualKey(inputs, VK_TAB);
                        handled = true;
                    } else if (token == L"ESC" || token == L"ESCAPE") {
                        AppendVirtualKey(inputs, VK_ESCAPE);
                        handled = true;
                    } else if (token == L"SPACE" || token == L" ") {
                        AppendVirtualKey(inputs, VK_SPACE);
                        handled = true;
                    } else if (token == L"BACKSPACE" || token == L"BS") {
                        AppendVirtualKey(inputs, VK_BACK);
                        handled = true;
                    } else if (token == L"UP") {
                        AppendVirtualKey(inputs, VK_UP);
                        handled = true;
                    } else if (token == L"DOWN") {
                        AppendVirtualKey(inputs, VK_DOWN);
                        handled = true;
                    } else if (token == L"LEFT") {
                        AppendVirtualKey(inputs, VK_LEFT);
                        handled = true;
                    } else if (token == L"RIGHT") {
                        AppendVirtualKey(inputs, VK_RIGHT);
                        handled = true;
                    }
                    // add more tokens here if desired
                }

                if (handled) {
                    i = j + 1;
                    continue;
                } else {
                    // treat the '{' as a literal if token not recognized
                    AppendUnicodeChar(inputs, L'{');
                    i = i + 1;
                    continue;
                }
            } else {
                // no closing brace, treat as literal '{'
                AppendUnicodeChar(inputs, L'{');
                i = i + 1;
                continue;
            }
        } else {
            // normal character: append as Unicode char
            AppendUnicodeChar(inputs, text[i]);
            ++i;
        }
    }

    SendInputs(inputs);
}

UINT ComputeNextIntervalMs() {
    int sec = GetBaseSeconds();
    if (sec < 1) sec = 1;
    int var = GetVariationMs();
    std::uniform_int_distribution<int> dist(-var, var);
    int offset = dist(rng);
    int ms = sec * 1000 + offset;
    if (ms < 1) ms = 1;
    return (UINT)ms;
}

void StartInjectionTimer(HWND hWnd) {
    // Single-shot behavior: compute first interval, set timer once.
    UINT ms = ComputeNextIntervalMs();
    SetTimer(hWnd, TIMER_INJECT, ms, NULL);
    EnableWindow(hBtnStart, FALSE);
    EnableWindow(hBtnStop, TRUE);
}

void StopInjectionTimer(HWND hWnd) {
    KillTimer(hWnd, TIMER_INJECT);
    EnableWindow(hBtnStart, TRUE);
    EnableWindow(hBtnStop, FALSE);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        // Initialize common controls for trackbars
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(icex);
        icex.dwICC = ICC_BAR_CLASSES;
        InitCommonControlsEx(&icex);

        // Edit with default token {F10}
        hEdit = CreateWindowExW(0, L"EDIT", L"{F10}",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            10, 10, 560, 24, hWnd, (HMENU)IDC_EDIT_TEXT, GetModuleHandle(NULL), NULL);

        // Inject Now
        hBtnInject = CreateWindowW(L"BUTTON", L"Inject Now",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            580, 10, 90, 24, hWnd, (HMENU)IDC_BTN_INJECT, GetModuleHandle(NULL), NULL);

        // Base interval slider
        hSlider = CreateWindowExW(0, TRACKBAR_CLASS, NULL,
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
            10, 50, 660, 40, hWnd, (HMENU)IDC_TRACK_SLIDER, GetModuleHandle(NULL), NULL);
        SendMessage(hSlider, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(1, 60));
        SendMessage(hSlider, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)5); // default 5s
        SendMessage(hSlider, TBM_SETTICFREQ, 5, 0);

        hLabel = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
            10, 90, 300, 20, hWnd, (HMENU)IDC_LABEL_SEC, GetModuleHandle(NULL), NULL);

        // Variation slider (100..900 ms)
        hSliderVar = CreateWindowExW(0, TRACKBAR_CLASS, NULL,
            WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
            10, 115, 660, 40, hWnd, (HMENU)IDC_TRACK_VAR, GetModuleHandle(NULL), NULL);
        SendMessage(hSliderVar, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(100, 900));
        SendMessage(hSliderVar, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)200); // default 200 ms
        SendMessage(hSliderVar, TBM_SETTICFREQ, 100, 0);

        hLabelVar = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
            10, 155, 300, 20, hWnd, (HMENU)IDC_LABEL_VAR, GetModuleHandle(NULL), NULL);

        // Start / Stop buttons
        hBtnStart = CreateWindowW(L"BUTTON", L"Start",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD,
            10, 185, 90, 28, hWnd, (HMENU)IDC_BTN_START, GetModuleHandle(NULL), NULL);

        hBtnStop = CreateWindowW(L"BUTTON", L"Stop",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD,
            110, 185, 90, 28, hWnd, (HMENU)IDC_BTN_STOP, GetModuleHandle(NULL), NULL);

        UpdateLabelFromSlider();
        UpdateLabelFromVarSlider();

        EnableWindow(hBtnStop, FALSE);

        break;
    }
    case WM_HSCROLL: {
        HWND ctrl = (HWND)lParam;
        if (ctrl == hSlider) {
            UpdateLabelFromSlider();
        } else if (ctrl == hSliderVar) {
            UpdateLabelFromVarSlider();
        }
        break;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDC_BTN_INJECT: {
            std::wstring txt = GetEditText();
            SendParsedInput(txt);
            break;
        }
        case IDC_BTN_START: {
            StartInjectionTimer(hWnd);
            break;
        }
        case IDC_BTN_STOP: {
            StopInjectionTimer(hWnd);
            break;
        }
        default:
            break;
        }
        break;
    }
    case WM_TIMER: {
        if (wParam == TIMER_INJECT) {
            // Single-shot behavior: kill current timer, inject, then schedule next with variation
            KillTimer(hWnd, TIMER_INJECT);

            std::wstring txt = GetEditText();
            if (!txt.empty()) {
                SendParsedInput(txt);
            }

            // Schedule next
            UINT ms = ComputeNextIntervalMs();
            SetTimer(hWnd, TIMER_INJECT, ms, NULL);
        }
        break;
    }
    case WM_DESTROY:
        KillTimer(hWnd, TIMER_INJECT);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    // Seed RNG
    rng.seed((uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count());

    WNDCLASSW wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Yamt - Keyboard Input Injector (with jitter & tokens)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 720, 260,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hWnd) return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}