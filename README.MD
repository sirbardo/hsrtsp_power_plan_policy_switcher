# Short Thread Policy Switcher

A lightweight Windows utility to control the **Heterogeneous Short Running Thread Scheduling Policy**. This hidden power setting can significantly affect game performance, especially when using PBO on AMD X3D CPUs.

## Features

- **GUI Version**: Visual interface with radio buttons, auto-refresh, and system tray-friendly operation
- **Console Version**: Minimal background hotkey daemon
- **PowerShell Scripts**: Quick one-click policy switching with auto-elevation
- **Global Hotkeys**: Switch policies from anywhere without opening the app
- **Audio Feedback**: Different beep tones for each policy setting

## Policy Options

0. **All processors** - Short threads can run on any core
1. **Performant processors** - Short threads run only on P-cores
2. **Prefer performant processors** - Short threads prefer P-cores
3. **Efficient processors** - Short threads run only on E-cores
4. **Prefer efficient processors** - Short threads prefer E-cores
5. **Automatic** - Let Windows decide

## Usage

### GUI Version (Recommended)

Run `policy_switcher_gui.exe` as Administrator. The GUI shows:
- Current policy setting (auto-refreshes every 2 seconds)
- Radio buttons to select and apply any policy
- Status information

**Hotkeys** (work even when minimized):
- **ALT+X** - Cycle through all 6 policies
- **ALT+V** - Cycle through performance policies (All → Performant → Prefer Performant)

### Console Version

Run `policy_switcher.exe` as Administrator for background hotkey daemon:
- **ALT+X** - Cycle through all 6 policies
- **ALT+V** - Cycle through performance policies

### PowerShell Scripts

Right-click and "Run as Administrator" or just double-click (auto-elevates):
- `set_all_processors.ps1` - Set to "All processors" (recommended for gaming)
- `set_prefer_performant.ps1` - Set to "Prefer performant processors"

## Requirements

- **Administrator privileges** (required to modify power settings)

## Building

### GUI Version

```batch
build_gui.bat
```

Requires either:
- Visual Studio (cl.exe)
- MinGW-w64 (gcc, windres)

### Console Version

```batch
build.bat
```

Requires either cl.exe or gcc in PATH.

## Download

Prefer not to build? Download pre-compiled executables from the [Releases](../../releases) tab.

## Technical Details

Modifies the Windows power setting:
- **Subgroup GUID**: `54533251-82be-4824-96c1-47b60b740d00` (Processor power management)
- **Setting GUID**: `bae08b81-2d5e-4688-ad6a-13243356654b` (Heterogeneous short running thread scheduling policy)

You can find more details on the current hypothesis on how this affects FPS here: https://x.com/bardozVAL/status/1999931009119162572

This setting is hidden by default. The app automatically unhides it on first run.
