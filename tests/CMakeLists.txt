function(add_tl4ai_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE tl4ai_core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_tl4ai_test(test_state_manager)
add_tl4ai_test(test_config_manager)
add_tl4ai_test(test_ipc_server)
add_tl4ai_test(test_ai_tool_strategy)
add_tl4ai_test(test_logger)

# TrafficLightWidget test needs Qt6::Widgets through tl4ai_gui.
add_executable(test_traffic_light_widget test_traffic_light_widget.cpp)
target_link_libraries(test_traffic_light_widget PRIVATE tl4ai_gui Qt6::Test)
add_test(NAME test_traffic_light_widget COMMAND test_traffic_light_widget)
set_tests_properties(test_traffic_light_widget PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_settings_dialog
    test_settings_dialog.cpp
)
target_link_libraries(test_settings_dialog PRIVATE tl4ai_gui Qt6::Test)
add_test(NAME test_settings_dialog COMMAND test_settings_dialog)
set_tests_properties(test_settings_dialog PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_floating_window
    test_floating_window.cpp
)
target_link_libraries(test_floating_window PRIVATE tl4ai_gui Qt6::Test)
add_test(NAME test_floating_window COMMAND test_floating_window)
set_tests_properties(test_floating_window PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_tray_icon
    test_tray_icon.cpp
)
target_link_libraries(test_tray_icon PRIVATE tl4ai_gui Qt6::Test)
add_test(NAME test_tray_icon COMMAND test_tray_icon)
set_tests_properties(test_tray_icon PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_sound_utils test_sound_utils.cpp)
target_link_libraries(test_sound_utils PRIVATE tl4ai_gui Qt6::Test)
add_test(NAME test_sound_utils COMMAND test_sound_utils)
set_tests_properties(test_sound_utils PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# tl4ai-ctl integration test needs the ctl binary
add_executable(test_tl4ai_ctl test_tl4ai_ctl.cpp)
target_link_libraries(test_tl4ai_ctl PRIVATE tl4ai_core Qt6::Test)
add_test(NAME test_tl4ai_ctl COMMAND test_tl4ai_ctl)
