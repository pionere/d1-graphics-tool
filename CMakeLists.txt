cmake_minimum_required(VERSION 3.5)

project(D1HeroPlanner VERSION 0.5.0 LANGUAGES CXX)
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
        source/affixsliderwidget.cpp
        source/affixsliderwidget.h
        source/config.cpp
        source/config.h
        source/d1cel.cpp
        source/d1cel.h
        source/d1celframe.cpp
        source/d1celframe.h
        source/d1cl2.cpp
        source/d1cl2.h
        source/d1cl2frame.cpp
        source/d1cl2frame.h
        source/d1gfx.cpp
        source/d1gfx.h
        source/d1hro.cpp
        source/d1hro.h
        source/d1image.cpp
        source/d1image.h
        source/d1pal.cpp
        source/d1pal.h
        source/d1trn.cpp
        source/d1trn.h
        source/dungeon/cursor.cpp
        source/dungeon/engine.cpp
        source/dungeon/gendung.cpp
        source/dungeon/interfac.cpp
        source/dungeon/inv.cpp
        source/dungeon/itemdat.cpp
        source/dungeon/items.cpp
        source/dungeon/lighting.cpp
        source/dungeon/misdat.cpp
        source/dungeon/missiles.cpp
        source/dungeon/monstdat.cpp
        source/dungeon/monster.cpp
        source/dungeon/pack.cpp
        source/dungeon/player.cpp
        source/dungeon/questdat.cpp
        source/dungeon/spelldat.cpp
        source/dungeon/spells.cpp
        source/herodetailswidget.cpp
        source/herodetailswidget.h
        source/herodetailswidget.ui
        source/heroskillscombobox.cpp
        source/heroskillscombobox.h
        source/heroview.cpp
        source/heroview.h
        source/heroview.ui
        source/itemdetailswidget.cpp
        source/itemdetailswidget.h
        source/itemdetailswidget.ui
        source/itempropertieswidget.cpp
        source/itempropertieswidget.h
        source/itempropertieswidget.ui
        source/itemselectordialog.cpp
        source/itemselectordialog.h
        source/itemselectordialog.ui
        source/lineeditwidget.cpp
        source/lineeditwidget.h
        source/main.cpp
        source/mainwindow.cpp
        source/mainwindow.h
        source/mainwindow.ui
        source/monsterdetailswidget.cpp
        source/monsterdetailswidget.h
        source/monsterdetailswidget.ui
        source/openasdialog.cpp
        source/openasdialog.h
        source/openasdialog.ui
        source/palettewidget.cpp
        source/palettewidget.h
        source/palettewidget.ui
        source/popupdialog.cpp
        source/popupdialog.h
        source/popupdialog.ui
        source/progressdialog.cpp
        source/progressdialog.h
        source/progressdialog.ui
        source/progresswidget.ui
        source/pushbuttonwidget.cpp
        source/pushbuttonwidget.h
        source/pvpdetailswidget.cpp
        source/pvpdetailswidget.h
        source/pvpdetailswidget.ui
        source/saveasdialog.cpp
        source/saveasdialog.h
        source/saveasdialog.ui
        source/settingsdialog.cpp
        source/settingsdialog.h
        source/settingsdialog.ui
        source/sidepanelwidget.cpp
        source/sidepanelwidget.h
        source/sidepanelwidget.ui
        source/skilldetailswidget.cpp
        source/skilldetailswidget.h
        source/skilldetailswidget.ui
        source/sliderwidget.cpp
        source/sliderwidget.h
        source/spinboxwidget.cpp
        source/spinboxwidget.h
)

qt_add_resources(RESOURCES
        resources/d1files.qrc
        ${TRANSLATIONS_QRC})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${BIN_TARGET}
        resources/D1HeroPlanner.rc
        ${RESOURCES}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(${BIN_TARGET}
        resources/D1HeroPlanner.rc
        ${RESOURCES}
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(${BIN_TARGET} PRIVATE Qt${QT_VERSION_MAJOR}::Concurrent Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Multimedia)

set_target_properties(${BIN_TARGET} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER d1-hero-planner.pionere.net
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
  set(desktop_file "${PROJECT_SOURCE_DIR}/resources/nix/d1hp.desktop")

  find_program(DFI desktop-file-install)
  if(DFI)
    execute_process(COMMAND ${DFI} --dir=${CMAKE_BINARY_DIR} --set-key=Exec --set-value=${BIN_TARGET} ${desktop_file})
    set(desktop_file "${CMAKE_BINARY_DIR}/d1hp.desktop")
  endif()

  install(FILES "${desktop_file}"
    DESTINATION "share/applications"
  )
  install(FILES "${PROJECT_SOURCE_DIR}/resources/icon.svg"
    DESTINATION "share/icons/hicolor/512x512/apps"
    RENAME "d1hp.svg"
  )

  # -G DEB
  set(CPACK_PACKAGE_CONTACT "${PROJECT_NAME} https://github.com/pionere/d1-hero-planner")
  set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/pionere/d1-hero-planner")
  set(CPACK_PACKAGE_DESCRIPTION "Diablo 1 Hero Planner to design character-builds for Devilx.")
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
