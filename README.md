# Process Killer

A terminal-based process manager and app launcher with real-time search and vim-style navigation.

## Features

### Process Killer (no args)
- **List all processes** — reads from `/proc` to display running processes with PID and name
- **Real-time search** — filter by name or PID (case-insensitive), updates as you type
- **Dual-panel UI** — Tab to switch between Search and List panels
- **Vim/arrow key navigation** — scroll through processes with `j`/`k` or `↑`/`↓`
- **Viewport scrolling** — scroll indicator shows position (`─── 31-60 of 342 ───`)
- **Kill processes** — select a process and press Enter to send SIGTERM (with y/N confirmation)

### App Launcher (`--drun`)
- **Launch apps** — scan `.desktop` files from `/usr/share/applications/`
- **Real-time search** — filter by app name as you type
- **Enter to launch** — runs the app and exits (like rofi/wofi)

## Build & Run

```sh
make
```

Or manually:

```sh
clang++ main.cpp -o build/main && ./build/main
```

### App Launcher

```sh
./build/main --drun
```

Or bind to a key in Hyprland:

```
bind = SUPER SHIFT, D, exec, kitty --class hyprland-run -e ~/path/to/build/main --drun
```

## Controls

### Process Killer Mode

| Key | Panel | Action |
|-----|-------|--------|
| Type anything | Search | Filter processes by name or PID |
| `Tab` | Both | Switch between Search and List |
| `j` / `↓` | List | Scroll down |
| `k` / `↑` | List | Scroll up |
| `Enter` | List | Kill selected process |
| `Backspace` | Search | Delete last character |
| `q` | Both | Quit |

### App Launcher Mode

| Key | Action |
|-----|--------|
| Type anything | Filter apps by name |
| `j` / `↓` | Scroll down |
| `k` / `↑` | Scroll up |
| `Enter` | Launch selected app |
| `Backspace` | Delete last character |
| `q` | Quit |

## Requirements

- Linux (uses `/proc` filesystem)
- C++ compiler (tested with clang++)
- Terminal with ANSI color support
