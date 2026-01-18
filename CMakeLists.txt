cmake_minimum_required(VERSION 3.5)

project(D1GraphicsTool VERSION 0.5.0 LANGUAGES CXX)
string(TOLOWER ${PROJECT_NAME} project_name)

set(BIN_TARGET ${PROJECT_NAME})

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CPACK "Configure CPack" ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Concurrent Widgets Multimedia LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Concurrent Widgets Multimedia LinguistTools)

# Translation files
SET(DGT_TS_FILES
        languages/lang_de_DE.ts
        languages/lang_en_US.ts
        languages/lang_es_ES.ts
        languages/lang_fr_FR.ts
        languages/lang_hu_HU.ts
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  qt_create_translation(DGT_QM_FILES ${CMAKE_CURRENT_SOURCE_DIR}/source ${DGT_TS_FILES})
else()
  qt5_create_translation(DGT_QM_FILES ${CMAKE_CURRENT_SOURCE_DIR}/source ${DGT_TS_FILES})
endif()

# Create translations QRC file - ts.qrc
set(TRANSLATIONS_QRC "${CMAKE_CURRENT_BINARY_DIR}/ts.qrc")
file(WRITE ${TRANSLATIONS_QRC} "<RCC>\n    <qresource prefix=\"/\">")
foreach(QM_FILE ${DGT_QM_FILES})
    get_filename_component(QM_FILE_NAME ${QM_FILE} NAME)
#    file(APPEND ${TRANSLATIONS_QRC} "\n\t\t<file alias=\"${QM_FILE_NAME}\">${QM_FILE_NAME}</file>")
    file(APPEND ${TRANSLATIONS_QRC} "\n        <file>${QM_FILE_NAME}</file>")
endforeach()
file(APPEND ${TRANSLATIONS_QRC} "\n    </qresource>\n</RCC>")

# Source files
include_directories(source)

set(PROJECT_SOURCES
        source/builderwidget.cpp
        source/builderwidget.h
        source/builderwidget.ui
        source/celview.cpp
        source/celview.h
        source/celview.ui
        source/checkgfxsetstaskdialog.cpp
        source/checkgfxsetstaskdialog.h
        source/checkgfxsetstaskdialog.ui
        source/cppdataeditdialog.cpp
        source/cppdataeditdialog.h
        source/cppdataeditdialog.ui
        source/cppcolumnchangedialog.cpp
        source/cppcolumnchangedialog.h
        source/cppcolumnchangedialog.ui
        source/cpprowchangedialog.cpp
        source/cpprowchangedialog.h
        source/cpprowchangedialog.ui
        source/cppview.cpp
        source/cppview.h
        source/cppview.ui
        source/cppviewentrywidget.cpp
        source/cppviewentrywidget.h
        source/cppviewentrywidget.ui
        source/config.cpp
        source/config.h
        source/d1cel.cpp
        source/d1cel.h
        source/d1celframe.cpp
        source/d1celframe.h
        source/d1celtileset.cpp
        source/d1celtileset.h
        source/d1celtilesetframe.cpp
        source/d1celtilesetframe.h
        source/d1cl2.cpp
        source/d1cl2.h
        source/d1cl2frame.cpp
        source/d1cl2frame.h
        source/d1clc.cpp
        source/d1clc.h
        source/d1cpp.cpp
        source/d1cpp.h
        source/d1dun.cpp
        source/d1dun.h
        source/d1font.cpp
        source/d1font.h
        source/d1gfx.cpp
        source/d1gfx.h
        source/d1gfxset.cpp
        source/d1gfxset.h
        source/d1image.cpp
        source/d1image.h
        source/d1min.cpp
        source/d1min.h
        source/d1pal.cpp
        source/d1pal.h
        source/d1palhits.cpp
        source/d1palhits.h
        source/d1pcx.cpp
        source/d1pcx.h
        source/d1sla.cpp
        source/d1sla.h
        source/d1smk.cpp
        source/d1smk.h
        source/d1tableset.cpp
        source/d1tableset.h
        source/d1tbl.cpp
        source/d1tbl.h
        source/d1tla.cpp
        source/d1tla.h
        source/d1til.cpp
        source/d1til.h
        source/d1tileset.cpp
        source/d1tileset.h
        source/d1trn.cpp
        source/d1trn.h
        source/d1trs.cpp
        source/d1trs.h
        source/d1wav.cpp
        source/d1wav.h
        source/dungeon/automap.cpp
        source/dungeon/drlg_l1.cpp
        source/dungeon/drlg_l2.cpp
        source/dungeon/drlg_l3.cpp
        source/dungeon/drlg_l4.cpp
        source/dungeon/engine.cpp
        source/dungeon/gendung.cpp
        source/dungeon/interfac.cpp
        source/dungeon/itemdat.cpp
        source/dungeon/items.cpp
        source/dungeon/lighting.cpp
        source/dungeon/misdat.cpp
        source/dungeon/missiles.cpp
        source/dungeon/monstdat.cpp
        source/dungeon/monster.cpp
        source/dungeon/objdat.cpp
        source/dungeon/objects.cpp
        source/dungeon/patchdat.cpp
        source/dungeon/player.cpp
        source/dungeon/questdat.cpp
        source/dungeon/quests.cpp
        source/dungeon/spelldat.cpp
        source/dungeon/themes.cpp
        source/dungeon/towners.cpp
        source/dungeon/trigs.cpp
        source/dungeongeneratedialog.cpp
        source/dungeongeneratedialog.h
        source/dungeongeneratedialog.ui
        source/dungeonsubtilewidget.cpp
        source/dungeonsubtilewidget.h
        source/dungeonsubtilewidget.ui
        source/dungeonresourcedialog.cpp
        source/dungeonresourcedialog.h
        source/dungeonresourcedialog.ui
        source/dungeonsearchdialog.cpp
        source/dungeonsearchdialog.h
        source/dungeonsearchdialog.ui
        source/exportdialog.cpp
        source/exportdialog.h
        source/exportdialog.ui
        source/gfxcomponentdialog.cpp
        source/gfxcomponentdialog.h
        source/gfxcomponentdialog.ui
        source/gfxsetview.cpp
        source/gfxsetview.h
        source/gfxsetview.ui
        source/importdialog.cpp
        source/importdialog.h
        source/importdialog.ui
        source/levelcelview.cpp
        source/levelcelview.h
        source/levelcelview.ui
        source/leveltabframewidget.cpp
        source/leveltabframewidget.h
        source/leveltabframewidget.ui
        source/leveltabsubtilewidget.cpp
        source/leveltabsubtilewidget.h
        source/leveltabsubtilewidget.ui
        source/leveltabtilewidget.cpp
        source/leveltabtilewidget.h
        source/leveltabtilewidget.ui
        source/libsmacker/smacker.cpp
        source/lineeditwidget.cpp
        source/lineeditwidget.h
        source/main.cpp
        source/mainwindow.cpp
        source/mainwindow.h
        source/mainwindow.ui
        source/mergeframesdialog.cpp
        source/mergeframesdialog.h
        source/mergeframesdialog.ui
        source/openasdialog.cpp
        source/openasdialog.h
        source/openasdialog.ui
        source/paintwidget.cpp
        source/paintwidget.h
        source/paintwidget.ui
        source/paletteshowdialog.cpp
        source/paletteshowdialog.h
        source/paletteshowdialog.ui
        source/palettewidget.cpp
        source/palettewidget.h
        source/palettewidget.ui
        source/patchdungeondialog.cpp
        source/patchdungeondialog.h
        source/patchdungeondialog.ui
        source/patchgfxdialog.cpp
        source/patchgfxdialog.h
        source/patchgfxdialog.ui
        source/patchtilesetdialog.cpp
        source/patchtilesetdialog.h
        source/patchtilesetdialog.ui
        source/progressdialog.cpp
        source/progressdialog.h
        source/progressdialog.ui
        source/progresswidget.ui
        source/pushbuttonwidget.cpp
        source/pushbuttonwidget.h
        source/remapdialog.cpp
        source/remapdialog.h
        source/remapdialog.ui
        source/resizedialog.cpp
        source/resizedialog.h
        source/resizedialog.ui
        source/saveasdialog.cpp
        source/saveasdialog.h
        source/saveasdialog.ui
        source/settingsdialog.cpp
        source/settingsdialog.h
        source/settingsdialog.ui
        source/smkaudiowidget.cpp
        source/smkaudiowidget.h
        source/smkaudiowidget.ui
        source/tblview.cpp
        source/tblview.h
        source/tblview.ui
        source/tilesetlightdialog.cpp
        source/tilesetlightdialog.h
        source/tilesetlightdialog.ui
        source/tilesetlightentrywidget.cpp
        source/tilesetlightentrywidget.h
        source/tilesetlightentrywidget.ui
        source/trngeneratecolentrywidget.cpp
        source/trngeneratecolentrywidget.h
        source/trngeneratecolentrywidget.ui
        source/trngeneratedialog.cpp
        source/trngeneratedialog.h
        source/trngeneratedialog.ui
        source/trngeneratepalentrywidget.cpp
        source/trngeneratepalentrywidget.h
        source/trngeneratepalentrywidget.ui
        source/trngeneratepalpopupdialog.cpp
        source/trngeneratepalpopupdialog.h
        source/trngeneratepalpopupdialog.ui
        source/upscaledialog.cpp
        source/upscaledialog.h
        source/upscaledialog.ui
        source/upscaler.cpp
        source/upscaler.h
        source/upscaletaskdialog.cpp
        source/upscaletaskdialog.h
        source/upscaletaskdialog.ui
)

qt_add_resources(RESOURCES
        resources/d1files.qrc
        ${TRANSLATIONS_QRC})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${BIN_TARGET}
        resources/D1GraphicsTool.rc
        ${RESOURCES}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(${BIN_TARGET}
        resources/D1GraphicsTool.rc
        ${RESOURCES}
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(${BIN_TARGET} PRIVATE Qt${QT_VERSION_MAJOR}::Concurrent Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Multimedia)

set_target_properties(${BIN_TARGET} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER d1-graphics-tool.pionere.net
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS ${BIN_TARGET}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${BIN_TARGET})
endif()

if(CPACK AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CPACK_PACKAGE_NAME ${project_name})
  set(CPACK_GENERATOR "7Z")
  # Common *nix files
  set(CPACK_STRIP_FILES TRUE)
  install(TARGETS ${BIN_TARGET} DESTINATION bin)
  set(desktop_file "${PROJECT_SOURCE_DIR}/resources/nix/d1gt.desktop")

  find_program(DFI desktop-file-install)
  if(DFI)
    execute_process(COMMAND ${DFI} --dir=${CMAKE_BINARY_DIR} --set-key=Exec --set-value=${BIN_TARGET} ${desktop_file})
    set(desktop_file "${CMAKE_BINARY_DIR}/d1gt.desktop")
  endif()

  install(FILES "${desktop_file}"
    DESTINATION "share/applications"
  )
  install(FILES "${PROJECT_SOURCE_DIR}/resources/icon.svg"
    DESTINATION "share/icons/hicolor/512x512/apps"
    RENAME "d1gt.svg"
  )

  # -G DEB
  set(CPACK_PACKAGE_CONTACT "${PROJECT_NAME} https://github.com/pionere/d1-graphics-tool")
  set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/pionere/d1-graphics-tool")
  set(CPACK_PACKAGE_DESCRIPTION "Diablo 1 Graphics Tool to manipulate asset files (CEL/CL2/DUN etc.) of Diablo 1.")
  #set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  #if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  #  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6widgets6 (>= 6.2.4), qt6-qpa-plugins (>= 6.2.4)")
  #else()
  #  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5widgets5 (>= 5.15.0)")
  #endif()
  set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)

  # -G RPM
  set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

  find_program(RPMBUILD rpmbuild)
  if(RPMBUILD)
    list(APPEND CPACK_GENERATOR "RPM")
  endif()
  find_program(DPKG dpkg)
  if(DPKG)
    list(APPEND CPACK_GENERATOR "DEB")
  endif()

  set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
  include(CPack)
endif()
