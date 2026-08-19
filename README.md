# gccauto

A tool that automates installing gcc on Windows, and remembers your compiler
flags so you don't have to type them every time.

## What problem does this solve?

Setting up a working C compiler on Windows normally means: install MSYS2,
run pacman to pull the right toolchain packages, manually add the right
folder to your PATH, open a new terminal, and hope you got the flags right.
`gccauto` turns that into one command, and one more command per project
after that.

## Who needs this?

- Students or beginners setting up C on a new Windows machine for the first
  time, without wanting to learn MSYS2/pacman internals just to compile a
  file.
- Anyone who's tired of typing the same gcc flags (`-Wall -Wextra -std=c11`,
  etc.) on every single compile.
- Instructors or bootcamps who want students to get a working `gcc` in one
  step instead of a 20-minute setup detour.

If you already have a gcc setup you're happy with, you don't need this.

## Install

Open **PowerShell** (not Command Prompt) and run:

```powershell
irm https://raw.githubusercontent.com/ubay-mostafa/gccauto/main/install.ps1 | iex
```

This downloads `gccauto.exe` into `Documents\GCCAuto` and adds it to your
PATH. It works in the same window right after — no need to reopen anything
for `gccauto` itself (only plain `gcc` needs a fresh terminal, once gcc is
installed — that's a Windows limitation, not this tool).

## Usage

**First time?** Just run:

```powershell
gccauto
```

This walks you through everything: installing gcc, and picking which
compiler flags you want saved (e.g. `-Wall`, `-O2`, `-std=c11`).

**Day to day**, once set up:

```powershell
gccauto build myfile.c
```

Compiles `myfile.c` into `myfile.exe`, automatically using whatever flags
you saved.

**Individual commands**, if you don't want the guided wizard:

| Command | What it does |
|---|---|
| `gccauto setup` | Installs gcc only, no flag prompts |
| `gccauto config add <flag>` | Save one flag, e.g. `-Wall` |
| `gccauto config list` | Show currently saved flags |
| `gccauto config clear` | Remove all saved flags |
| `gccauto help` | Show the command list |

Your saved flags and setup logs live in the same folder as `gccauto.exe`
itself (e.g. `Documents\GCCAuto`), so they're easy to find.

## How it works

Under the hood, `gccauto setup` downloads the official
[MSYS2](https://www.msys2.org/) installer, runs it silently, then uses
MSYS2's `pacman` package manager to install the `mingw-w64-ucrt-x86_64`
gcc toolchain — the same toolchain you'd get by following MSYS2's manual
setup instructions, just automated.

## Contributing

Contributions are welcome — this is a small, single-file C tool, so it's
easy to get into.

1. Fork the repo and clone your fork.
2. Make your changes in `main.c` (or `install.ps1` for the installer
   script).
3. Compile and test locally:
   ```
   gcc main.c -o gccauto.exe -lurlmon
   ```
4. Open a pull request describing what you changed and why.

Good first contributions:
- More default flag options in the setup wizard.
- Support for a second compiler/toolchain (e.g. clang).
- Better error messages when a step fails.
- Testing on machines with unusual PATH setups and reporting issues.

## How to help (without writing code)

- **Test it** on a machine you don't mind experimenting on, and open an
  issue for anything confusing or broken — screenshots/terminal output are
  very useful.
- **Star the repo** if you find it useful — it helps others find it too.
- **Share feedback** on what flags or workflows you'd want supported.
- **Spread the word** to anyone learning C on Windows who's stuck on setup.

## License

MIT — see [LICENSE](LICENSE).
