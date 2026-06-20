# Changelog

## [1.0.2] - 2025-06-20

- Fix an issue where the gamepad was still being disabled despite -nojoy being unchecked
- Fix WinEventProc blocking the main thread and suspending Steam UI thread at startup
- Fix shortcut not being updated when all shortcut options were unchecked
- Fix a leak that was occurring on every CEF window creation
- Fix CEF mode set with the tray being ignored by the monitor thread: "Enable CEF" no longer disables CEF when a game runs

## [1.0.1] - 2025-02-07

- Add -console as an option
- Fix bug with shortcut path being truncated

## [1.0.0] - 2025-02-07

### Initial Release

- Options dialog on first launch with shortcut options (sign into friends, launch minimized, no joystick, no shaders, no GPU, animated avatars, show game icons), Automatic CEF, and dialog/tray visibility toggles
- System-tray icon with menu: Automatic CEF, Enable CEF, Disable CEF, Options
- Automatic CEF mode: disables CEF when a game runs, re-enables when the game closes
- Config stored in `SteamLite.ini` (same folder as the DLL); delete to reset
- Desktop shortcut "Steam Lite" created when any shortcut option is checked (launches Steam with saved options)
