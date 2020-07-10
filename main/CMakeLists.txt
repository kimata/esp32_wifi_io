idf_component_register(SRCS "esp32_wifi_io.c" "wifi_task.c" "http_task.c" "http_ota_handler.c" "part_info.c"
                       INCLUDE_DIRS "."
                       EMBED_FILES "../angular/dist/esp32-wifi-io/index.html"
                                   "../angular/dist/esp32-wifi-io/runtime.js.gz"
                                   "../angular/dist/esp32-wifi-io/main.js.gz"
                                   "../angular/dist/esp32-wifi-io/polyfills.js.gz"
                                   "../angular/dist/esp32-wifi-io/scripts.js.gz"
                                   "../angular/dist/esp32-wifi-io/styles.css.gz"
                                   "../angular/dist/esp32-wifi-io/favicon.ico.gz")
