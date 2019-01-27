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
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(KF5TextEditor)

if (UNIX)
  set(LCEDIT_PLUGIN_DEPLOY_PATH "${CMAKE_INSTALL_PREFIX}/lib/qt/plugins" CACHE PATH "LCEdit plugin deploy path")
endif()

set(LCEDIT_UI src/include.h src/lcedit.ui) # src/include.h is needed because it includes ui_lcedit.h, and AUTOUIC doesn't generate it if src/include.h isn't specified as source

set(LIBLCEDIT_SRC
	src/lcedit.cpp
	src/lib/lcsettings.cpp
	${LCEDIT_UI})

set(LCEDIT_SRC
  src/main.cpp
  ${LCEDIT_UI}
)

set (LIBC4GROUP_SRC src/lib/c4group.cpp)

# Create code from a list of Qt designer ui files.
#qt5_wrap_ui(src/main.cpp src/include.h src/lcedit.ui)

add_library(liblcedit STATIC ${LIBLCEDIT_SRC})
add_executable(lcedit ${LCEDIT_SRC})

# Use the Widgets module from Qt 5.
target_link_libraries(liblcedit Qt5::Core Qt5::Widgets)
target_link_libraries(lcedit Qt5::Core Qt5::Widgets liblcedit)

add_library(libc4group SHARED ${LIBC4GROUP_SRC})
target_link_libraries(libc4group Qt5::Core ${ZLIB_LIBRARIES})

add_library(plugin_c4group SHARED src/plugins/c4group.cpp src/plugins/c4group.json ${LCEDIT_UI})
target_link_libraries(plugin_c4group Qt5::Widgets libc4group liblcedit)
set_target_properties(plugin_c4group PROPERTIES PREFIX "lcedit_")

if (KF5TextEditor_FOUND)
  add_library(plugin_texteditor SHARED src/plugins/texteditor.cpp src/plugins/texteditor.json ${LCEDIT_UI})
  target_link_libraries(plugin_texteditor Qt5::Widgets KF5::TextEditor liblcedit)
  set_target_properties(plugin_texteditor PROPERTIES PREFIX "lcedit_")
endif()

add_library(plugin_search SHARED src/plugins/search.cpp src/plugins/search.ui src/plugins/search.ui ${LCEDIT_UI})
target_link_libraries(plugin_search Qt5::Widgets liblcedit)
set_target_properties(plugin_search PROPERTIES PREFIX "lcedit_")

if (UNIX)
  install(TARGETS liblcedit libc4group DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/lcedit")
  install(TARGETS lcedit DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
  install(TARGETS plugin_c4group plugin_search DESTINATION "${LCEDIT_PLUGIN_DEPLOY_PATH}")
  if (KF5TextEditor_FOUND)
	install(TARGETS plugin_texteditor DESTINATION "${LCEDIT_PLUGIN_DEPLOY_PATH}")
  endif()
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/lcedit.desktop" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications")
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/lcedit.png" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/64x64/apps")
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/res/lcedit.appdata.xml" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/metainfo/lcedit.appdata.xml")
endif()
