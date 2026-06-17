# Core library (non-GUI, testable)
add_library(tl4ai_core STATIC
    StateManager.cpp
    ConfigManager.cpp
    IpcServer.cpp
    Tl4aiClient.cpp
)
target_include_directories(tl4ai_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tl4ai_core PUBLIC Qt6::Core Qt6::Network)
target_compile_definitions(tl4ai_core PRIVATE
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)

# GUI library (shared by the app and GUI-focused tests)
add_library(tl4ai_gui STATIC
    TrafficLightWidget.cpp
    FloatingWindow.cpp
    TrayIcon.cpp
    SettingsDialog.cpp
    SoundUtils.cpp
)
target_include_directories(tl4ai_gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tl4ai_gui PUBLIC tl4ai_core Qt6::Widgets Qt6::Multimedia)
target_compile_definitions(tl4ai_gui PRIVATE
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)

# GUI executable
add_executable(trafficlight4ai
    main.cpp
)
qt_add_resources(trafficlight4ai "images"
    PREFIX "/images"
    BASE "${CMAKE_SOURCE_DIR}/resources"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/images/light_off.png"
        "${CMAKE_SOURCE_DIR}/resources/images/light_red.png"
        "${CMAKE_SOURCE_DIR}/resources/images/light_yellow.png"
        "${CMAKE_SOURCE_DIR}/resources/images/light_green.png"
)
qt_add_resources(trafficlight4ai "effects"
    PREFIX "/effects"
    BASE "${CMAKE_SOURCE_DIR}/resources"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/effects/yellow.ogg"
        "${CMAKE_SOURCE_DIR}/resources/effects/green.ogg"
)
set(TS_FILES
    "${CMAKE_SOURCE_DIR}/translations/trafficlight4ai_zh.ts"
    "${CMAKE_SOURCE_DIR}/translations/trafficlight4ai_ja.ts"
)
qt_add_translations(trafficlight4ai
    TS_FILES ${TS_FILES}
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)
qt_add_resources(trafficlight4ai "i18n"
    PREFIX "/i18n"
    BASE "${CMAKE_CURRENT_BINARY_DIR}"
    FILES ${QM_FILES}
)
target_link_libraries(trafficlight4ai PRIVATE tl4ai_gui)
set_target_properties(trafficlight4ai PROPERTIES
    WIN32_EXECUTABLE TRUE
)
if(APPLE)
    set_target_properties(trafficlight4ai PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "trafficlight4ai"
        MACOSX_BUNDLE_GUI_IDENTIFIER "dev.yhz61010.trafficlight4ai"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()
target_compile_definitions(trafficlight4ai PRIVATE
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)
