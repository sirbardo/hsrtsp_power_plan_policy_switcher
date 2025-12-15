/*
 * Short Thread Policy Switcher - GUI Version
 * Shows current heterogeneous short running thread scheduling policy with radio buttons
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
#include <stdio.h>
#include <stdlib.h>

#define IDC_RADIO_BASE 1000
#define IDC_REFRESH_BTN 2000
#define IDC_APPLY_BTN 2001
#define NUM_POLICIES 6
#define WM_REFRESH_TIMER (WM_USER + 1)
#define TIMER_ID 1
#define HOTKEY_ALL 1
#define HOTKEY_PERF 2

const char* SUBGROUP_GUID = "54533251-82be-4824-96c1-47b60b740d00";
const char* SETTING_GUID = "bae08b81-2d5e-4688-ad6a-13243356654b";

const char* POLICY_NAMES[] = {
    "All processors",
    "Performant processors",
    "Prefer performant processors",
    "Efficient processors",
    "Prefer efficient processors",
    "Automatic"
};

HWND hRadioButtons[NUM_POLICIES];
HWND hRefreshBtn;
HWND hApplyBtn;
HWND hStatusLabel;
int current_policy = 0;

// Forward declarations
void update_ui_from_policy(HWND hwnd);
void play_beep(int policy_index);

int run_hidden_command(const char* command, char* output, int output_size) {
    SECURITY_ATTRIBUTES sa = {0};
    HANDLE hReadPipe, hWritePipe;
    PROCESS_INFORMATION pi = {0};
    STARTUPINFO si = {0};
    DWORD bytesRead;

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        return 0;
    }

    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    char cmdline[1024];
    snprintf(cmdline, sizeof(cmdline), "cmd.exe /c %s", command);

    if (!CreateProcess(NULL, cmdline, NULL, NULL, TRUE, CREATE_NO_WINDOW,
                       NULL, NULL, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return 0;
    }

    CloseHandle(hWritePipe);

    if (output && output_size > 0) {
        output[0] = '\0';
        DWORD totalRead = 0;
        DWORD bytesAvailable;

        // Read all output in a loop
        while (totalRead < (DWORD)(output_size - 1)) {
            // Check if there's data available
            if (!PeekNamedPipe(hReadPipe, NULL, 0, NULL, &bytesAvailable, NULL)) {
                break;
            }

            if (bytesAvailable == 0) {
                // Wait a bit for more data
                if (WaitForSingleObject(pi.hProcess, 100) == WAIT_OBJECT_0) {
                    // Process finished, try one more read
                    ReadFile(hReadPipe, output + totalRead, output_size - 1 - totalRead, &bytesRead, NULL);
                    totalRead += bytesRead;
                    break;
                }
                continue;
            }

            if (!ReadFile(hReadPipe, output + totalRead, output_size - 1 - totalRead, &bytesRead, NULL) || bytesRead == 0) {
                break;
            }

            totalRead += bytesRead;
        }

        output[totalRead] = '\0';
    }

    WaitForSingleObject(pi.hProcess, 5000);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hReadPipe);

    return 1;
}

int get_current_policy() {
    char command[512];
    char buffer[4096];
    int value = -1;

    snprintf(command, sizeof(command),
        "powercfg /query SCHEME_CURRENT %s %s",
        SUBGROUP_GUID, SETTING_GUID);

    if (run_hidden_command(command, buffer, sizeof(buffer))) {
        char* line = buffer;
        char* next;

        while (line && *line) {
            next = strchr(line, '\n');
            if (next) *next = '\0';

            if (strstr(line, "Current AC Power Setting Index:")) {
                char* hex = strstr(line, "0x");
                if (hex) {
                    value = (int)strtol(hex, NULL, 16);
                }
                break;
            }

            line = next ? next + 1 : NULL;
        }
    }

    return value;
}

void set_policy(int policy_index) {
    char command[512];

    // Set AC value
    snprintf(command, sizeof(command),
        "powercfg /setacvalueindex SCHEME_CURRENT %s %s %d",
        SUBGROUP_GUID, SETTING_GUID, policy_index);
    run_hidden_command(command, NULL, 0);

    // Set DC value (battery)
    snprintf(command, sizeof(command),
        "powercfg /setdcvalueindex SCHEME_CURRENT %s %s %d",
        SUBGROUP_GUID, SETTING_GUID, policy_index);
    run_hidden_command(command, NULL, 0);

    // Apply changes
    run_hidden_command("powercfg /setactive SCHEME_CURRENT", NULL, 0);
}

void unhide_setting() {
    char command[512];
    // Unhide the setting so it can be queried and modified
    snprintf(command, sizeof(command),
        "powercfg -attributes %s %s -ATTRIB_HIDE",
        SUBGROUP_GUID, SETTING_GUID);
    run_hidden_command(command, NULL, 0);
}

void play_beep(int policy_index) {
    // Different beep patterns for different policies
    // Higher frequency = higher policy number
    int freq = 400 + (policy_index * 100);  // 400Hz to 900Hz
    Beep(freq, 100);  // 100ms beep
}

void cycle_policy_all(HWND hwnd) {
    // Get current policy
    int current = get_current_policy();
    if (current >= 0 && current < NUM_POLICIES) {
        current_policy = current;
    }

    // Cycle to next (all 6 policies)
    int next_policy = (current_policy + 1) % NUM_POLICIES;

    // Apply new policy
    set_policy(next_policy);
    current_policy = next_policy;

    // Beep for feedback
    play_beep(next_policy);

    // Update UI if window is valid
    if (hwnd) {
        update_ui_from_policy(hwnd);
    }
}

void cycle_policy_perf(HWND hwnd) {
    // Get current policy
    int current = get_current_policy();

    // Cycle between 0, 1, 2 (All, Performant, Prefer performant)
    int next_policy;
    if (current == 0) next_policy = 1;
    else if (current == 1) next_policy = 2;
    else next_policy = 0;

    // Apply new policy
    set_policy(next_policy);
    current_policy = next_policy;

    // Beep for feedback
    play_beep(next_policy);

    // Update UI if window is valid
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

        // Update status
        char status[256];
        snprintf(status, sizeof(status), "Current: %s", POLICY_NAMES[current_policy]);
        SetWindowText(hStatusLabel, status);
    } else {
        // Debug: show the actual output
        char command[512];
        char debug_output[2048];
        snprintf(command, sizeof(command),
            "powercfg /query SCHEME_CURRENT %s %s 2>&1",
            SUBGROUP_GUID, SETTING_GUID);

        if (run_hidden_command(command, debug_output, sizeof(debug_output))) {
            // Show more of the output for debugging
            char status[1024];
            debug_output[500] = '\0'; // Show more
            // Replace newlines with spaces for display
            for (char* p = debug_output; *p; p++) {
                if (*p == '\n' || *p == '\r') *p = ' ';
            }
            snprintf(status, sizeof(status), "ERROR - Output: %.500s", debug_output);
            SetWindowText(hStatusLabel, status);
        } else {
            SetWindowText(hStatusLabel, "Error: CreateProcess failed");
        }

        // Default to first option if we can't read
        SendMessage(hRadioButtons[0], BM_SETCHECK, BST_CHECKED, 0);
    }
}

void apply_selected_policy(HWND hwnd) {
    // Disable timer to prevent interference
    KillTimer(hwnd, TIMER_ID);

    // Find which radio button is selected
    int selected = -1;
    for (int i = 0; i < NUM_POLICIES; i++) {
        if (SendMessage(hRadioButtons[i], BM_GETCHECK, 0, 0) == BST_CHECKED) {
            selected = i;
            break;
        }
    }

    if (selected >= 0) {
        SetWindowText(hStatusLabel, "Applying...");

        set_policy(selected);
        current_policy = selected;
        play_beep(selected);

        Sleep(500);
        update_ui_from_policy(hwnd);
    }

    SetTimer(hwnd, TIMER_ID, 2000, NULL);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Create title label
            CreateWindow("STATIC", "Short Thread Scheduling Policy:",
                        WS_VISIBLE | WS_CHILD,
                        20, 10, 300, 20,
                        hwnd, NULL, NULL, NULL);

            // Create radio buttons
            int y_pos = 40;
            for (int i = 0; i < NUM_POLICIES; i++) {
                char label[256];
                snprintf(label, sizeof(label), "%d - %s", i, POLICY_NAMES[i]);

                // Only first radio button gets WS_GROUP to create a group
                DWORD style = WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON;
                if (i == 0) {
                    style |= WS_GROUP;
                }

                hRadioButtons[i] = CreateWindow("BUTTON", label,
                    style,
                    30, y_pos, 350, 25,
                    hwnd, (HMENU)(LONG_PTR)(IDC_RADIO_BASE + i), NULL, NULL);

                y_pos += 30;
            }

            // Create buttons
            hRefreshBtn = CreateWindow("BUTTON", "Refresh",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                30, y_pos + 10, 100, 30,
                hwnd, (HMENU)IDC_REFRESH_BTN, NULL, NULL);

            hApplyBtn = CreateWindow("BUTTON", "Apply",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                140, y_pos + 10, 100, 30,
                hwnd, (HMENU)IDC_APPLY_BTN, NULL, NULL);

            // Create status label - make it bigger for debug output
            hStatusLabel = CreateWindow("STATIC", "Initializing...",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                30, y_pos + 50, 420, 60,
                hwnd, NULL, NULL, NULL);

            // Create hotkey info label
            CreateWindow("STATIC", "Hotkeys: ALT+X (cycle all)  |  ALT+Z (cycle performance)",
                WS_VISIBLE | WS_CHILD | SS_LEFT,
                30, y_pos + 120, 420, 20,
                hwnd, NULL, NULL, NULL);

            // Unhide the setting first (needs admin)
            unhide_setting();

            // Initial update
            update_ui_from_policy(hwnd);

            // Set timer for auto-refresh every 2 seconds
            SetTimer(hwnd, TIMER_ID, 2000, NULL);

            // Register global hotkeys
            if (!RegisterHotKey(hwnd, HOTKEY_ALL, MOD_ALT, 'X')) {
                MessageBox(hwnd, "Failed to register ALT+X hotkey.\nAnother application may be using it.",
                          "Hotkey Registration Failed", MB_OK | MB_ICONWARNING);
            }
            if (!RegisterHotKey(hwnd, HOTKEY_PERF, MOD_ALT, 'Z')) {
                MessageBox(hwnd, "Failed to register ALT+Z hotkey.\nAnother application may be using it.",
                          "Hotkey Registration Failed", MB_OK | MB_ICONWARNING);
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
            }
            else if (LOWORD(wParam) == IDC_APPLY_BTN) {
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
    const char CLASS_NAME[] = "PolicySwitcherWindow";

    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "Short Thread Policy Switcher",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 500, 400,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
