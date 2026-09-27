# 1337Launcher

An "as simple as possible" Minecraft: Java Edition launcher, written in C++ with Qt 6.

## Features

- **Microsoft account login** (device-code flow: you get a short code and enter it at microsoft.com/link), with automatic token refresh
- **Offline accounts** (any player name, for singleplayer / offline-mode servers)
- **Instances**: each has its own `.minecraft` folder (worlds, mods, resource packs, options)
- Installs any vanilla version (releases, snapshots, old alpha/beta) from Mojang's version manifest
- Downloads libraries, natives, assets and log config in parallel, with SHA-1 verification and retries
- **Downloads the correct Java runtime automatically** (Java 8/17/21, whatever the version needs) from Mojang, or use your own Java
- Game log console with a kill button
- Works offline once a version has been downloaded

## Download

Grab a build from the [Releases](../../releases) page, or the latest CI build from the [Actions](../../actions) tab:

- **Linux**: `1337Launcher-x86_64.AppImage`: `chmod +x` it and run.
- **Windows**: `1337Launcher-windows-x64.zip`: extract and run `1337Launcher.exe`.

Data is stored in `~/.local/share/1337Launcher` on Linux and `%APPDATA%\1337Launcher` on Windows.
Put an empty `portable.txt` next to the executable to keep everything in a `data` folder beside it instead.

## Microsoft login setup

Microsoft login needs an Azure application (client) ID. It's not a secret, but every launcher has to register its own:

1. In the [Azure portal](https://portal.azure.com/#view/Microsoft_AAD_RegisteredApps/ApplicationsListBlade), create a new app registration.
   Supported account types: **Personal Microsoft accounts only**.
2. Under *Authentication*, enable **Allow public client flows**.
3. Apply for Minecraft API access using [Mojang's form](https://aka.ms/mce-reviewappid). Until the app is approved,
   the final login step fails with HTTP 403 ("Invalid app registration").
4. Put the *Application (client) ID* in the launcher's **Settings → Microsoft client ID**, or bake it into builds:
   - locally: `cmake -DMSA_CLIENT_ID=<id> ...`
   - on GitHub Actions: add a repository **variable** named `MSA_CLIENT_ID` (Settings → Secrets and variables → Actions → Variables).

Offline accounts work without any of this.

## Building

Requirements: CMake ≥ 3.16, a C++17 compiler, Qt ≥ 6.4 (Widgets, Network).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DMSA_CLIENT_ID=<optional client id>
cmake --build build
./build/1337Launcher
```

On Debian/Ubuntu: `sudo apt install cmake g++ qt6-base-dev libgl-dev`.

## Continuous integration

`.github/workflows/build.yml` builds on every push and pull request:

- **Linux**: Qt 6.8, packaged into an AppImage with linuxdeploy
- **Windows**: Qt 6.8 + MSVC, packaged with windeployqt into a zip

Builds are uploaded as workflow artifacts. Pushing a tag starting with `v` (e.g. `git tag v0.2.0 && git push origin v0.2.0`)
also publishes both files as a GitHub Release.

## Third-party code

- [miniz](https://github.com/richgel999/miniz) (MIT) for extracting native libraries, in `third_party/miniz`.
