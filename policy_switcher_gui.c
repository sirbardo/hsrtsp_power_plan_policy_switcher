/*
 * Short Thread Policy Switcher - GUI Version
 * Controls the heterogeneous short running thread scheduling policy
 * Uses Windows Power Management API directly (no powercfg dependency)
 *
 * Subgroup GUID: 54533251-82be-4824-96c1-47b60b740d00 (Processor power management)
 * Setting GUID:  bae08b81-2d5e-4688-ad6a-13243356654b (Short thread scheduling policy)
 *
 * Values:
 *   0 - All processors
 *   1 - Performant processors
 *   2 - Prefer performant processors
 *   3 - Efficient processors
 *   4 - Prefer efficient processors
 *   5 - Automatic
 */

#include <windows.h>
#include <powrprof.h>
#include <stdio.h>

#ifdef _MSC_VER
#pragma comment(lib, "PowrProf.lib")
#endif

#define IDC_RADIO_BASE 1000
#define IDC_REFRESH_BTN 2000
#define IDC_APPLY_BTN 2001
#define NUM_POLICIES 6
#define TIMER_ID 1
#define HOTKEY_ALL 1
#define HOTKEY_PERF 2

// Power setting GUIDs
static const GUID GUID_PROCESSOR_SUBGROUP = {
    0x54533251, 0x82be, 0x4824,
    {0x96, 0xc1, 0x47, 0xb6, 0x0b, 0x74, 0x0d, 0x00}
};

static const GUID GUID_SHORT_THREAD_POLICY = {
    0xbae08b81, 0x2d5e, 0x4688,
    {0xad, 0x6a, 0x13, 0x24, 0x33, 0x56, 0x65, 0x4b}
};

static const char* POLICY_NAMES[] = {
    "All processors",
    "Performant processors",
    "Prefer performant processors",
    "Efficient processors",
    "Prefer efficient processors",
    "Automatic"
};

static HWND hRadioButtons[NUM_POLICIES];
static HWND hRefreshBtn;
static HWND hApplyBtn;
static HWND hStatusLabel;
static int current_policy = 0;
static DWORD last_error = ERROR_SUCCESS;

// Forward declarations
void update_ui_from_policy(HWND hwnd);
void play_beep(int policy_index);

int get_current_policy() {
    GUID *pActiveScheme = NULL;
    DWORD value = 0;
    int result = -1;

    DWORD status = PowerGetActiveScheme(NULL, &pActiveScheme);
    if (status == ERROR_SUCCESS) {
        status = PowerReadACValueIndex(NULL, pActiveScheme,
            &GUID_PROCESSOR_SUBGROUP, &GUID_SHORT_THREAD_POLICY, &value);

        if (status == ERROR_SUCCESS) {
            result = (int)value;
        }
        last_error = status;
        LocalFree(pActiveScheme);
    } else {
        last_error = status;
    }

    return result;
}

int set_policy(int policy_index) {
    GUID *pActiveScheme = NULL;
    int success = 0;

    DWORD status = PowerGetActiveScheme(NULL, &pActiveScheme);
    if (status == ERROR_SUCCESS) {
        // Set AC value (plugged in)
        DWORD result1 = PowerWriteACValueIndex(NULL, pActiveScheme,
            &GUID_PROCESSOR_SUBGROUP, &GUID_SHORT_THREAD_POLICY, (DWORD)policy_index);

        // Set DC value (battery)
        DWORD result2 = PowerWriteDCValueIndex(NULL, pActiveScheme,
            &GUID_PROCESSOR_SUBGROUP, &GUID_SHORT_THREAD_POLICY, (DWORD)policy_index);

        // Apply changes
        DWORD result3 = PowerSetActiveScheme(NULL, pActiveScheme);

        success = (result1 == ERROR_SUCCESS &&
                   result2 == ERROR_SUCCESS &&
                   result3 == ERROR_SUCCESS);

        if (!success) {
            last_error = result1 != ERROR_SUCCESS ? result1 :
                        result2 != ERROR_SUCCESS ? result2 : result3;
        }

        LocalFree(pActiveScheme);
    } else {
        last_error = status;
    }

    return success;
}

void play_beep(int policy_index) {
    int freq = 400 + (policy_index * 100);  // 400Hz to 900Hz
    Beep(freq, 100);
}

void cycle_policy_all(HWND hwnd) {
    int current = get_current_policy();
    if (current >= 0 && current < NUM_POLICIES) {
        current_policy = current;
    }

    int next_policy = (current_policy + 1) % NUM_POLICIES;

    if (set_policy(next_policy)) {
        current_policy = next_policy;
        play_beep(next_policy);
    } else {
        Beep(200, 200);  // Error beep
    }

    if (hwnd) {
        update_ui_from_policy(hwnd);
    }
}

void cycle_policy_perf(HWND hwnd) {
    int current = get_current_policy();

    // Cycle between 0, 1, 2 (All, Performant, Prefer performant)
    int next_policy;
    if (current == 0) next_policy = 1;
    else if (current == 1) next_policy = 2;
    else next_policy = 0;

    if (set_policy(next_policy)) {
        current_policy = next_policy;
        play_beep(next_policy);
    } else {
        Beep(200, 200);  // Error beep
    }

    if (hwnd) {
        update_ui_from_policy(hwnd);
    }
}

