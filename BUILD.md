# Building CCT

Instructions for building Clavicula Turing (CCT) on different platforms.

## Linux

### Requirements
- GCC compiler
- Make
- Standard development tools

### Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential
```

**Fedora/RHEL:**
```bash
sudo dnf groupinstall "Development Tools"
```

**Arch Linux:**
```bash
sudo pacman -S base-devel
```

### Build

```bash
# Clone the repository
git clone https://github.com/your-repo/cct.git
cd cct

# Build the compiler
make

# Run tests
make test

# Create release package
make release
```

This will create:
- `cct-v0.12-linux-x86_64.tar.gz` (on x86_64)
- `cct-v0.12-linux-arm64.tar.gz` (on ARM64/aarch64)
- `cct-v0.12-linux-*.tar.gz.sha256` (checksum file)

### Install

```bash
# Install to /usr/local
sudo make install

# Or install to custom location
make install PREFIX=/opt/cct

# Uninstall
sudo make uninstall
```

---

## macOS

### Requirements
- Xcode Command Line Tools (includes GCC/Clang and Make)

### Install Dependencies

```bash
# Install Xcode Command Line Tools
xcode-select --install
```

### Build

```bash
# Clone the repository
git clone https://github.com/your-repo/cct.git
cd cct

# Build the compiler
make

# Run tests
make test

# Create release package
make release
```

This will create:
- `cct-v0.12-macos-arm64.tar.gz` (on Apple Silicon)
- `cct-v0.12-macos-x86_64.tar.gz` (on Intel Mac)
- `cct-v0.12-macos-*.tar.gz.sha256` (checksum file)

### Install

```bash
# Install to /usr/local
sudo make install

# Uninstall
sudo make uninstall
```

---

## Windows

CCT can be built on Windows using WSL2 (recommended) or MSYS2/MinGW.

### Option 1: WSL2 (Recommended)

**Best for:** Development and testing on Windows.

1. **Install WSL2:**
   ```powershell
   # Run in PowerShell as Administrator
   wsl --install
   ```

2. **Install Ubuntu from Microsoft Store**

3. **Build in WSL:**
   ```bash
   # Inside WSL Ubuntu terminal
   sudo apt update
   sudo apt install build-essential

   git clone https://github.com/your-repo/cct.git
   cd cct
   make release
   ```

This creates a **Linux** binary that runs in WSL:
- `cct-v0.12-linux-x86_64.tar.gz`

**Usage:**
```bash
# In WSL
./cct run examples/hello.cct
```

### Option 2: MSYS2 UCRT64 (Native Windows Binary)

**Best for:** Creating native Windows executables (.exe).

1. **Install MSYS2:**
   - Download from https://www.msys2.org/
   - Run the installer
   - Follow installation steps

2. **Install Build Tools:**
   ```bash
   # In MSYS2 UCRT64 terminal
   pacman -S mingw-w64-ucrt-x86_64-gcc make
   ```

3. **Build:**
   ```bash
   # In MSYS2 UCRT64 terminal
   cd /c/path/to/cct
   mingw32-make release
   ```

This creates a **Windows** native binary:
- `cct-v0.12-windows-x86_64.zip`
- Contains `cct.exe` and `cct.bat`

> **Note:** Uses PowerShell `Compress-Archive` — nenhuma dependência extra necessária.

**Usage from the MSYS2 UCRT64 terminal** (recommended — PATH already set):
```bash
./cct examples/hello.cct
```

**Usage from Windows CMD or PowerShell:**

The `cct.exe` binary is statically linked and runs standalone, but to compile
`.cct` files it needs to invoke the host C compiler (`gcc`). When running from
CMD/PowerShell you must point the `CC` environment variable to the full path of
`gcc.exe`:

```cmd
set CC=C:\msys64\ucrt64\bin\gcc.exe
cct examples\hello.cct
```

To make this permanent, add it to your user environment variables via
*System Properties → Environment Variables*, or add `C:\msys64\ucrt64\bin` to
your system `PATH`.

> **Note:** The `CC` environment variable is respected by all `cct` compilation
> commands. If it is not set and `gcc` is not in `PATH`, a hint is printed with
> the exact command to run.

### Option 3: MinGW-w64

1. **Install MinGW-w64:**
   - Download from https://winlibs.com/ or
   - Install via package manager (Chocolatey, Scoop)

2. **Add to PATH:**
   ```powershell
   # Add MinGW bin directory to PATH
   $env:PATH += ";C:\mingw64\bin"
   ```

3. **Build:**
   ```bash
   # In PowerShell or CMD with MinGW in PATH
   make release
   ```

---

## Platform-Specific Build Artifacts

| Platform | Binary | Wrapper | Package Name |
|----------|--------|---------|--------------|
| Linux x86_64 | `cct.bin` (ELF) | `cct` (shell) | `cct-v0.12-linux-x86_64.tar.gz` |
| Linux ARM64 | `cct.bin` (ELF) | `cct` (shell) | `cct-v0.12-linux-arm64.tar.gz` |
| macOS ARM64 | `cct.bin` (Mach-O) | `cct` (shell) | `cct-v0.12-macos-arm64.tar.gz` |
| macOS x86_64 | `cct.bin` (Mach-O) | `cct` (shell) | `cct-v0.12-macos-x86_64.tar.gz` |
| Windows x86_64 | `cct.exe` (PE) | `cct.bat` (batch) | `cct-v0.12-windows-x86_64.zip` |

---

## Verifying Release Checksums

After downloading a release, verify its integrity:

**Linux/macOS:**
```bash
sha256sum -c cct-v0.12-*.tar.gz.sha256
```

**Windows (PowerShell):**
```powershell
Get-FileHash cct-v0.12-windows-*.zip -Algorithm SHA256
# Compare output with .sha256 file content
```

---

## Troubleshooting

### Linux: "gcc: command not found"
Install build-essential or development tools for your distribution.

### macOS: "xcrun: error: invalid active developer path"
Install Xcode Command Line Tools: `xcode-select --install`

### Windows: "make: command not found"
- WSL2: Install build-essential in Ubuntu
- MSYS2: Ensure you're in the **UCRT64** terminal (not the plain MSYS2 terminal)
- MinGW: Add MinGW bin directory to PATH

### Windows CMD: "'gcc' is not recognized" when running `cct`
The host C compiler is not in the system PATH. Set the `CC` variable before
invoking `cct`:
```cmd
set CC=C:\msys64\ucrt64\bin\gcc.exe
```
Or add `C:\msys64\ucrt64\bin` to the system `PATH` permanently.

### All Platforms: "Permission denied"
Make the binary executable: `chmod +x cct` (Unix) or check file permissions (Windows)

---

## Development Build

For development (no release package):

```bash
# Build
make

# Clean
make clean

# Run tests
make test

# Format code
make fmt

# Lint code
make lint
```

---

## Cross-Compilation

Cross-compilation is not currently supported. Build on the target platform or use appropriate toolchains.

For creating multi-platform releases, build on each platform separately:
1. Build on Linux → `cct-v0.12-linux-x86_64.tar.gz`
2. Build on macOS → `cct-v0.12-macos-arm64.tar.gz`
3. Build on Windows (MSYS2) → `cct-v0.12-windows-x86_64.tar.gz`

---

## Questions?

- Documentation: `docs/`
- Issues: https://github.com/your-repo/cct/issues
- Release Notes: `docs/release/FASE_12_RELEASE_NOTES.md`
