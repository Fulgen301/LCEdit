cmake_minimum_required(VERSION 3.0)
project(LCEdit)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Find the QtWidgets library
find_package(Qt5Widgets CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(KF5TextEditor)
include_directories("/home/tokgeo/Git/lc/src")

set(C4GROUP_SRC
  ../lc/src/C4Group.cpp
  ../lc/src/StdBuf.cpp
  ../lc/src/Standard.cpp
  ../lc/src/StdFile.cpp
  ../lc/src/CStdFile.cpp 
  ../lc/src/StdSha1OpenSSL.cpp
  ../lc/src/Standard.cpp
  ../lc/src/C4InputValidation.cpp
  ../lc/src/StdBuf.cpp
  ../lc/src/StdMarkup.cpp
  ../lc/src/zlib/gzio.c
)

set(C4GROUP_LIBRARIES
  ${ZLIB_LIBRARIES}
  ${OPENSSL_LIBRARIES}
)

set(LCEDIT_SRC
  src/main.cpp
  src/lcedit.cpp
  ${C4GROUP_SRC}
)

set (PLUGIN_C4GROUP_SRC src/plugins/c4group.cpp ${C4GROUP_SRC})

# Create code from a list of Qt designer ui files.
set(CMAKE_AUTOUIC ON) # use this if you have CMake 3.x instead of the following

# Tell CMake to create the helloworld executable
add_executable(lcedit ${LCEDIT_SRC})

# Use the Widgets module from Qt 5.
target_link_libraries(lcedit Qt5::Widgets ${C4GROUP_LIBRARIES})

add_library(plugin_c4group SHARED ${PLUGIN_C4GROUP_SRC})
target_link_libraries(plugin_c4group Qt5::Widgets ${C4GROUP_LIBRARIES})

if (KF5TextEditor_FOUND)
  add_library(plugin_texteditor SHARED src/plugins/texteditor.cpp)
  target_link_libraries(plugin_texteditor Qt5::Widgets KF5::TextEditor)
endif()

# Install the executable
#install(TARGETS lcedit DESTINATION bin)
