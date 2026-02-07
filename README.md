# Steam Lite

A tiny program to condense Steam’s resource usage by controlling its CEF (Chromium Embedded Framework) and offering lightweight launch options.

<p align="center">
  <img width="366" height="397" alt="Capture" src="https://github.com/user-attachments/assets/251056af-e0f4-4f18-ab96-fa81b70d24e2" />
</p>

> [!NOTE]
>**Steam Lite is a DLL** that Steam loads at startup (it replaces the system DLL `user32.dll` when placed in the Steam folder).\
> Once loaded, it presents an options dialog on first run and a system-tray icon for quick CEF toggling.

## What it does

- **CEF control** – Turn CEF off, leave it on, or set it to **Automatic** (off while a game is running, on when no game is running).
  
- **Launch Options** – Configure Steam's launch options: launch minimized, no joystick, no GPU for CEF.\
  If no options are checked, no shortcut is created.

- **Desktop shortcut** – When you save options, Steam Lite creates or updates a **Steam Lite** shortcut on your Desktop that starts Steam with the chosen launch options.\
   Use this shortcut instead of `steam.exe` to launch Steam.

- **Config file** – All options are stored in **`SteamLite.ini`** in the same folder as the DLL (Steam root directory).


## Usage

1. Download the latest release from [GitHub Releases](https://github.com/otacoo/steam-lite/releases).

2. Place **`user32.dll`** in your Steam installation directory (the folder that contains `steam.exe`).

3. Start Steam normally, configure the options. Then close Steam again.

4. Use the **Desktop shortcut** to launch Steam.

5. Use the **system-tray icon** to switch between Automatic CEF, Enable CEF, and Disable CEF, or open **Options** to change settings again.


**Notes**

- To reset everything, delete **`SteamLite.ini`** in the Steam folder (and optionally remove the Desktop shortcut).
- To remove Steam Lite, delete **user32.dll**.

## Build

Steam Lite is built with **GCC** in an **MSYS2** environment.

1. Install [MSYS2](https://www.msys2.org/).

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
5. The built DLL will be at **`src/bin/user32.dll`**.

## Credits

This program is inspired by [AveYo](https://github.com/AveYo/)'s [steam_min.bat](https://github.com/AveYo/Gaming/blob/main/steam_min.bat) from which it takes its shortcut creation commands.




