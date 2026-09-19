cmake_minimum_required(VERSION 3.16)

project(amethyst VERSION 0.3.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==========================================
# Build Speed Optimizations (ccache, mold/lld)
# ==========================================
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Build speedup: ccache detected -> ${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

if(NOT WIN32 AND NOT APPLE)
    find_program(MOLD_PATH mold)
    if(MOLD_PATH)
        message(STATUS "Build speedup: using mold linker -> ${MOLD_PATH}")
        add_link_options("-fuse-ld=mold")
    else()
        find_program(LLD_PATH ld.lld)
        if(LLD_PATH)
            message(STATUS "Build speedup: using lld linker -> ${LLD_PATH}")
            add_link_options("-fuse-ld=lld")
        endif()
    endif()
endif()

if(MSVC)
    add_compile_options(/utf-8)
endif()

find_package(Qt6 6.8.2 REQUIRED COMPONENTS Core Widgets Gui Concurrent LinguistTools Test Svg)

# libgit2 ships a CMake package on some platforms and only a pkg-config file or
# plain headers/libraries on others. Support all three layouts so developers do
# not have to pass platform-specific paths to CMake.
set(AMETHYST_LIBGIT2_TARGET "")
set(AMETHYST_LIBGIT2_INCLUDE_DIRS "")

get_filename_component(AMETHYST_COMPILER_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
get_filename_component(AMETHYST_TOOLCHAIN_PREFIX "${AMETHYST_COMPILER_BIN_DIR}" DIRECTORY)
set(AMETHYST_LIBGIT2_HINTS
    "$ENV{LIBGIT2_ROOT}"
    "$ENV{MINGW_PREFIX}"
    "${AMETHYST_TOOLCHAIN_PREFIX}"
    "$ENV{HOMEBREW_PREFIX}/opt/libgit2"
    "/opt/homebrew/opt/libgit2"
    "/usr/local/opt/libgit2"
)
list(FILTER AMETHYST_LIBGIT2_HINTS EXCLUDE REGEX "^$")

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBGIT2 QUIET IMPORTED_TARGET libgit2)
    if(TARGET PkgConfig::LIBGIT2)
        set(AMETHYST_LIBGIT2_TARGET PkgConfig::LIBGIT2)
    endif()
endif()

if(NOT AMETHYST_LIBGIT2_TARGET)
    find_package(libgit2 CONFIG QUIET HINTS ${AMETHYST_LIBGIT2_HINTS})
    if(TARGET libgit2::libgit2package)
        set(AMETHYST_LIBGIT2_TARGET libgit2::libgit2package)
    elseif(TARGET libgit2::libgit2)
        set(AMETHYST_LIBGIT2_TARGET libgit2::libgit2)
    endif()
endif()

if(NOT AMETHYST_LIBGIT2_TARGET)
    find_path(LIBGIT2_INCLUDE_DIR
        NAMES git2.h
        HINTS ${AMETHYST_LIBGIT2_HINTS}
        PATH_SUFFIXES include
    )
    find_library(LIBGIT2_LIBRARY
        NAMES git2 libgit2
        HINTS ${AMETHYST_LIBGIT2_HINTS}
        PATH_SUFFIXES lib lib64
    )
    if(NOT LIBGIT2_INCLUDE_DIR OR NOT LIBGIT2_LIBRARY)
        message(FATAL_ERROR
            "libgit2 was not found. Install the libgit2 development package "
            "or set LIBGIT2_ROOT to its installation prefix."
        )
    endif()
    set(AMETHYST_LIBGIT2_TARGET "${LIBGIT2_LIBRARY}")
    set(AMETHYST_LIBGIT2_INCLUDE_DIRS "${LIBGIT2_INCLUDE_DIR}")
endif()

if(NOT WIN32)
    find_package(ZLIB QUIET)
    find_package(OpenSSL QUIET)
    find_library(PCRE_LIB NAMES pcre pcre2-8)
    find_library(HTTP_PARSER_LIB NAMES http_parser llhttp)
endif()

set(PROJECT_SOURCES
        main.cpp
)

qt_add_executable(${PROJECT_NAME}
    MANUAL_FINALIZATION
    main.cpp

    # App
    app/IDEWindow/idewindow.cpp
    app/IDEWindow/idewindow.h
    app/WelcomeWindow/WelcomeForm/welcome_form.cpp
    app/WelcomeWindow/WelcomeForm/welcome_form.h
    app/WelcomeWindow/RecentProjectsPage/recent_projects_page.cpp
    app/WelcomeWindow/RecentProjectsPage/recent_projects_page.h
    app/WelcomeWindow/CreateProjectPage/create_project_page.cpp
    app/WelcomeWindow/CreateProjectPage/create_project_page.h
    app/WelcomeWindow/ProjectCard/project_card.cpp
    app/WelcomeWindow/ProjectCard/project_card.h

    # Dialogs
    dialogs/filecreatedialog.cpp
    dialogs/filecreatedialog.h
    dialogs/settingsdialog.cpp
    dialogs/settingsdialog.h
    dialogs/aboutdialog.cpp
    dialogs/aboutdialog.h

    # Widgets
    widgets/clickablelineedit.h
    widgets/filetab.cpp
    widgets/filetab.h
    widgets/filetreepanel.cpp
    widgets/filetreepanel.h
    widgets/terminal/terminalwidget.cpp
    widgets/terminal/terminalwidget.h
    widgets/search/searchpanel.cpp
    widgets/search/searchpanel.h

    # Resources
    resources/amethyst_res.qrc

    # Utils
    utils/utils.h
    utils/utils.cpp
        core/locale/LanguageManager.cpp
        core/locale/LanguageManager.h

    # Core
    core/file/filecontext.h
    core/file/FileDataBuffer.cpp
    core/file/FileDataBuffer.h
    core/file/filemanager.cpp
    core/file/filemanager.h

    core/project/projects_history_manager.cpp
    core/project/projects_history_manager.h

    core/disk/disk_image_parser.cpp
    core/disk/disk_image_parser.h

    core/search/searchengine.cpp
    core/search/searchengine.h

    core/settings/appsettings.cpp
    core/settings/appsettings.h

    core/icons/iconprovider.cpp
    core/icons/iconprovider.h

    core/settings/exclusionfilterproxymodel.cpp
    core/settings/exclusionfilterproxymodel.h

    core/modules/ModuleManager.h
    core/modules/ModuleManager.cpp
    core/modules/TabBase.h
    core/modules/WindowBase.h
    core/modules/ReferenceBase.h

    # Git
    core/git/gitmanager.cpp
    core/git/gitmanager.h

    # UI
    ui/FilesTabWidget/filestabwidget.cpp
    ui/FilesTabWidget/filestabwidget.h
    ui/ToolsTabWidget/toolstabwidget.cpp
    ui/ToolsTabWidget/toolstabwidget.h

    # UI Menubar
    ui/MenuBar/menubarbuilder.h ui/MenuBar/menubarbuilder.cpp
    ui/MenuBar/menufactory.h ui/MenuBar/menufactory.cpp
    ui/MenuBar/basemenu.h
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/core/file
    ${CMAKE_CURRENT_SOURCE_DIR}/core/git
    ${CMAKE_CURRENT_SOURCE_DIR}/core/project
    ${CMAKE_CURRENT_SOURCE_DIR}/core/disk
    ${CMAKE_CURRENT_SOURCE_DIR}/core/settings
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/ToolsTabWidget
    ${CMAKE_CURRENT_SOURCE_DIR}/FilesTabWidget
    ${CMAKE_CURRENT_SOURCE_DIR}/app/WelcomeWindow/WelcomeForm
    ${CMAKE_CURRENT_SOURCE_DIR}/app/WelcomeWindow/RecentProjectsPage
    ${CMAKE_CURRENT_SOURCE_DIR}/app/WelcomeWindow/CreateProjectPage
    ${CMAKE_CURRENT_SOURCE_DIR}/app/WelcomeWindow/ProjectCard
)

# Precompile common Qt headers to drastically accelerate compilation
target_precompile_headers(${PROJECT_NAME} PRIVATE
    <QtCore/QObject>
    <QtCore/QString>
    <QtCore/QVector>
    <QtCore/QList>
    <QtCore/QByteArray>
    <QtCore/QMap>
    <QtCore/QHash>
    <QtCore/QDebug>
    <QtWidgets/QWidget>
    <QtWidgets/QLabel>
    <QtWidgets/QPushButton>
    <QtGui/QIcon>
    <QtGui/QPixmap>
)

file(GLOB_RECURSE PHOICONS_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/phoicons/*"
)

qt_add_resources(${PROJECT_NAME} "phoicons"
    PREFIX "/icons"
    BASE "resources/icons"
    FILES ${PHOICONS_FILES}
)

# - - Libs - -
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/CodeEditor/ ${CMAKE_BINARY_DIR}/libs/CodeEditor/)
target_link_libraries(${PROJECT_NAME} PRIVATE CodeEditor)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/libs/HexEditor/ ${CMAKE_BINARY_DIR}/libs/HexEditor/)
target_link_libraries(${PROJECT_NAME} PRIVATE HexEditor)

# - - Modules - -
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Modules/References/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Modules/Tabs/)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Modules/Windows/)

# - - Menus - -
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ui/MenuBar/Menus/)

if(AMETHYST_LIBGIT2_INCLUDE_DIRS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${AMETHYST_LIBGIT2_INCLUDE_DIRS})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Concurrent
    Qt6::Svg
    ${AMETHYST_LIBGIT2_TARGET}
)

if(NOT WIN32)
    if(HTTP_PARSER_LIB)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${HTTP_PARSER_LIB})
    endif()
    if(PCRE_LIB)
        target_link_libraries(${PROJECT_NAME} PRIVATE ${PCRE_LIB})
    endif()
    if(TARGET OpenSSL::SSL)
        target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()
    if(TARGET ZLIB::ZLIB)
        target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)
    endif()
endif()

set(APP_LANGUAGES
        en
        ru
)

list(JOIN APP_LANGUAGES ";" APP_LANGUAGES_STR)

configure_file(
        ${CMAKE_SOURCE_DIR}/config.h.in
        ${CMAKE_BINARY_DIR}/config.h
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_BINARY_DIR}
)

set(TS_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/locale/translations/app_ru.ts
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/locale/translations/qtbase_ru.ts
)

qt6_add_translation(QM_FILES ${TS_FILES})

add_custom_target(translations ALL DEPENDS ${QM_FILES})
add_dependencies(${PROJECT_NAME} translations)

if (APPLE)
    set(TRANSLATIONS_DIR
            "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/Resources/translations"
    )
else ()
    set(TRANSLATIONS_DIR
            "${CMAKE_BINARY_DIR}/Resources/translations"
    )
endif ()

file(MAKE_DIRECTORY ${TRANSLATIONS_DIR})

foreach(QM_FILE ${QM_FILES})
    get_filename_component(QM_FILENAME ${QM_FILE} NAME)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QM_FILE}"
            "${TRANSLATIONS_DIR}/${QM_FILENAME}"
    )
endforeach()

find_program(WINDEPLOYQT_EXECUTABLE windeployqt)

if(WIN32 AND WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${WINDEPLOYQT_EXECUTABLE}
                $<TARGET_FILE:${PROJECT_NAME}>
        COMMENT "Deploying Qt dependencies with windeployqt"
    )
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    QT_NO_WINDOWS_APP_MANIFEST TRUE
)

if(WIN32)
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/resources/amethyst.rc")
    target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/resources")
endif()

if(APPLE)
    set(ICNS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/amethyst.icns")
    set(MACOSX_BUNDLE_ICON_FILE amethyst.icns)
    set_source_files_properties(${ICNS_FILE} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(${PROJECT_NAME} PRIVATE ${ICNS_FILE})

    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE amethyst.icns
        MACOSX_BUNDLE_BUNDLE_NAME "Amethyst"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.atimenka.amethyst"
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/Info.plist.in"
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/resources/macos/Assets.car
            $<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/Assets.car
        COMMENT "Copying prebuilt Tahoe icon asset"
    )
endif()

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Guard against missing app.exe.manifest.in when cross-compiling with host Linux Qt
set(CAN_FINALIZE TRUE)
if(WIN32 AND NOT CMAKE_HOST_WIN32)
    if(NOT EXISTS "${Qt6_DIR}/windows/app.exe.manifest.in" AND NOT EXISTS "${Qt6Core_DIR}/../Qt6/windows/app.exe.manifest.in")
        set(CAN_FINALIZE FALSE)
    endif()
endif()

if(CAN_FINALIZE)
    qt_finalize_executable(${PROJECT_NAME})
endif()
