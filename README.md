<div align="center">

<img src="docs/amethyst_icon_stroke.svg" width="330" alt="Amethyst logo">

<h2>Amethyst</h2>
<p><b>A Windows-focused fork of Cremniy — a low-level IDE that keeps code, bytes, and binaries in one place.</b></p>

[![License: GPL-3.0](https://img.shields.io/badge/License-GPLv3-blue?style=flat-square)](LICENSE)
[![Platform: Windows](https://img.shields.io/badge/Platform-Windows-0078D6?style=flat-square&logo=windows)](https://github.com/Atimenka/Amethyst)

</div>

---

> ### 🧬 Amethyst is a fork of Cremniy
>
> **Amethyst** is an unofficial fork of [**Cremniy**](https://github.com/munirov/cremniy), originally created by [munirov](https://github.com/munirov) and contributors.
>
> Original repository: https://github.com/munirov/cremniy  
> Original license: GPL-3.0.  
> Amethyst is distributed under the same license.

## What is Amethyst?

Amethyst is a low-level development environment for Windows. It combines a code editor, a HEX editor, and a disassembler into a single application, so you don't have to switch between multiple windows.

**Who it's for:**

- system software developers
- reverse engineers
- cybersecurity specialists
- embedded systems developers

## How is it different from Cremniy?

Amethyst is a **Windows-first** fork. While the original Cremniy was primarily developed for Linux, Amethyst focuses on:

- native Windows builds
- fixing platform-specific bugs
- improved dark theme and DPI scaling
- a Windows installer and portable version
- expanding functionality without Linux dependencies

## Features

### Available now

| Tool | Description |
|---|---|
| Code editor | Write and edit low-level code with syntax highlighting |
| HEX editor | Inspect and modify binary data at the byte level |
| Disassembler | Decode machine instructions into readable assembly |

### Planned

- 🐛 Debugger: step-through execution, registers, stack, memory
- 🧠 Memory visualization: layout maps and allocation views
- 🪟 Native Windows build (MSVC + vcpkg instead of MSYS2)
- 🎨 Improved dark theme and proper DPI scaling
- 📦 Windows installer and portable build
- 🔌 Plugin API

## Installation & Windows Installer

Standalone portable bundles and setup installers for Windows can be generated using the scripts in `packaging/windows/`.

* 📄 **[Windows Packaging & Installer Guide](docs/RU/windows_packaging_guide.md)**
* 🚀 **One-Click Installer Builder:** run `packaging\windows\build_installer.bat` (or `package.ps1`) to compile in Release, deploy all Qt6 runtime DLLs with `windeployqt`, and compile the `Amethyst_Setup_x64.exe` installer with Inno Setup.

## Developer & Credits

- **Fork Developer:** [Atimenka](https://github.com/Atimenka)
- **GitHub Repository:** [https://github.com/Atimenka/Amethyst](https://github.com/Atimenka/Amethyst)
- **Original Project:** [Cremniy](https://github.com/munirov/cremniy) by [Munirov](https://github.com/munirov)

## Contributing

Pull requests and issues are welcome. If you take on a task, please comment on the corresponding issue to avoid duplicate work.

## License

Amethyst is distributed under the **GNU General Public License v3.0**.  
See [LICENSE](LICENSE) for details.

This is a fork of [Cremniy](https://github.com/munirov/cremniy). All original copyright notices are preserved.  
See [NOTICE](NOTICE) for attribution.
