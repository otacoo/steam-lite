# Changelog

## [1.0.0] - 2025-02-07

### Initial Release

- Options dialog on first launch with shortcut options (sign into friends, launch minimized, no joystick, no shaders, no GPU, animated avatars, show game icons), Automatic CEF, and dialog/tray visibility toggles
- System-tray icon with menu: Automatic CEF, Enable CEF, Disable CEF, Options
- Automatic CEF mode: disables CEF when a game runs, re-enables when the game closes
- Config stored in `SteamLite.ini` (same folder as the DLL); delete to reset
- Desktop shortcut "Steam Lite" created when any shortcut option is checked (launches Steam with saved options)
