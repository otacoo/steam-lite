# Steam Lite

A tiny program to condense Steam’s resource use by controlling its CEF (Chromium Embedded Framework) and offering lightweight launch options.

> [!NOTE]
>**Steam Lite is a DLL** that Steam loads at startup (it replaces the system DLL `user32.dll` when placed in the Steam folder). Once loaded, it adds an options dialog on first run, a system-tray icon for quick toggles, and saves its settings to a config file in the Steam root folder.

## What it does

- **CEF control** – Turn CEF off, leave it on, or set it to **Automatic** (off while a game is running, on when no game is running).
- **Launch Options** – Configure Steam launch options (e.g. launch minimized, no joystick, no GPU for CEF) from the options dialog. If no options are checked, no shortcut is created.
- **Desktop shortcut** – When you save options, Steam Lite creates or updates a **Steam Lite** shortcut on your Desktop that starts Steam with your chosen launch options. Use this shortcut instead of `steam.exe` to apply those options.
- **Config file** – All options are stored in **`SteamLite.ini`** in the same folder as the DLL (Steam root directory). You can edit or delete this file to change or reset settings.


## Usage

1. Download the latest release from [GitHub Releases](https://github.com/otacoo/SteamLite/releases).

2. Place **`user32.dll`** in your Steam installation directory (the folder that contains `steam.exe`). Steam loads this DLL by name from the app directory, so the file **must** be named `user32.dll` to be loaded.

3. Start Steam normally, configure the options. Then close Steam again.

4. Use the **Desktop shortcut** to launch Steam.

5. Use the **system-tray icon** to switch between Automatic CEF, Enable CEF, and Disable CEF, or open **Options** to change settings again.


**Notes**

- To avoid the Steam window appearing when restored, enable “Launch minimized” in options or use the Desktop shortcut (which can include `-silent`).
- To reset everything, delete **`SteamLite.ini`** in the Steam folder (and optionally remove the Desktop shortcut).

## Build

Steam Lite is built with **GCC** in an **MSYS2** environment.

1. Install [MSYS2](https://www.msys2.org/):

2. Launch MSYS2 **UCRT64** and update:

   ```bash
   pacman -Syu --noconfirm
   ```

3. Install the required gcc MinGW toolchain:

   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc --noconfirm
   ```

4. Browse to the project directory, and run the build script:

   ```bash
   cd /path/to/SteamLite
   ./build.sh
   ```

5. The built DLL will be at **`src/bin/user32.dll`**. Copy it into your Steam folder as in step 2 above.

## Credits
This program is inspired by [AveYo](https://github.com/AveYo/)'s [steam_min.bat](https://github.com/AveYo/Gaming/blob/main/steam_min.bat) from which it takes its shortcut creation commands.