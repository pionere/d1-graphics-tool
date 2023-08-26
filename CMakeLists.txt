cmake_minimum_required(VERSION 3.5)

project(D1GraphicsTool VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Concurrent Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Concurrent Widgets LinguistTools)

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
        source/config.cpp
        source/config.h
        source/d1cel.cpp
        source/d1celframe.cpp
        source/d1celframe.h
        source/d1cel.h
        source/d1celtileset.cpp
        source/d1celtilesetframe.cpp
        source/d1celtilesetframe.h
        source/d1celtileset.h
        source/d1cl2.cpp
        source/d1cl2.h
        source/d1cl2frame.cpp
        source/d1cl2frame.h
        source/d1dun.cpp
        source/d1dun.h
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
        source/dungeon/monstdat.cpp
        source/dungeon/monster.cpp
        source/dungeon/objdat.cpp
        source/dungeon/objects.cpp
        source/dungeon/player.cpp
        source/dungeon/questdat.cpp
        source/dungeon/quests.cpp
        source/dungeon/spelldat.cpp
        source/dungeon/themes.cpp
        source/dungeon/trigs.cpp
        source/dungeondecoratedialog.cpp
        source/dungeondecoratedialog.h
        source/dungeondecoratedialog.ui
        source/dungeongeneratedialog.cpp
        source/dungeongeneratedialog.h
        source/dungeongeneratedialog.ui
        source/dungeonresourcedialog.cpp
        source/dungeonresourcedialog.h
        source/dungeonresourcedialog.ui
        source/dungeonsearchdialog.cpp
        source/dungeonsearchdialog.h
        source/dungeonsearchdialog.ui
        source/exportdialog.cpp
        source/exportdialog.h
        source/exportdialog.ui
        source/gfxsetview.cpp
        source/gfxsetview.h
        source/gfxsetview.ui
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
        source/lineeditwidget.cpp
        source/lineeditwidget.h
        source/main.cpp
        source/mainwindow.cpp
        source/mainwindow.h
        source/mainwindow.ui
        source/openasdialog.cpp
        source/openasdialog.h
        source/openasdialog.ui
        source/paintwidget.cpp
        source/paintwidget.h
        source/paintwidget.ui
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
        source/tblview.cpp
        source/tblview.h
        source/tblview.ui
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
    qt_add_executable(D1GraphicsTool
        resources/D1GraphicsTool.rc
        ${RESOURCES}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(D1GraphicsTool
        resources/D1GraphicsTool.rc
        ${RESOURCES}
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(D1GraphicsTool PRIVATE Qt${QT_VERSION_MAJOR}::Concurrent Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(D1GraphicsTool PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER d1-graphics-tool.savagesteel.net
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS D1GraphicsTool
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(D1GraphicsTool)
endif()
