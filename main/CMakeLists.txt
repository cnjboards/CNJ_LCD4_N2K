idf_component_register(SRCS "cnj_Lcd4_N2K.c" 
                            "lvgl_port.c" 
                            "waveshare-display.cpp" 
                            "statusBar.cpp" 
                            "Exio.cpp" 
                            "OneWire.cpp" 
                            "wifi.c" 
                            "images/engine_ok.c" 
                            "images/engine_check.c" 
                        INCLUDE_DIRS "."
                        EMBED_TXTFILES web/index.html)
idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