void update_ui_from_policy(HWND hwnd) {
    current_policy = get_current_policy();

    if (current_policy >= 0 && current_policy < NUM_POLICIES) {
        // Update radio buttons
        for (int i = 0; i < NUM_POLICIES; i++) {
            SendMessage(hRadioButtons[i], BM_SETCHECK,
                       (i == current_policy) ? BST_CHECKED : BST_UNCHECKED, 0);
        }

        char status[256];
        snprintf(status, sizeof(status), "Current: %s", POLICY_NAMES[current_policy]);
        SetWindowText(hStatusLabel, status);
    } else {
        // Error reading policy
        char status[256];
        if (last_error == ERROR_FILE_NOT_FOUND) {
            snprintf(status, sizeof(status),
                "ERROR: Setting not found.\n"
                "Requires Windows with heterogeneous thread scheduling support.");
        } else if (last_error == ERROR_ACCESS_DENIED) {
            snprintf(status, sizeof(status),
                "ERROR: Access denied.\n"
                "Please run as Administrator.");
        } else {
            snprintf(status, sizeof(status),
                "ERROR: Failed to read policy (code: %lu)", last_error);
        }
        SetWindowText(hStatusLabel, status);

        SendMessage(hRadioButtons[0], BM_SETCHECK, BST_CHECKED, 0);
        for (int i = 1; i < NUM_POLICIES; i++) {
            SendMessage(hRadioButtons[i], BM_SETCHECK, BST_UNCHECKED, 0);
        }
    }
}

void apply_selected_policy(HWND hwnd) {
    KillTimer(hwnd, TIMER_ID);

    int selected = -1;
    for (int i = 0; i < NUM_POLICIES; i++) {
        if (SendMessage(hRadioButtons[i], BM_GETCHECK, 0, 0) == BST_CHECKED) {
            selected = i;
            break;
        }
    }

    if (selected >= 0) {
        SetWindowText(hStatusLabel, "Applying...");

        if (set_policy(selected)) {
            current_policy = selected;
            play_beep(selected);
        } else {
            char status[256];
            if (last_error == ERROR_ACCESS_DENIED) {
                snprintf(status, sizeof(status),
                    "ERROR: Access denied. Run as Administrator.");
            } else {
                snprintf(status, sizeof(status),
                    "ERROR: Failed to apply (code: %lu)", last_error);
            }
            SetWindowText(hStatusLabel, status);
            Beep(200, 200);
        }

        Sleep(300);
        update_ui_from_policy(hwnd);
    }

    SetTimer(hwnd, TIMER_ID, 2000, NULL);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            CreateWindow("STATIC", "Short Thread Scheduling Policy:",
                        WS_VISIBLE | WS_CHILD,
                        20, 10, 300, 20,
                        hwnd, NULL, NULL, NULL);

            int y_pos = 40;
            for (int i = 0; i < NUM_POLICIES; i++) {
                char label[256];
                snprintf(label, sizeof(label), "%d - %s", i, POLICY_NAMES[i]);

                DWORD style = WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON;
                if (i == 0) style |= WS_GROUP;

                hRadioButtons[i] = CreateWindow("BUTTON", label,
                    style, 30, y_pos, 350, 25,
                    hwnd, (HMENU)(LONG_PTR)(IDC_RADIO_BASE + i), NULL, NULL);

                y_pos += 30;
            }

            hRefreshBtn = CreateWindow("BUTTON", "Refresh",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                30, y_pos + 10, 100, 30,
                hwnd, (HMENU)IDC_REFRESH_BTN, NULL, NULL);

            hApplyBtn = CreateWindow("BUTTON", "Apply",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                140, y_pos + 10, 100, 30,
                hwnd, (HMENU)IDC_APPLY_BTN, NULL, NULL);

            hStatusLabel = CreateWindow("STATIC", "Initializing...",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                30, y_pos + 50, 420, 60,
                hwnd, NULL, NULL, NULL);

            CreateWindow("STATIC", "Hotkeys: ALT+X (cycle all)  |  ALT+V (cycle performance)",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                30, y_pos + 120, 420, 20,
                hwnd, NULL, NULL, NULL);

            update_ui_from_policy(hwnd);

            SetTimer(hwnd, TIMER_ID, 2000, NULL);

            if (!RegisterHotKey(hwnd, HOTKEY_ALL, MOD_ALT, 'X')) {
                MessageBox(hwnd, "Failed to register ALT+X hotkey.",
                          "Warning", MB_OK | MB_ICONWARNING);
            }
            if (!RegisterHotKey(hwnd, HOTKEY_PERF, MOD_ALT, 'V')) {
                MessageBox(hwnd, "Failed to register ALT+V hotkey.",
                          "Warning", MB_OK | MB_ICONWARNING);
            }

            return 0;
        }

        case WM_TIMER:
            if (wParam == TIMER_ID) {
                update_ui_from_policy(hwnd);
            }
            return 0;

        case WM_HOTKEY:
            if (wParam == HOTKEY_ALL) {
                cycle_policy_all(hwnd);
            } else if (wParam == HOTKEY_PERF) {
                cycle_policy_perf(hwnd);
            }
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_REFRESH_BTN) {
                update_ui_from_policy(hwnd);
            } else if (LOWORD(wParam) == IDC_APPLY_BTN) {
                apply_selected_policy(hwnd);
            }
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, TIMER_ID);
            UnregisterHotKey(hwnd, HOTKEY_ALL);
            UnregisterHotKey(hwnd, HOTKEY_PERF);
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    const char CLASS_NAME[] = "PolicySwitcherWindow";

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, "Short Thread Policy Switcher",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
