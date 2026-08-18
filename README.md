# Weqi

**Weqi** is a modern, clean, open-source desktop chess application. All chess rules are implemented in a local C++ engine, with support for human vs human, human vs AI, AI vs AI, and game replay.

> **Read this document in your language:**
> [简体中文](docs/zh-CN/README.md) · [繁體中文](docs/zh-TW/README.md) · [日本語](docs/ja/README.md) · [Español](docs/es/README.md) · [Українська](docs/uk/README.md) · [한국어](docs/ko/README.md)

---

## Features

- **Four game modes**
  - **Human vs Human** — two players on the same device.
  - **Human vs AI** — play against an AI provider of your choice.
  - **AI vs AI** — two AIs play against each other automatically (does not count toward your stats).
  - **Replay** — review your historical games.
- **Full chess rules** implemented locally in C++ (legal move validation, check, checkmate, stalemate, castling, en passant, promotion, etc.).
- **Player profile & rating** — track your rating, best rating, and edit your player name.
- **Statistics** — games played, win rate, wins, draws, losses, best/current win streak, best rating.
- **Game history** — browse past games with date, mode, opponent, result, and rating change.
- **Continue unfinished game** — resume an in-progress game at any time.
- **AI providers** — add, edit, and delete AI providers (name, type, base URL, API key, model). API keys are stored privately in the system user-data directory, never in the project.
- **AI chat** — the AI briefly comments on its move; the message follows your UI language.
- **Internationalization (i18n)** — 7 interface languages: Simplified Chinese, Traditional Chinese, English, Japanese, Spanish, Ukrainian, and Korean. Switch languages at runtime without restarting.

## Tech Stack

- **C++17**
- **Qt 6** (Widgets)
- **CMake**

## Directory Structure

```
weqi/
├── CMakeLists.txt
├── README.md
├── ai_adapter/          # Python AI adapter (language detection, retry/coaching)
│   ├── main.py
│   ├── parser.py
│   └── providers/
│       └── openai_compatible.py
├── dist/                # Distributable directory (compiled binary + packaging scripts)
├── resources/           # Qt resource files
├── scripts/             # Helper scripts
├── src/
│   ├── main.cpp         # Program entry
│   ├── MainWindow.h/.cpp
│   ├── GameController.h/.cpp
│   ├── ChessBoard.h/.cpp
│   ├── ChessPiece.h/.cpp
│   ├── ai/              # AI manager
│   ├── data/            # Settings, language, profile, stats, history managers
│   └── pages/           # Home, NewGame, Game, AIOpponent, AIVsAI, History, Settings, About
├── tests/               # C++ tests
├── tools/               # Development tools
└── translations/        # Qt .ts translation files (7 languages)
```

## Build

### Dependencies

- CMake ≥ 3.16
- Qt 6 (≥ 6.2, including the Widgets module)
- A C++17-capable compiler (GCC / Clang)
- Python 3 (for the AI adapter)

### Build Steps

```bash
# 1. Configure
cmake -S . -B build

# 2. Build
cmake --build build -j

# 3. Run
./build/Weqi
```

Or as a one-liner:

```bash
cmake -S . -B build && cmake --build build -j && ./build/Weqi
```

### Translations

Translation files live in `translations/` and are compiled into the binary via Qt's `qt_add_translations`. To update them after changing `tr()` strings:

```bash
# Update .ts files (extract new strings)
lupdate src -ts translations/weqi_<lang>.ts

# Release .qm files
lrelease translations/weqi_<lang>.ts
```

## Packaging

The `dist/` directory contains the compiled binary and the Python AI adapter, ready to be packaged into installers.

```
dist/
├── Weqi                  # Compiled C++ binary (Qt6 Widgets, 7 languages embedded)
├── ai_adapter/           # Python AI adapter
├── package-deb.sh        # Build a .deb (Linux)
├── package-rpm.sh        # Build a .rpm (Linux, Fedora/RHEL/openSUSE)
├── weqi.spec             # RPM spec file
├── package-exe.sh        # Build a .exe (Windows, run on Windows)
└── README.md
```

- **.deb**: `cd dist && ./package-deb.sh` → `weqi_0.1.0_amd64.deb`
- **.rpm**: `cd dist && ./package-rpm.sh` → `weqi-0.1.0-1.fc44.x86_64.rpm`
- **.exe**: run `./package-exe.sh` on Windows → `weqi-win/` directory (uses `windeployqt`)

The binary locates `ai_adapter/main.py` relative to the executable, or at `/usr/share/weqi/ai_adapter/main.py` when installed via a package.

## Usage

- **Select a piece**: left-click any piece to highlight it.
- **Move a piece**: after selecting, click a target square. Legal moves are validated by the local engine.
- **Undo**: use the "Undo" button to take back a move.
- **New game**: start a fresh game from the game page or the home screen.
- **AI vs AI**: choose two AI providers, then use Start / Pause / Resume / Stop to control the automatic match.
- **Change language**: open Settings → Language and pick one of the 7 supported languages (or "Follow System").

## Data Storage

- **Settings & profile**: stored in the system user-data directory (e.g. `~/.local/share/Weqi/` on Linux).
- **AI provider config**: stored separately in the user-data directory, never in the project.
- **Game history & saved games**: stored in the user-data directory.

## License

Open source. See the repository for details.
