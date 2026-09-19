<div align="center">

<img src="docs/amethyst_icon_stroke.svg" width="330" alt="Amethyst logo">

<h2>Amethyst</h2>
<p><b>Ориентированный на Windows форк Cremniy — среда низкоуровневой разработки, объединяющая код, байты и бинарники в одном месте.</b></p>

[![Лицензия: GPL-3.0](https://img.shields.io/badge/License-GPLv3-blue?style=flat-square)](LICENSE)
[![Платформа: Windows](https://img.shields.io/badge/Platform-Windows-0078D6?style=flat-square&logo=windows)](https://github.com/Atimenka/Amethyst)

[English](README.md) • Русский

</div>

---

> ### 🧬 Amethyst — это форк Cremniy
>
> **Amethyst** — неофициальный форк проекта [**Cremniy**](https://github.com/munirov/cremniy), изначально созданного [munirov](https://github.com/munirov) и сообществом контрибьюторов.
>
> Оригинальный репозиторий: https://github.com/munirov/cremniy  
> Лицензия оригинала: GPL-3.0.  
> Amethyst распространяется на условиях той же лицензии.

## Что такое Amethyst?

Amethyst — среда разработки низкоуровневого ПО для Windows. Она объединяет редактор кода, HEX-редактор и дизассемблер в едином приложении, избавляя от необходимости постоянно переключаться между разными окнами.

**Для кого это:**

- 🛠 Разработчиков системного ПО
- 🔍 Reverse-инженеров
- 🔐 Специалистов по информационной безопасности
- 📡 Разработчиков embedded-систем

## Чем Amethyst отличается от Cremniy?

Amethyst — это **Windows-first** форк. В то время как оригинальный Cremniy разрабатывался с упором на Linux, Amethyst фокусируется на:

- нативных сборках под Windows (MSVC + vcpkg);
- исправлении платформенных ошибок Windows;
- улучшенной тёмной теме и корректном масштабировании интерфейса (HiDPI);
- инсталляторе для Windows и готовой portable-версии;
- расширении функционала без привязки к зависимостям Linux.

## Возможности ✨

### Доступно сейчас

| Инструмент | Описание |
|---|---|
| 📝 Редактор кода | Написание и редактирование низкоуровневого кода с поддержкой синтаксиса |
| 🔢 HEX-редактор | Просмотр и изменение бинарных данных на уровне байт (патчинг) |
| 🔧 Дизассемблер | Декодирование машинных инструкций в читаемый ассемблер |

### В планах

- 🐛 **Отладчик (Debugger):** пошаговое выполнение, регистры, стек, память
- 🧠 **Визуализация памяти:** наглядные карты расположения и выделения памяти
- 🪟 **Нативная сборка Windows:** MSVC + vcpkg вместо MSYS2
- 🎨 **Улучшенная тёмная тема** и правильное масштабирование DPI
- 📦 **Windows-инсталлятор** и портативная сборка (ZIP)
- 🔌 **API плагинов**

## Установка и сборка инсталлятора

Готовый установщик и portable-версия для Windows доступны для сборки через автоматизированные скрипты в папке `packaging/windows/`.

* 📄 **[Подробное руководство по созданию Setup-инсталлятора для Windows](docs/RU/windows_packaging_guide.md)**
* 🚀 **Быстрая сборка инсталлятора:** запустите `packaging\windows\build_installer.bat` — он скомпилирует Amethyst, автоматически подтянет все DLL и плагины Qt6 через `windeployqt` и соберёт установщик `dist\Amethyst_Setup_x64.exe` через Inno Setup.

## Сборка 🛠️

### Зависимости

| Зависимость | Мин. версия |
|---|---|
| **[CMake](https://cmake.org/download/)** | 3.16 |
| **[Qt](https://www.qt.io/)** | 6.8.2 |
| **[libgit2](https://libgit2.org/)** | 1.x |
| **Компилятор C++** | Поддержка C++17 (MSVC 2022 / GCC / Clang) |

<details>
<summary><b>🪟 Сборка под Windows (MSVC + vcpkg) — Рекомендуется</b></summary>

1. Установите Visual Studio 2022 (с компонентом «Разработка классических приложений на C++»).
2. Установите vcpkg и зависимости:
```powershell
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg install libgit2:x64-windows
```
3. Сборка проекта:
```powershell
cmake -B build -S src -G "Ninja" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE="<путь_к_vcpkg>\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release
```
4. Развёртывание библиотек Qt (windeployqt):
```powershell
windeployqt.exe build\amethyst.exe
```
</details>

<details>
<summary><b>🪟 Сборка под Windows (MinGW / MSYS2)</b></summary>

```bash
pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-libgit2

mkdir build && cd build
cmake -G "MinGW Makefiles" ../src -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```
</details>

<details>
<summary><b>🐧 Linux (Arch Linux / Manjaro)</b></summary>

```bash
sudo pacman -S --needed base-devel cmake ninja qt6-base qt6-svg qt6-tools libgit2

cmake -B build -S src -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
</details>

<details>
<summary><b>🐧 Linux (Debian / Ubuntu)</b></summary>

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build qt6-base-dev qt6-svg-dev qt6-tools-dev qt6-tools-dev-tools libgit2-dev

cmake -B build -S src -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```
</details>

## Разработчик и благодарности

- **Главный разработчик форка:** [Atimenka](https://github.com/Atimenka)
- **Репозиторий проекта:** [https://github.com/Atimenka/Amethyst](https://github.com/Atimenka/Amethyst)
- **Оригинальный проект:** [Cremniy](https://github.com/munirov/cremniy), автор [Munirov](https://github.com/munirov)

## Участие в разработке 👋

Pull request'ы и issues приветствуются. Если вы берете задачу в работу, пожалуйста, оставьте комментарий к соответствующему issue.

## Лицензия 📖

Amethyst распространяется под лицензией **GNU General Public License v3.0**.  
Подробности в файле [LICENSE](LICENSE).

Проект является форком [Cremniy](https://github.com/munirov/cremniy). Все оригинальные уведомления об авторских правах сохранены.
