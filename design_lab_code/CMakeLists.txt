cmake_minimum_required(VERSION 3.20)

# Parametry mikrokontrolera i programatora
set(AVR_MCU atmega328pb)
set(F_CPU 16000000UL)
set(AVR_PROGRAMMER xplainedmini)

project(atmega328pb_app C CXX)

set(CMAKE_C_STANDARD 99)

# Flagi kompilacji
add_compile_options(
    -mmcu=${AVR_MCU}
    -DF_CPU=${F_CPU}
    -Os
    -Wall
    -Wextra
    -ffunction-sections
    -fdata-sections
)

# Flagi linkera (usuwanie nieużywanego kodu)
add_link_options(
    -mmcu=${AVR_MCU}
    -Wl,--gc-sections
)

# Źródła projektu
file(GLOB_RECURSE SOURCES "src/*.c")
add_executable(${PROJECT_NAME}.elf ${SOURCES})

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/*.c"
)

target_sources(${PROJECT_NAME}.elf PRIVATE ${LIB_SOURCES})

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/lib"
)

# Generowanie pliku .hex oraz raportu rozmiaru
set(HEX_FILE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.hex)

add_custom_command(
    TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${AVR_OBJCOPY} -O ihex -R .eeprom $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
    COMMAND ${AVR_SIZE} --format=berkeley $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "Generowanie pliku .hex i sprawdzanie rozmiaru..."
)

# Target 'flash' – automatycznie przebudowuje projekt i wgrywa program
add_custom_target(flash
    COMMAND avrdude -c ${AVR_PROGRAMMER} -p m328pb -U flash:w:${HEX_FILE}:i
    DEPENDS ${PROJECT_NAME}.elf
    COMMENT "Wgrywanie programu na ATmega328PB Xplained Mini..."
)