# Process Killer

A terminal-based process manager and app launcher with real-time search and vim-style navigation.

Two implementations live in this repo:

| Directory       | Language    | Type           | Description                                 |
| --------------- | ----------- | -------------- | ------------------------------------------- |
| `app-drun-tui/` | C++         | TUI (terminal) | Process killer + `.desktop` app launcher    |
| `app-drun-gui/` | Rust (Iced) | GUI (window)   | `.desktop` app launcher with mouse/keyboard |

---

## Features

### Process Killer (`app-drun-tui`, no args)

- **List all processes** — reads from `/proc` to display running processes with PID and name
- **Column header UI** — NAME and PID columns with divider line, right-aligned PIDs
- **Real-time search** — filter by name or PID (case-insensitive), updates as you type
- **Dual-panel UI** — Tab to switch between Search and List panels
- **Vim/arrow key navigation** — scroll through processes with `j`/`k` or `↑`/`↓`
- **Viewport scrolling** — scroll indicator shows position (`─── 31-60 of 342 ───`)
- **Kill processes** — select a process and press Enter to send SIGTERM (with y/N confirmation)
- **Live refresh** — press `r` to re-scan `/proc` without restarting

### App Launcher (`--drun`)

- **Launch apps** — scan `.desktop` files from `/usr/share/applications/` and `~/.local/share/applications/`
- **Real-time search** — filter by app name as you type
- **Enter to launch** — runs the app and exits (like rofi/wofi)

### App Launcher (GUI)

- **Same `.desktop` parsing** — uses `freedesktop-desktop-entry` crate
- **Iced 0.14 GUI** — GruvboxDark theme, always-on-top, centered, undecorated
- **Keyboard + mouse wheel** — arrows, Tab, Enter, Escape, and scroll support
- **Hidden scrollbar** — visual noise minimized, scrolls perfectly in sync with selection

## Build & Run

### TUI (C++)

```sh
make -C app-drun-tui
./app-drun-tui/build/main          # process killer
./app-drun-tui/build/main --drun   # app launcher
```

Or manually:

```sh
clang++ app-drun-tui/main.cpp -o app-drun-tui/build/main
```

Hyprland bind:

```
bind = SUPER SHIFT, D, exec, kitty --class hyprland-run -e ~/path/app-drun-tui/build/main --drun
```

### GUI (Rust)

```sh
cargo run --manifest-path app-drun-gui/Cargo.toml
cargo build --release --manifest-path app-drun-gui/Cargo.toml
# binary: app-drun-gui/target/release/app-drun
```

## Controls

### Process Killer Mode (TUI)

| Key           | Panel  | Action                          |
| ------------- | ------ | ------------------------------- |
| Type anything | Search | Filter processes by name or PID |
| `Tab`         | Both   | Switch between Search and List  |
| `j` / `↓`     | List   | Scroll down                     |
| `k` / `↑`     | List   | Scroll up                       |
| `Enter`       | List   | Kill selected process           |
| `Backspace`   | Search | Delete last character           |
| `r`           | Both   | Refresh process list from `/proc` |
| `q`           | Both   | Quit                            |

### App Launcher Mode (TUI `--drun`)

| Key           | Action                |
| ------------- | --------------------- |
| Type anything | Filter apps by name   |
| `j` / `↓`     | Scroll down           |
| `k` / `↑`     | Scroll up             |
| `Enter`       | Launch selected app   |
| `Backspace`   | Delete last character |
| `q`           | Quit                  |

### App Launcher (GUI)

| Key               | Action              |
| ----------------- | ------------------- |
| Type anything     | Filter apps by name |
| `↓` / `Tab`       | Select next         |
| `↑` / `Shift+Tab` | Select previous     |
| `Enter`           | Launch selected app |
| `Escape`          | Quit                |
| Mouse scroll      | Select next/prev    |

## Requirements

- Linux (uses `/proc` filesystem)
- C++ compiler (tested with clang++) — for TUI
- Rust toolchain — for GUI
- Terminal with ANSI color support — for TUI
