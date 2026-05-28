# obs-fade-dock

A native OBS Studio plugin that adds a **Scene Fade Controls** dock — just like the built-in Scene Transitions dock, but with per-scene fade in, fade out, and switch buttons plus a global duration slider.

## What it does

- Lists all your scenes live in a scrollable dock
- Highlights the currently active scene
- **▲ In** — fades in to that scene (sets Fade transition, switches to it)
- **▼ Out** — fades out from that scene to black (auto-creates a hidden `__FadeDock_FTB__` scene)
- **⇌** — switches to that scene using a fade
- Global **duration slider** (0.1 s → 5 s)
- Animated progress bar under each scene during the fade
- Auto-updates when you add/rename/delete scenes

---

## Requirements

- OBS Studio 28 or later (obs-websocket NOT required)
- Qt 5.15 or Qt 6.x (whichever your OBS was built with)
- CMake 3.16+
- A C++17 compiler

---

## Building

### Option A — In-tree build (recommended, easiest)

1. Clone obs-studio and this plugin together:
   ```
   git clone --recursive https://github.com/obsproject/obs-studio.git
   cd obs-studio/plugins
   git clone <this repo> obs-fade-dock
   ```

2. Add to `obs-studio/plugins/CMakeLists.txt`:
   ```cmake
   add_subdirectory(obs-fade-dock)
   ```

3. Build OBS normally:
   ```bash
   cmake -S . -B build -DENABLE_PLUGINS=ON
   cmake --build build
   ```

---

### Option B — Out-of-tree build (standalone)

#### Linux

```bash
# Install deps (Ubuntu/Debian)
sudo apt install cmake libobs-dev qtbase5-dev

# Or for Qt6:
# sudo apt install cmake libobs-dev qt6-base-dev

git clone <this repo> obs-fade-dock
cd obs-fade-dock
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=/usr \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

#### Windows

```powershell
# Prerequisites:
# - OBS Studio installed (or built from source)
# - Qt 6.x installed via Qt Installer
# - CMake, Visual Studio 2022

cmake -S . -B build `
  -A x64 `
  -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2019_64" `
  -DLibObs_DIR="C:\Program Files\obs-studio\cmake"

cmake --build build --config Release
cmake --install build --prefix "C:\Program Files\obs-studio"
```

#### macOS

```bash
brew install cmake qt@6

cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

---

## Manual install (if you have a compiled .so / .dll)

### Linux
```
~/.config/obs-studio/plugins/obs-fade-dock/bin/64bit/obs-fade-dock.so
~/.config/obs-studio/plugins/obs-fade-dock/data/locale/en-US.ini
```

### Windows
```
C:\Program Files\obs-studio\obs-plugins\64bit\obs-fade-dock.dll
C:\Program Files\obs-studio\data\obs-plugins\obs-fade-dock\locale\en-US.ini
```

### macOS
```
~/Library/Application Support/obs-studio/plugins/obs-fade-dock/bin/obs-fade-dock.so
~/Library/Application Support/obs-studio/plugins/obs-fade-dock/data/locale/en-US.ini
```

---

## Usage

After installing and restarting OBS:

1. Go to **View → Docks** and enable **Scene Fade Controls**
   (or it appears automatically on first launch)
2. Drag the dock wherever you like — it's fully dockable like any native dock
3. Adjust the **Duration** slider at the top
4. Click **▲ In**, **▼ Out**, or **⇌** on any scene row

> **Note:** The plugin auto-creates a hidden scene called `__FadeDock_FTB__` used as a black frame for fade-outs. Don't delete it.

---

## How fades work

OBS doesn't expose per-source opacity animation in its public API, so fades are implemented by:

- **Fade In** → sets the global transition to `Fade` at your chosen duration, then switches the program scene to the target
- **Fade Out** → switches the program scene to a blank (black) scene with a Fade transition
- **Switch** → same as Fade In but from whatever the current scene is

This is identical to what OBS does internally with transition overrides.
