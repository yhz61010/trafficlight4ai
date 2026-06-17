# tl4ai-ctl: lightweight CLI client for the GUI's local IPC socket.
# Links tl4ai_core for the shared Tl4aiClient send logic (also used by the GUI).
add_executable(tl4ai-ctl tl4ai_ctl.cpp)
target_link_libraries(tl4ai-ctl PRIVATE tl4ai_core Qt6::Core Qt6::Network)
