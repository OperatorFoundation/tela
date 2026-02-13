project(tela-esp)

include_directories(${TELA_LIB_SOURCE_DIR} ${GFX_DIR})

set(TELA_ESP_SKETCH_NAME tela-esp)
set(SKETCH_FILE ${TELA_ESP_DIR}/examples/${TELA_ESP_SKETCH_NAME}/${TELA_ESP_SKETCH_NAME}.ino)

set(BOARD "esp32:esp32:esp32s3")
set(PORT "/dev/cu.usbserial-110")
set(REMOTE_PORT "/dev/ttyUSB0")
set(REMOTE_HOST "brandon@bonnie.local")
set(REMOTE_TEMP_DIR "/tmp/esp32_upload")

# Screen configurations
set(SCREEN_CONFIGS "basic_5;advance_5")

# Per-config settings
# Basic 5": ESP32-S3, 4MB flash, no PSRAM, ILI6122/ILI5960
set(BOARD_basic_5 "esp32:esp32:esp32s3")
set(BOARD_OPTIONS_basic_5 "PSRAM=opi,USBMode=hwcdc,FlashSize=4M")
set(FLASH_SIZE_basic_5 "4MB")
set(EXPECTED_PSRAM_basic_5 "true")

# Advance 5": ESP32-S3, 16MB flash, 8MB PSRAM, ST7262
set(BOARD_advance_5 "esp32:esp32:esp32s3")
set(BOARD_OPTIONS_advance_5 "PSRAM=opi,USBMode=hwcdc,FlashSize=16M")
set(FLASH_SIZE_advance_5 "16MB")
set(EXPECTED_PSRAM_advance_5 "true")

# Board verification script
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/verify_board.sh [=[
#!/bin/bash
set -e
PORT="$1"
EXPECTED_FLASH="$2"
EXPECTED_PSRAM="$3"
CONFIG_NAME="$4"

echo "Verifying board on $PORT for config: $CONFIG_NAME"
echo "Expected flash: $EXPECTED_FLASH, Expected PSRAM: $EXPECTED_PSRAM"

# Query chip info
INFO=$(esptool.py --chip esp32s3 --port "$PORT" --baud 460800 --before default_reset flash_id 2>&1) || true

# Check flash size
FLASH_LINE=$(echo "$INFO" | grep -i "Detected flash size" || echo "")
if [ -z "$FLASH_LINE" ]; then
    echo "WARNING: Could not detect flash size, proceeding anyway"
else
    echo "Detected: $FLASH_LINE"
    if echo "$FLASH_LINE" | grep -qi "$EXPECTED_FLASH"; then
        echo "Flash size OK"
    else
        echo "ERROR: Flash size mismatch! Expected $EXPECTED_FLASH"
        echo "Are you sure this is a $CONFIG_NAME board?"
        exit 1
    fi
fi

# Check PSRAM
PSRAM_LINE=$(echo "$INFO" | grep -i "PSRAM" || echo "")
if [ "$EXPECTED_PSRAM" = "true" ]; then
    if [ -z "$PSRAM_LINE" ]; then
        echo "ERROR: Expected PSRAM but none detected!"
        echo "Are you sure this is a $CONFIG_NAME board?"
        exit 1
    else
        echo "PSRAM detected OK: $PSRAM_LINE"
    fi
else
    if echo "$PSRAM_LINE" | grep -qi "PSRAM 8MB\|PSRAM 2MB"; then
        echo "ERROR: PSRAM detected but $CONFIG_NAME should have none!"
        echo "Are you sure this is a $CONFIG_NAME board?"
        exit 1
    else
        echo "No PSRAM OK (expected for $CONFIG_NAME)"
    fi
fi

echo "Board verification passed for $CONFIG_NAME"
]=])
execute_process(COMMAND chmod +x ${CMAKE_CURRENT_BINARY_DIR}/verify_board.sh)

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
    --build-property "compiler.cpp.extra_flags=-std=c++17 -D_BSD_SOURCE -D_DEFAULT_SOURCE -DCROWPANEL_${VARIANT_UPPER} -DCROWPANEL_SIZE=${SIZE} ${CMAKE_CXX__FLAGS_DEBUG}"
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

  # Verify board before upload
  add_custom_target(verify_board_${CONFIG}
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/verify_board.sh
    ${PORT}
    ${FLASH_SIZE_${CONFIG}}
    ${EXPECTED_PSRAM_${CONFIG}}
    ${CONFIG}
    COMMENT "Verifying board matches ${CONFIG} configuration"
  )

  add_custom_target(upload_tela_esp_${CONFIG}
    COMMAND esptool.py --chip esp32s3 --port ${PORT} --baud 460800
    --before default_reset --after hard_reset
    write_flash -z --flash_mode dio --flash_freq 80m --flash_size ${FLASH_SIZE_${CONFIG}}
    0x0 ${CONFIG_OUTPUT_DIR}/${TELA_ESP_SKETCH_NAME}.ino.merged.bin
    COMMENT "Uploading tela-esp (${VARIANT} ${SIZE}\") via esptool"
  )
  add_dependencies(upload_tela_esp_${CONFIG} build_tela_esp_${CONFIG})
  add_dependencies(upload_tela_esp_${CONFIG} verify_board_${CONFIG})

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