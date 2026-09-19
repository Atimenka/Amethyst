# 📦 Руководство по сборке Windows .exe и созданию Setup-установщика

В этом руководстве подробно описано, как скомпилировать **Amethyst IDE** в нативный исполняемый файл `.exe` для Windows и упаковать его в профессиональный дистрибутив / инсталлятор (`Setup.exe`), который:
1. Содержит все необходимые библиотеки **Qt6** и плагины — пользователю **не требуется** устанавливать Qt6 вручную;
2. Создаёт красивый ярлык на Рабочем столе и в меню «Пуск» с официальной иконкой Amethyst;
3. Добавляет пункт *«Открыть в Amethyst»* в контекстное меню Проводника Windows;
4. Регистрируется в «Панели управления» / «Установленных приложениях» Windows с деинсталлятором.

---

## ❓ Почему «голый» `amethyst.exe` не запускается на чистой Windows?

Если просто скомпилировать C++/Qt проект через компилятор, на выходе получается один файл `amethyst.exe`. Однако при попытке его запуска на машине без установленного Qt Windows выдаст ошибку:

> *"Система не обнаружила Qt6Widgets.dll / Qt6Core.dll"*  
> или  
> *"This application failed to start because no Qt platform plugin could be initialized."*

### Что именно нужно исполняемому файлу:
* **Базовые динамические библиотеки Qt6:** `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Widgets.dll`, `Qt6Svg.dll`, `Qt6Concurrent.dll`.
* **Оконный плагин Windows:** папка `platforms/` с файлом `qwindows.dll` (без него Qt не может создать ни одно окно в ОС Windows).
* **Плагины форматов изображений:** папка `imageformats/` с файлами `qsvg.dll`, `qico.dll` (без них иконки интерфейса Amethyst не будут отображаться).
* **Рантайм компилятора:**
  * Для MinGW: `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`.
  * Для Visual Studio (MSVC): `vcruntime140.dll`, `msvcp140.dll` (или установка официального пакета Microsoft Visual C++ Redistributable).

> **Примечание о ресурсах программы:** Все встроенные стили, темы оформлений, шрифты (JetBrains Mono) и иконки Amethyst уже скомпилированы непосредственно внутрь бинарного кода через механизм Qt Resources (`.qrc`). Поэтому они никогда не потеряются.

---

## 🚀 Способ 1: Автоматическая сборка инсталлятора «в 1 клик»

В репозитории подготовлен готовый пакет автоматизации.

### Необходимые инструменты:
1. **Qt 6** (с MinGW или MSVC) — [официальный установщик Qt](https://www.qt.io/download-qt-installer) или через MSYS2;
2. **CMake** (3.16+) — [cmake.org](https://cmake.org/download/);
3. **Inno Setup 6** (бесплатный инструмент сборки инсталляторов) — [jrsoftware.org/isdl.php](https://jrsoftware.org/isdl.php).

### Запуск сборки:
Просто запустите из проводника или терминала Windows:
```cmd
packaging\windows\build_installer.bat
```
или через PowerShell:
```powershell
powershell -ExecutionPolicy Bypass -File packaging\windows\package.ps1
```

### Что сделает скрипт:
1. Скомпилирует Amethyst в максимальном режиме оптимизации (`Release`);
2. Создаст изолированную директорию сборки `packaging\windows\bundle\`;
3. Запустит утилиту `windeployqt.exe`, которая автоматически найдёт и скопирует все нужные DLL и плагины;
4. Скопирует библиотеки рантайма C++;
5. Запустит компилятор Inno Setup (`ISCC.exe`) по сценарию `packaging\windows\amethyst_setup.iss`;
6. Создаст готовый файл **`dist\Amethyst_Setup_x64.exe`** и откроет папку с ним!

---

## 🐧 Способ 2: Сборка Windows Setup-инсталлятора прямо из Arch Linux / Arch WSL

Если вы работаете в **Arch Linux** или **Arch Linux в WSL**, вам **не нужно** переключаться в Windows, чтобы собрать готовый Setup-установщик! В репозитории есть специальный скрипт:

```bash
./packaging/windows/build_installer_arch.sh
```

### Вариант А: Если вы работаете в Arch Linux WSL (Windows Subsystem for Linux)
1. Установите Inno Setup 6 в Windows (он по умолчанию устанавливается в `C:\Program Files (x86)\Inno Setup 6\ISCC.exe`);
2. В терминале Arch WSL запустите:
   ```bash
   ./packaging/windows/build_installer_arch.sh
   ```
3. Скрипт через WSL Interop сам вызовет компилятор `ISCC.exe` и утилиту `windeployqt`, соберёт все библиотеки Qt6 и скомпилирует инсталлятор **`dist/Amethyst_Setup_x64.exe`**!

### Вариант Б: Если вы работаете в чистом Arch Linux (без Windows)
1. Установите Wine и утилиту распаковки:
   ```bash
   sudo pacman -S --needed wine innoextract
   ```
2. Запустите скрипт сборщика:
   ```bash
   ./packaging/windows/build_installer_arch.sh
   ```
3. Скрипт сам автоматически скачает портативный компилятор Inno Setup 6, упакует все DLL Qt6, создаст нужные ярлыки, контекстное меню реестра и скомпилирует готовый Windows-установщик `Amethyst_Setup_x64.exe` прямо в Linux!

---

## 🛠 Способ 3: Ручная пошаговая сборка и развёртывание

Если вы хотите выполнить все этапы вручную через командную строку:

### Шаг 1. Сборка исполняемого файла в Release
Откройте терминал `x64 Native Tools Command Prompt for VS` (для MSVC) или консоль MinGW64:
```cmd
cd C:\path\to\Amethyst
cmake -B build_win -S src -DCMAKE_BUILD_TYPE=Release
cmake --build build_win --config Release --parallel
```
В результате появится исполняемый файл: `build_win\Release\amethyst.exe` (или `build_win\amethyst.exe`).

### Шаг 2. Развёртывание библиотек с помощью `windeployqt`
Создайте чистую папку для готовой программы:
```cmd
mkdir C:\Amethyst_Portable
copy build_win\Release\amethyst.exe C:\Amethyst_Portable\
```
Вызовите утилиту `windeployqt`:
```cmd
windeployqt.exe --release --compiler-runtime C:\Amethyst_Portable\amethyst.exe
```
Теперь папку `C:\Amethyst_Portable\` можно заархивировать в `.zip` и передать на любой компьютер с Windows 10/11 — Amethyst запустится без установки какого-либо стороннего софта.

### Шаг 3. Компиляция Setup-инсталлятора через Inno Setup
Скопируйте содержимое portable-папки в `packaging\windows\bundle\`:
```cmd
xcopy /E /I C:\Amethyst_Portable packaging\windows\bundle
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" packaging\windows\amethyst_setup.iss
```
Готовый установщик сохранится в папке `dist\Amethyst_Setup_x64.exe`.

---

## 📋 Особенности созданного инсталлятора

* **Выбор пути установки:** по умолчанию в `C:\Program Files\Amethyst` (или в локальную директорию пользователя `%LocalAppData%\Programs\Amethyst`, если запуск выполнен без прав администратора).
* **Создание ярлыков:** на Рабочем столе и в главном меню Пуск с привязкой иконки `amethyst.ico`.
* **Ассоциация и интеграция:** при установке пользователь может отметить галочку «Добавить пункт в контекстное меню Проводника», чтобы открывать проекты правой кнопкой мыши по любой папке.
* **Удаление программы:** в комплекте идёт деинсталлятор `unins000.exe`, корректно очищающий все файлы и записи реестра.
