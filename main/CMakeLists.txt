idf_component_register(
    SRCS "cnj_Lcd4_N2K.c" "lvgl_port.c" "waveshare-display.cpp" "sw6106.c" "wifi.c" "OneWire.cpp"
    INCLUDE_DIRS "."
    EMBED_TXTFILES web/index.html)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
