cmake_minimum_required(VERSION 3.16)

set(MYPROJ TuneScape )

project(${MYPROJ} VERSION 1.1.6 LANGUAGES CXX)

# Find includes in the build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Turn on automatic invocation of the MOC, UIC & RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Add a compiler flag
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Make this a GUI application on Windows
if(WIN32)
  set(CMAKE_WIN32_EXECUTABLE ON)
endif()

# Find the QtWidgets library
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Multimedia)

qt_standard_project_setup()

add_subdirectory(include)

add_subdirectory(src)

# Tell CMake to create the helloworld executable
qt_add_executable(${MYPROJ} ${${MYPROJ}_SOURCES} ${${MYPROJ}_HEADERS} mainwindow.ui res.qrc)

# Add the Qt5 Widgets for linking
target_link_libraries(${MYPROJ} PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::Multimedia)

install(TARGETS ${MYPROJ}
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET ${MYPROJ}
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
