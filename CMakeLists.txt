#Copyright (c) 2018-2019, George Tokmaji

#Permission to use, copy, modify, and/or distribute this software for any
#purpose with or without fee is hereby granted, provided that the above
#copyright notice and this permission notice appear in all copies.

#THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
#WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
#MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
#ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
#WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
#ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
#OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

cmake_minimum_required(VERSION 3.0)
project(LCEdit)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 17)

# Find the QtWidgets library
find_package(Qt5Core CONFIG REQUIRED)
find_package(Qt5Widgets CONFIG REQUIRED)
find_package(KF5TextEditor)

if (UNIX)
  set(LCEDIT_PLUGIN_DEPLOY_PATH "${CMAKE_INSTALL_PREFIX}/lib/qt/plugins" CACHE PATH "LCEdit plugin deploy path")
endif()

set(LCEDIT_UI src/lcedit.h src/lcedit.ui) # src/lcedit.h is needed because it includes ui_lcedit.h, and AUTOUIC doesn't generate it if src/lcedit.h isn't specified as source

set(LIBLCEDIT_SRC
	src/lcedit.cpp
	${LCEDIT_UI})

set(LCEDIT_SRC
  src/main.cpp
  ${LCEDIT_UI}
)

find_package(ZLIB)
if (ZLIB_FOUND)
  add_subdirectory(src/lib/cc4group EXCLUDE_FROM_ALL)
endif()

# Create code from a list of Qt designer ui files.
#qt5_wrap_ui(src/main.cpp src/include.h src/lcedit.ui)

add_library(liblcedit STATIC ${LIBLCEDIT_SRC})
add_executable(lcedit ${LCEDIT_SRC})

# Use the Widgets module from Qt 5.
target_link_libraries(liblcedit Qt5::Core Qt5::Widgets)
target_link_libraries(lcedit Qt5::Core Qt5::Widgets liblcedit)

if (ZLIB_FOUND)
  add_library(plugin_c4group SHARED src/plugins/c4group.cpp src/plugins/c4group.json ${LCEDIT_UI})
  target_link_libraries(plugin_c4group Qt5::Widgets cppc4group liblcedit)
  set_target_properties(plugin_c4group PROPERTIES PREFIX "lcedit_")
endif()

if (KF5TextEditor_FOUND)
  add_library(plugin_texteditor SHARED src/plugins/texteditor.cpp src/plugins/texteditor.json ${LCEDIT_UI})
  target_link_libraries(plugin_texteditor Qt5::Widgets KF5::TextEditor liblcedit)
  set_target_properties(plugin_texteditor PROPERTIES PREFIX "lcedit_")
endif()

add_library(plugin_search SHARED src/plugins/search.cpp src/plugins/search.ui src/plugins/search.json ${LCEDIT_UI})
target_link_libraries(plugin_search Qt5::Widgets liblcedit)
set_target_properties(plugin_search PROPERTIES PREFIX "lcedit_")

add_library(plugin_graphicsviewer SHARED src/plugins/graphicsviewer.cpp src/plugins/graphicsviewer.json ${LCEDIT_UI})
target_link_libraries(plugin_graphicsviewer Qt5::Widgets liblcedit)
set_target_properties(plugin_graphicsviewer PROPERTIES PREFIX "lcedit_")

add_library(plugin_workspaces SHARED src/plugins/workspaces.cpp src/plugins/workspaces.json ${LCEDIT_UI})
target_link_libraries(plugin_workspaces Qt5::Widgets liblcedit)
set_target_properties(plugin_workspaces PROPERTIES PREFIX "lcedit_")

if (UNIX)
  install(TARGETS lcedit DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")

  if (ZLIB_FOUND)
	install(TARGETS plugin_c4group plugin_search DESTINATION "${LCEDIT_PLUGIN_DEPLOY_PATH}")
  endif()

  if (KF5TextEditor_FOUND)
    install(TARGETS plugin_texteditor DESTINATION "${LCEDIT_PLUGIN_DEPLOY_PATH}")
  endif()

  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/lcedit.desktop" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications")
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/lcedit.png" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64/apps")
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/lcedit.appdata.xml" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/metainfo/lcedit.appdata.xml")
endif()
