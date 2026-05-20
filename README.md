# Process Killer

A terminal-based process manager with real-time search and vim-style navigation.

## Features

- **List all processes** — reads from `/proc` to display running processes with PID and name
- **Real-time search** — filter by name or PID (case-insensitive), updates as you type
- **Dual-panel UI** — Tab to switch between Search and List panels
- **Vim/arrow key navigation** — scroll through processes with `j`/`k` or `↑`/`↓`
- **Viewport scrolling** — scroll indicator shows position (`─── 31-60 of 342 ───`)
- **Kill processes** — select a process and press Enter to send SIGTERM (with y/N confirmation)
- **Colored TUI** — fully colored terminal UI with box-drawing characters

## Build & Run

```sh
make
```

Or manually:

```sh
clang++ main.cpp -o build/main && ./build/main
```

The `Makefile` also has a `run` target: `make run`.

## Controls

| Key | Panel | Action |
|-----|-------|--------|
| Type anything | Search | Filter processes by name or PID |
| `Tab` | Both | Switch between Search and List |
| `j` / `↓` | List | Scroll down |
| `k` / `↑` | List | Scroll up |
| `Enter` | List | Kill selected process |
| `Backspace` | Search | Delete last character |
| `q` | Both | Quit |

## Requirements

- Linux (uses `/proc` filesystem)
- C++ compiler (tested with clang++)
- Terminal with ANSI color support
