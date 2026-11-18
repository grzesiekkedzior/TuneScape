cmake_minimum_required(VERSION 3.21)
project(TuneScape VERSION 6.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Network Multimedia Svg Concurrent)
qt_standard_project_setup()

set(core_dir "${CMAKE_CURRENT_SOURCE_DIR}/TuneScapeCore")
set(source_manifest "${core_dir}/sources.list")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${source_manifest}")
file(STRINGS "${source_manifest}" core_files)
list(TRANSFORM core_files PREPEND "${core_dir}/")
set(CMAKE_AUTOUIC_SEARCH_PATHS "${core_dir}")

function(tunescape_add_executable target)
    qt_add_executable(${target} ${ARGN} ${core_files})
    target_include_directories(${target} PRIVATE "${core_dir}")
    target_link_libraries(${target} PRIVATE
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Multimedia Qt6::Svg Qt6::Concurrent)
endfunction()

tunescape_add_executable(TuneScapeCore WIN32 "${core_dir}/src/main.cpp")

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Test)
    tunescape_add_executable(TuneScapeTests TuneScapeTests/tst_tunescapetests.cpp)
    target_link_libraries(TuneScapeTests PRIVATE Qt6::Test)
    add_test(NAME TuneScapeTests COMMAND TuneScapeTests)
    set_tests_properties(TuneScapeTests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 60)
    qt_add_executable(TuneScapeAudioDiagnostics TuneScapeTests/audio_diagnostics/main.cpp)
    target_link_libraries(TuneScapeAudioDiagnostics PRIVATE
        Qt6::Core Qt6::Gui Qt6::Network Qt6::Multimedia Qt6::Test)
    if(WIN32)
        target_link_libraries(TuneScapeAudioDiagnostics PRIVATE psapi dbghelp)
    endif()
    add_test(NAME TuneScapeAudioDiagnostics COMMAND TuneScapeAudioDiagnostics)
    set_tests_properties(TuneScapeAudioDiagnostics PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        TIMEOUT 400)
endif()

install(TARGETS TuneScapeCore RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
if(Qt6_VERSION VERSION_GREATER_EQUAL 6.5)
    qt_generate_deploy_app_script(
        TARGET TuneScapeCore
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR)
    install(SCRIPT "${deploy_script}")
endif()
