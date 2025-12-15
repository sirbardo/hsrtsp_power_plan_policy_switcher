/*
 * Short Thread Policy Switcher
 * Press ALT+X to cycle through heterogeneous short running thread scheduling policies
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

#define HOTKEY_ALL 1
#define HOTKEY_PERF 2
#define NUM_POLICIES 6

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

int current_policy = 0;

void unhide_setting() {
    char command[512];
    // Unhide the setting so it can be queried and modified
    snprintf(command, sizeof(command),
        "powercfg -attributes %s %s -ATTRIB_HIDE",
        SUBGROUP_GUID, SETTING_GUID);
    system(command);
}

void play_beep(int policy_index) {
    // Different beep patterns for different policies
    // Higher frequency = higher policy number
    int freq = 400 + (policy_index * 100);  // 400Hz to 900Hz
    Beep(freq, 100);  // 100ms beep
}

int get_current_policy() {
    char command[512];
    char buffer[1024];
    FILE* pipe;
    int value = -1;

    // Query current AC value for active scheme
    snprintf(command, sizeof(command),
        "powercfg -query SCHEME_CURRENT %s %s",
        SUBGROUP_GUID, SETTING_GUID);

    pipe = _popen(command, "r");
    if (pipe) {
        while (fgets(buffer, sizeof(buffer), pipe)) {
            // Look for "Current AC Power Setting Index:"
            if (strstr(buffer, "Current AC Power Setting Index:")) {
                char* hex = strstr(buffer, "0x");
                if (hex) {
                    value = (int)strtol(hex, NULL, 16);
                }
                break;
            }
        }
        _pclose(pipe);
    }
    return value;
}

void set_policy(int policy_index) {
    char command[512];

    // Set AC value
    snprintf(command, sizeof(command),
        "powercfg -setacvalueindex SCHEME_CURRENT %s %s %d",
        SUBGROUP_GUID, SETTING_GUID, policy_index);
    system(command);

    // Set DC value (battery)
    snprintf(command, sizeof(command),
        "powercfg -setdcvalueindex SCHEME_CURRENT %s %s %d",
        SUBGROUP_GUID, SETTING_GUID, policy_index);
    system(command);

    // Apply changes
    system("powercfg -setactive SCHEME_CURRENT");
}

void cycle_policy_all() {
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

    // Just a beep - no popups that would steal focus
    play_beep(next_policy);
}

void cycle_policy_perf() {
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

    // Just a beep - no popups that would steal focus
    play_beep(next_policy);
}

int main() {
    MSG msg;

    printf("Short Thread Policy Switcher\n");
    printf("============================\n");
    printf("ALT+X: Cycle through all 6 policies\n");
    printf("ALT+Z: Cycle All/Performant/Prefer performant\n");
    printf("Press CTRL+C or close window to exit\n\n");

    // Unhide the setting first (needs admin)
    printf("Unhiding power setting...\n");
    unhide_setting();

    // Get and display current policy
    int current = get_current_policy();
    if (current >= 0 && current < NUM_POLICIES) {
        current_policy = current;
        printf("Current policy: [%d] %s\n\n", current, POLICY_NAMES[current]);
    } else {
        printf("Could not determine current policy\n\n");
    }

    // Register global hotkey ALT+X (all policies)
    if (!RegisterHotKey(NULL, HOTKEY_ALL, MOD_ALT, 'X')) {
        printf("ERROR: Could not register hotkey ALT+X\n");
        return 1;
    }

    // Register global hotkey ALT+Z (perf toggle)
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

    // Cleanup
    UnregisterHotKey(NULL, HOTKEY_ALL);
    UnregisterHotKey(NULL, HOTKEY_PERF);
    return 0;
}
