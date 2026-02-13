project(tela-esp)

include_directories(${TELA_LIB_SOURCE_DIR} ${GFX_DIR})

set(TELA_ESP_SKETCH_NAME tela-esp)
set(SKETCH_FILE ${TELA_ESP_DIR}/examples/${TELA_ESP_SKETCH_NAME}/${TELA_ESP_SKETCH_NAME}.ino)

set(PORT "/dev/cu.usbserial-110")
set(REMOTE_PORT "/dev/ttyUSB0")
set(REMOTE_HOST "brandon@bonnie.local")
set(REMOTE_TEMP_DIR "/tmp/esp32_upload")

# Screen configurations: variant_size
set(SCREEN_CONFIGS "basic_5;advance_5")

# Per-config settings
# Basic 5": ESP32-S3, 4MB flash, no PSRAM, ILI6122/ILI5960
set(BOARD_basic_5 "esp32:esp32:esp32s3")
set(BOARD_OPTIONS_basic_5 "USBMode=hwcdc,FlashSize=4M")
set(FLASH_SIZE_basic_5 "4MB")

# Advance 5": ESP32-S3, 16MB flash, 8MB PSRAM, ST7262
set(BOARD_advance_5 "esp32:esp32:esp32s3")
set(BOARD_OPTIONS_advance_5 "PSRAM=opi,USBMode=hwcdc,FlashSize=16M")
set(FLASH_SIZE_advance_5 "16MB")

foreach(CONFIG ${SCREEN_CONFIGS})
  string(REPLACE "_" ";" CONFIG_PARTS ${CONFIG})
  list(GET CONFIG_PARTS 0 VARIANT)
  list(GET CONFIG_PARTS 1 SIZE)
  string(TOUPPER ${VARIANT} VARIANT_UPPER)

  set(CONFIG_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/esp32s3/${TELA_ESP_SKETCH_NAME}-${CONFIG})

  add_custom_target(build_tela_esp_${CONFIG} ALL
    COMMAND arduino-cli compile
    --fqbn ${BOARD_${CONFIG}}
    --output-dir ${CONFIG_OUTPUT_DIR}
    --build-property "compiler.cpp.extra_flags=-std=c++17 -D_BSD_SOURCE -D_DEFAULT_SOURCE -DCROWPANEL_${VARIANT_UPPER} -DCROWPANEL_SIZE=${SIZE}"
    --board-options "${BOARD_OPTIONS_${CONFIG}}"
    --library ${GFX_DIR}
    --library ${VTERM_DIR}
    --library ${TRANSMISSION}
    --library ${TRANSMISSION_ARDUINO}
    --library ${TRANSMISSION_ESP32}
    --library ${TELA_LIB_DIR}
    --library ${TELA_ARDUINO_DIR}
    --library ${TELA_ESP_DIR}
    --library ${ONDA}
    --library ${ONDA_ARDUINO}
    ${SKETCH_FILE}
    WORKING_DIRECTORY ${TELA_ESP_SOURCE_DIR}
    COMMENT "Building tela-esp (${VARIANT} ${SIZE}\") with arduino-cli"
  )

  add_custom_target(upload_tela_esp_${CONFIG}
    COMMAND esptool.py --chip esp32s3 --port ${PORT} --baud 460800
    --before default_reset --after hard_reset
    write_flash -z --flash_mode dio --flash_freq 80m --flash_size ${FLASH_SIZE_${CONFIG}}
    0x0 ${CONFIG_OUTPUT_DIR}/${TELA_ESP_SKETCH_NAME}.ino.merged.bin
    COMMENT "Uploading tela-esp (${VARIANT} ${SIZE}\") via esptool"
  )
  add_dependencies(upload_tela_esp_${CONFIG} build_tela_esp_${CONFIG})

  add_custom_target(remote_upload_tela_esp_${CONFIG}
    COMMAND ssh ${REMOTE_HOST} "mkdir -p ${REMOTE_TEMP_DIR}"
    COMMAND scp ${CONFIG_OUTPUT_DIR}/${TELA_ESP_SKETCH_NAME}.ino.bootloader.bin ${REMOTE_HOST}:${REMOTE_TEMP_DIR}/${TELA_ESP_SKETCH_NAME}.bootloader.bin
    COMMAND scp ${CONFIG_OUTPUT_DIR}/${TELA_ESP_SKETCH_NAME}.ino.partitions.bin ${REMOTE_HOST}:${REMOTE_TEMP_DIR}/${TELA_ESP_SKETCH_NAME}.partitions.bin
    COMMAND scp ${CONFIG_OUTPUT_DIR}/${TELA_ESP_SKETCH_NAME}.ino.bin ${REMOTE_HOST}:${REMOTE_TEMP_DIR}/${TELA_ESP_SKETCH_NAME}.bin
    COMMAND ssh ${REMOTE_HOST} "arduino-cli upload --fqbn ${BOARD_${CONFIG}} --port ${REMOTE_PORT} --input-file ${REMOTE_TEMP_DIR}/${TELA_ESP_SKETCH_NAME}.bin"
    COMMAND ssh ${REMOTE_HOST} "rm -f ${REMOTE_TEMP_DIR}/${TELA_ESP_SKETCH_NAME}.bin"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Uploading tela-esp (${VARIANT} ${SIZE}\") to ESP32 via ${REMOTE_HOST}"
    VERBATIM
  )
  add_dependencies(remote_upload_tela_esp_${CONFIG} build_tela_esp_${CONFIG})
endforeach()