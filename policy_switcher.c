/*
 * Short Thread Policy Switcher - Console Version
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

#define HOTKEY_ALL 1
#define HOTKEY_PERF 2
#define NUM_POLICIES 6

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

static int current_policy = 0;

int get_current_policy() {
    GUID *pActiveScheme = NULL;
    DWORD value = 0;
    int result = -1;

    if (PowerGetActiveScheme(NULL, &pActiveScheme) == ERROR_SUCCESS) {
        if (PowerReadACValueIndex(NULL, pActiveScheme,
                &GUID_PROCESSOR_SUBGROUP, &GUID_SHORT_THREAD_POLICY, &value) == ERROR_SUCCESS) {
            result = (int)value;
        }
        LocalFree(pActiveScheme);
    }

    return result;
}

int set_policy(int policy_index) {
    GUID *pActiveScheme = NULL;
    int success = 0;

    if (PowerGetActiveScheme(NULL, &pActiveScheme) == ERROR_SUCCESS) {
        DWORD result1 = PowerWriteACValueIndex(NULL, pActiveScheme,
            &GUID_PROCESSOR_SUBGROUP, &GUID_SHORT_THREAD_POLICY, (DWORD)policy_index);

        DWORD result2 = PowerWriteDCValueIndex(NULL, pActiveScheme,
            &GUID_PROCESSOR_SUBGROUP, &GUID_SHORT_THREAD_POLICY, (DWORD)policy_index);

        DWORD result3 = PowerSetActiveScheme(NULL, pActiveScheme);

        success = (result1 == ERROR_SUCCESS &&
                   result2 == ERROR_SUCCESS &&
                   result3 == ERROR_SUCCESS);

        LocalFree(pActiveScheme);
    }

    return success;
}

void play_beep(int policy_index) {
    int freq = 400 + (policy_index * 100);  // 400Hz to 900Hz
    Beep(freq, 100);
}

void cycle_policy_all() {
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
}

void cycle_policy_perf() {
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
}

int main() {
    MSG msg;

    printf("Short Thread Policy Switcher\n");
    printf("============================\n");
    printf("ALT+X: Cycle through all 6 policies\n");
    printf("ALT+Z: Cycle All/Performant/Prefer performant\n");
    printf("Press CTRL+C or close window to exit\n\n");

    // Get and display current policy
    int current = get_current_policy();
    if (current >= 0 && current < NUM_POLICIES) {
        current_policy = current;
        printf("Current policy: [%d] %s\n\n", current, POLICY_NAMES[current]);
    } else {
        printf("Could not read current policy.\n");
        printf("Make sure you're running as Administrator.\n\n");
    }

    // Register hotkeys
    if (!RegisterHotKey(NULL, HOTKEY_ALL, MOD_ALT, 'X')) {
        printf("ERROR: Could not register hotkey ALT+X\n");
        return 1;
    }

    if (!RegisterHotKey(NULL, HOTKEY_PERF, MOD_ALT, 'Z')) {
        printf("ERROR: Could not register hotkey ALT+Z\n");
        UnregisterHotKey(NULL, HOTKEY_ALL);
        return 1;
    }

    printf("Hotkeys registered. Waiting...\n");

    // Message loop
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_HOTKEY) {
            if (msg.wParam == HOTKEY_ALL) {
                cycle_policy_all();
            } else if (msg.wParam == HOTKEY_PERF) {
                cycle_policy_perf();
            }
        }
    }

    UnregisterHotKey(NULL, HOTKEY_ALL);
    UnregisterHotKey(NULL, HOTKEY_PERF);
    return 0;
}
