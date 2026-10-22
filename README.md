# ASM51 IDE

Single-file 8051 assembly IDE for VS Code. No prerequisites besides VS Code itself.
Works on **Windows** and **Linux (x86 / x86_64)**. 

## Install

1. In VS Code: Extensions view → `…` menu → **Install from VSIX…** → pick `asm51-ide-*.vsix`
   (or run `code --install-extension asm51-ide-1.0.0.vsix`).
2. Open a folder with `.asm` files and open one.

Linux notes: the bundled ASEM-51 is a static 32-bit x86 program (no extra libraries needed); `tar` must be present
(it is on every distribution). On ARM machines (Raspberry Pi, ARM VMs) set `asm51.assemblerPath` to your own ASEM-51 build.

## Use

| Action | Status bar / title bar | Key |
| --- | --- | --- |
| Compile the active `.asm` file | **▶ ASM51 Compile** | F7 |
| Compile and flash | **⚡ Flash** | F8 |
| Choose serial port | **🔌 port** | |

* Errors appear in the *Problems* panel and as red squiggles; the full assembler output is in the *ASM51* output channel.
* **Hex viewer:** open any `.hex` file and click the **hex-file icon** in the editor title bar (or run *ASM51: View Hex File*;
  or right-click the tab → *Reopen Editor With…* → *Intel HEX Viewer*). It shows address, bytes and ASCII, validates record
  checksums, lists the used address ranges, and refreshes automatically after every compile. Hover a byte for its decimal/binary value.
* Compiling writes `<name>.hex` and `<name>.lst` next to the source file.
* The course dialect is supported: `#include "c:\aducgd1.inc"` is resolved (falling back to the bundled copy in `inc/`),
  and hex literals without a leading zero (`#efh`) are accepted. Line numbers in errors match your source.

## Flashing

**Compile and flash** (F8) downloads the program to the ADuC832 with the bundled `lsd` flasher: no extra software, drivers
or compilers are needed. Windows uses a PowerShell port of `lsd` (built into Windows); Linux uses a prebuilt static `lsd`
for x86_64 or arm64.

1. Connect the board and put it in **download mode** (hold the DOWNLOAD/BOOT button, press RESET).
2. Press **Flash** and pick the serial port (remembered afterwards; change it with the plug item in the status bar).
3. After downloading, the program is started on the board (`asm51.flash.run`).

The hex file is normalised before flashing (sorted by address, gaps filled with `FF`, padded to 256-byte pages), and programs
above 62 KB are rejected. If the board does not answer within ~12 s you get a hint to check download mode and the port.

**Linux:** serial ports are usually only usable by members of the `dialout` group (`uucp` on Arch). If flashing reports
"No permission", run `sudo usermod -aG dialout $USER`, then log out and back in.

To use another tool instead, set `asm51.flash.toolPath` and `asm51.flash.args` (placeholders `${hex}`, `${port}`, `${file}`).

## Installation check

On first start after installing (or updating), the extension runs a self-check: it unpacks and test-runs the assembler and the
flasher, and verifies `tar`/PowerShell and (Linux) serial-port permissions. Problems are reported in a notification and in the
*ASM51* output channel. Run it again at any time with **ASM51: Check Installation**. Nothing is compiled on the student's
machine: everything needed ships inside the VSIX.

## Building the VSIX

```
npx @vscode/vsce package --allow-missing-repository --skip-license
```

The VSIX contains the ready-made flasher binaries (`tools/`), so building needs no compiler.
Tests (not packaged): `test/hex.test.js`, and `test/fakeloader.py` + `test/e2e.js` (fake ADuC832 on a pty, Linux only).

## Changelog

### 1.0.0 (2026-10-21)
* Flash support: the bundled `lsd` flasher downloads the program to the ADuC832 (Windows: PowerShell port, Linux: prebuilt x64/arm64 binaries). No extra tools, drivers or compilers needed.
* The hex file is normalised before flashing (address-ordered, gaps and last page padded with `FF`); programs above 62 KB are rejected.
* Flashing has a timeout and clear messages for: board not in download mode, port busy/missing, NAK, and (Linux) missing `dialout` permission.
* New setting `asm51.flash.run` (start the program after download); `asm51.flash.toolPath` now only overrides the bundled flasher.
* Automatic installation check on first start and after each update, plus the command *ASM51: Check Installation*.
* Third-party `lsd` licences are included under `tools/licenses`; the package no longer references any external git server.

### 0.2.0
* Intel HEX viewer (custom editor, checksum validation, used address ranges, live refresh after compiling).
* Fixed the compile button failing on Windows when unpacking the assembler.

### 0.1.0
* Single-file 8051 assembly IDE replacing Reads51: syntax highlighting, compile with bundled ASEM-51 (Windows and Linux x86/x64), errors in the Problems panel, serial-port picker.
* Support for the course dialect (`#include "c:\aducgd1.inc"`, hex literals like `#efh`), with bundled `aducgd1.inc` / `aducgd2.inc`.

## Third-party

* `lsd` ADuC832 flasher (prebuilt binaries and a PowerShell port in `tools/`; third-party licences in `tools/licenses`).
* ASEM-51 V1.3 © W.W. Heinz — free software, redistributed **unmodified** in `vendor/` (`asem5113.zip` for Windows,
  `asem51-1.3-ELF.tar.gz` for Linux; see `LICENSE.DOC` inside / https://plit.de/asem-51/agreemnt.htm). Unpacked on first use.
* `inc/aducgd1.inc`, `inc/aducgd2.inc` © course authors (Roggemans M.), bundled as fallback for the exercises.
