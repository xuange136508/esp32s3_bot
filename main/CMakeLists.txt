set(SOURCES
        "protocols/protocol.cc"
        "system_info.cc"
        "settings.cc"
        "background_task.cc"
        "wifi_manager.cc"
        "display/display.cc"
        "display/lcd_display.cc"
        "myApp.cc"
        "ui/ui.c"
        "ui/img/qrcode.c"
        "ui/img/default1.c"
        "ui/img/default2.c"
        "ui/img/default3.c"
        "board/board.cc"
        "board/wifi_board.cc"
        "board/compact_wifi_board_lcd.cc"
        "board/button.cc"
        "audio_codecs/audio_codec.cc"
        "audio_codecs/no_audio_codec.cc"
        "utils/base64_encoder.cc"
        "http/http_server.cc"
        "http/ai_role.cc"
        "http/http_client.cc"
        "sd/spi_init.c"
        "sd/sd_init.c"
        "main.cc"
        )
set(INCLUDE_DIRS "." "protocols" "ui" "display" "board" "utils" "http" "sd")


list(APPEND SOURCES "protocols/websocket_protocol.cc")
idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS ${INCLUDE_DIRS}
                    EMBED_TXTFILES "../assets/http_server.html"  # 文本格式
                    EMBED_FILES "../assets/role_list.json" 
                    WHOLE_ARCHIVE
                    )

# SPIFFS构建配置
spiffs_create_partition_image(model ../assets/PICTURE FLASH_IN_PROJECT)

# -ffast-math: 这个选项会启用一些优化，以提高浮点运算的速度，可能会牺牲一些精度和正确性。它允许编译器做出一些假设，比如不考虑浮点数的精确性和顺序，这样可以生成更快的代码。
# -O3: 这是一个优化级别的选项，表示编译器会进行最高级别的优化。它会尝试通过多种技术来提高程序的执行速度，通常会增加编译时间和生成的二进制文件大小。
# -Wno-error=format: 这个选项会抑制与格式化字符串相关的警告，将其视为警告而不是错误。也就是说，如果代码中有格式化字符串的警告，编译器不会因为这些警告而中止编译。
# -Wno-format: 这个选项也用于抑制与格式化字符串相关的警告，具体来说是关闭所有格式化相关的警告。
# component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)
