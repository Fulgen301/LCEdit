cmake_minimum_required(VERSION 3.0)
project(LCEdit)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# Find the QtWidgets library
find_package(Qt5Widgets CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(KF5TextEditor)
include_directories("../lc/src")

set(C4GROUP_LIBRARIES
  ${ZLIB_LIBRARIES}
)

set(LCEDIT_SRC
  src/main.cpp
  src/lcedit.cpp
)

set(LCEDIT_UI src/lcedit.ui)

set (C4GROUP_SRC src/lib/c4group.cpp)
set (PLUGIN_C4GROUP_SRC src/plugins/c4group.cpp)

# Create code from a list of Qt designer ui files.
qt5_wrap_ui(src/main.cpp src/include.h src/lcedit.ui)

# Tell CMake to create the helloworld executable
add_executable(lcedit ${LCEDIT_SRC} ${LCEDIT_UI})

# Use the Widgets module from Qt 5.
target_link_libraries(lcedit Qt5::Widgets)

add_library(libc4group SHARED ${C4GROUP_SRC})
target_link_libraries(libc4group Qt5::Core ${C4GROUP_LIBRARIES})
add_library(plugin_c4group SHARED ${PLUGIN_C4GROUP_SRC} ${LCEDIT_UI})
target_link_libraries(plugin_c4group Qt5::Widgets libc4group)

if (KF5TextEditor_FOUND)
  add_library(plugin_texteditor SHARED src/plugins/texteditor.cpp ${LCEDIT_UI})
  target_link_libraries(plugin_texteditor Qt5::Widgets KF5::TextEditor)
endif()
# Install the executable
#install(TARGETS lcedit DESTINATION bin)
