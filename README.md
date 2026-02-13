
A lightweight, mean & modern C++23 DLL injection framework for runtime memory patching on Windows. **meanp** dynamically applies binary patches to processes, supporting both immediate patching and deferred patching via DLL load.
I've stumbled upon [@aixxe] (https://github.com/aixxe)'s memory patcher which inspired me to make one as well, so thank you & I hope you won't mind! Also big props! 
## Features

- **Multiple Address Modes**: Absolute addresses, RVAs (Relative Virtual Addresses), and file offsets
- **Dynamic DLL Monitoring**: Automatically patches target DLLs as they load using `LdrRegisterDllNotification`
- **Data Validation**: Optional verification of expected bytes before patching
- **Human-Readable Format**: Simple text-based `.mph` patch files
- **Thread-Safe**: Mutex-protected patch application for concurrent scenarios
- **Self-Unloading**: Automatically removes itself after all patches are applied
- **Color-Coded Logging**: Real-time console output with timestamps for debugging
- **Memory Protection Handling**: Automatic `VirtualProtect` management with RAII guards

## Quick Start

### Prerequisites

- **Compiler**: MSVC with C++23 support (Visual Studio 2022+)
- **Platform**: Windows 10/11 (x86 or x64)
- **Runtime**: MSVC Runtime (can be statically linked)

### Building

```bash
# Clone the repository
git clone https://github.com/yourusername/meanp.git
cd meanp

# Open in Visual Studio
start meanp.sln

# Or build via command line
msbuild meanp.sln /p:Configuration=Release /p:Platform=x86
```

### Usage

1. **Create patch files** (`.mph` format) with your desired memory patches
2. **Configure loading** via either:
   - `patches.txt` - List of patch file paths
   - `autopatch/` directory - Automatically discovered `.mph` files
3. **Inject the DLL** into your target process using any DLL injector

### Example Patch File

Create `example.mph`:

```
# Patch notepad.exe - NOP out a function call
notepad.exe 1A2B CC E9

# Validate data without patching (useful for version checking)
user32.dll 0 4D5A -

# Absolute address patch
- 401000 - 90909090

# File offset patch (converted to RVA at runtime)
kernel32.dll F+5000 8B FF 33C0

# Multi-byte replacement with validation
<host> 2000 558BEC 90909090
```

## Patch File Format

### Syntax

```
<target> <offset> <expected_bytes> <new_bytes>
```

| Field | Description | Examples |
|-------|-------------|----------|
| `<target>` | Module name, `-` for absolute, or `<host>` for main exe | `notepad.exe`, `kernel32.dll`, `-`, `<host>` |
| `<offset>` | Address (hex), prefix `F+` for file offset | `1000`, `F+2A00` |
| `<expected_bytes>` | Validation data (hex), `-` to skip | `4D5A`, `558BEC`, `-` |
| `<new_bytes>` | Replacement data (hex), omit for validation-only | `90`, `33C0C3` |

### Special Targets

- **`-`**: Absolute memory address (no base module)
- **`<host>`**: Main executable of the process
- **`"Module Name.dll"`**: Use quotes for names with spaces

### Address Types

1. **RVA (default)**: `notepad.exe 1000` - Relative to module base
2. **File Offset**: `kernel32.dll F+5000` - Converts from PE file offset
3. **Absolute**: `- 401000` - Direct memory address

### Comments

Lines starting with `#` or `;` are ignored. Empty lines are also ignored.

## Directory Structure

```
YourDLL/
├── mempatcher.dll          # Your compiled DLL
├── patches.txt             # (Optional) List of patch file paths
└── autopatch/              # (Optional) Auto-discovered .mph files
    ├── game_v1.2.mph
    └── ui_fixes.mph
```

### patches.txt Format

```
# List patch files (one per line)
C:\patches\critical.mph
.\local_patches\optional.mph

# Relative paths are supported
patches\game.mph

# Comments and blank lines are ignored
```

## Architecture

```
┌─────────────┐
│  DllMain    │ Entry point
└──────┬──────┘
       │
       ▼
┌─────────────────┐
│  Init Thread    │ Offloads work from DllMain
└──────┬──────────┘
       │
       ├──► Parser ──────► Read .mph files
       │
       ├──► Patcher ─────► Apply patches immediately
       │
       └──► Hook Manager ► Monitor for delayed targets
                │
                └──► LdrRegisterDllNotification
                            │
                            └──► Patches deferred targets on DLL load
```

### Key Components

| Module | Responsibility |
|--------|---------------|
| **parser.cpp** | Parse `.mph` files into `patch_t` structures |
| **patcher.cpp** | Apply patches with validation and memory protection |
| **hooks.cpp** | DLL load notifications and deferred patching |
| **console.cpp** | Color-coded logging with timestamps |
| **utils.cpp** | File discovery, path resolution, hex formatting |

Common errors:
- **File not found**: Check paths in `patches.txt`
- **Validation failed**: Expected bytes don't match (wrong version/already patched)
- **VirtualProtect failed**: Insufficient permissions or invalid address
- **Parse error**: Syntax issue in `.mph` file

## Security Considerations

⚠️ **Warning**: This tool modifies process memory at runtime. Use responsibly and only on software you own or have permission to modify of course.

- Memory patches persist only while the DLL is loaded
- Original bytes are restored on unload (if `off` data is provided)
- No persistence across process restarts
- No kernel-mode or system-level patching

## Limitations (For now)

- **Windows only** (uses Win32 API and NT internals)
- **PE executables only** (no support for ELF, Mach-O, etc.)
- **No wildcard patterns** (addresses must be explicit)
- **No code relocation** (patches are byte-for-byte replacements)
- **Single-process** (does not propagate to child processes)

## Contributing

Contributions are welcome! Areas for improvement:

- [ ] Pattern scanning with wildcards (`? ?` syntax)
- [ ] Support for relative jumps/calls with automatic offset calculation
- [ ] Code cave allocation and trampolines
- [ ] x64 → x86 cross-process patching support
- [ ] GUI for patch file creation
- [ ] Lua scripting for conditional patches

### Development Setup

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Acknowledgments

- Inspired by https://github.com/aixxe/mempatcher/tree/master & classic memory patching tools and game trainers
- Uses modern C++23 features (`std::expected`, ranges, format)
- Built with ❤️ for the reverse engineering and modding communities

## Disclaimer

This software is provided for educational and research purposes only. The authors are not responsible for any misuse or damage caused by this tool. Always respect software licenses and terms of service.

---

**Star ⭐ this repo if you find it useful!**

For questions or issues, please open a [GitHub Issue](https://github.com/neear0/meanp/issues).
