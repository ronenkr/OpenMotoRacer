# The game (port/src, as the PC builds it, with OMR_ESP32) and the board's side of it.
# idf.py -DBOARDTEST=1 build: the board's bring-up test instead (boardtest.c).
set(PORT ${CMAKE_CURRENT_LIST_DIR}/../../port/src)
set(REQ esp_driver_gpio esp_driver_i2c esp_driver_i2s esp_driver_spi esp_driver_ledc
        esp_driver_sdmmc esp_lcd fatfs esp_timer esp_psram vfs esp_driver_usb_serial_jtag mbedtls)
if(BOARDTEST)
  idf_component_register(SRCS main.c board.c boardtest.c INCLUDE_DIRS . PRIV_REQUIRES ${REQ})
  return()
endif()

file(GLOB GAME ${PORT}/*.c)
list(REMOVE_ITEM GAME ${PORT}/mister.c ${PORT}/prof.c)
idf_component_register(SRCS board.c sdl_esp32.c omr_esp32.c ${GAME} ${PORT}/third_party/decoders.c
                       INCLUDE_DIRS . sdl ${PORT}
                       PRIV_REQUIRES ${REQ}
                       LDFRAGMENTS linker.lf)
# the game's arithmetic as on the PC (NOTES §4.23): no fused multiply-add, char signed,
# signed overflow wrapping; its warnings are the PC's to mend, not errors here
target_compile_options(${COMPONENT_LIB} PRIVATE -fwrapv -ffp-contract=off -fsigned-char
                       -Wno-error -Wno-unused-variable -Wno-format-truncation)
target_compile_definitions(${COMPONENT_LIB} PRIVATE OMR_ESP32=1)
set_source_files_properties(${PORT}/main.c PROPERTIES COMPILE_DEFINITIONS main=omr_main)
set_source_files_properties(${PORT}/third_party/decoders.c PROPERTIES COMPILE_OPTIONS -w)
# the bikes' physics in single precision (the FPU's; doubles are software here): NOTES §4.26
set_source_files_properties(${PORT}/bikephys.c ${PORT}/cpu.c PROPERTIES
                            COMPILE_OPTIONS -fsingle-precision-constant
                            COMPILE_DEFINITIONS OMR_FAST_PHYS=1)
# and what is seen and heard (port/src/fastfloat.h)
foreach(f r3d bike3d pose race_sound camera effects shadow horizon)
  set_source_files_properties(${PORT}/${f}.c PROPERTIES COMPILE_OPTIONS -fsingle-precision-constant
                              COMPILE_DEFINITIONS OMR_FAST_FLOAT=1)
endforeach()
# both cores draw the race: every other row on core 1 (r3d.c, as the MiSTer's)
set_source_files_properties(${PORT}/r3d.c PROPERTIES COMPILE_DEFINITIONS "OMR_FAST_FLOAT=1;R3D_TWO=1")
