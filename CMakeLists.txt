cmake_minimum_required(VERSION 3.0)
project(LCEdit)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Find the QtWidgets library
find_package(Qt5Core CONFIG REQUIRED)
find_package(Qt5Widgets CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(KF5TextEditor)

set(LCEDIT_UI src/include.h src/lcedit.ui) # src/include.h is needed because it includes ui_lcedit.h, and AUTOUIC doesn't generate it if src/include.h isn't specified as source

set(LIBLCEDIT_SRC
	src/lcedit.cpp
	${LCEDIT_UI})

set(LCEDIT_SRC
  src/main.cpp
  ${LCEDIT_UI}
)

set (C4GROUP_SRC src/lib/c4group.cpp)
set (PLUGIN_C4GROUP_SRC src/plugins/c4group.cpp src/plugins/c4group.h)

# Create code from a list of Qt designer ui files.
#qt5_wrap_ui(src/main.cpp src/include.h src/lcedit.ui)

add_library(liblcedit STATIC ${LIBLCEDIT_SRC})
add_executable(lcedit ${LCEDIT_SRC})

# Use the Widgets module from Qt 5.
target_link_libraries(liblcedit Qt5::Core Qt5::Widgets)
target_link_libraries(lcedit Qt5::Core Qt5::Widgets liblcedit)

add_library(libc4group SHARED ${C4GROUP_SRC})
target_link_libraries(libc4group Qt5::Core ${ZLIB_LIBRARIES})
add_library(plugin_c4group SHARED ${PLUGIN_C4GROUP_SRC} ${LCEDIT_UI})
target_link_libraries(plugin_c4group Qt5::Widgets libc4group liblcedit)

if (KF5TextEditor_FOUND)
  add_library(plugin_texteditor SHARED src/plugins/texteditor.cpp ${LCEDIT_UI})
  target_link_libraries(plugin_texteditor Qt5::Widgets KF5::TextEditor liblcedit)
endif()

add_library(plugin_search SHARED src/plugins/search.cpp src/plugins/search.ui ${LCEDIT_UI})
target_link_libraries(plugin_search Qt5::Widgets liblcedit)
