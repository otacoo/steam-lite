#!/usr/bin/env bash
cd "$(dirname "$0")/src"
rm -rf bin
mkdir -p bin
gcc -Oz -Wl,--gc-sections,--exclude-all-symbols -municode -shared -nostdlib -s SteamLite.c -lntdll -lwtsapi32 -lkernel32 -luser32 -ladvapi32 -lshell32 -lole32 -o bin/user32.dll
