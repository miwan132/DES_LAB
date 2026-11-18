set(CMAKE_HOST_SYSTEM "Windows-10.0.26200")
set(CMAKE_HOST_SYSTEM_NAME "Windows")
set(CMAKE_HOST_SYSTEM_VERSION "10.0.26200")
set(CMAKE_HOST_SYSTEM_PROCESSOR "AMD64")

set(_CMAKE_INPUT_TOOLCHAIN_FILE [==[C:/Users/bartek/Documents/Semestr_5/Design Lab/design_lab_code/cmake/avr-toolchain.cmake]==])
set(_CMAKE_SYSTEM_TOOLCHAIN_FILE "C:/Users/bartek/Documents/Semestr_5/Design Lab/design_lab_code/cmake/avr-toolchain.cmake")
include("${_CMAKE_SYSTEM_TOOLCHAIN_FILE}")

set(CMAKE_SYSTEM "Generic")
set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_SYSTEM_VERSION "")
set(CMAKE_SYSTEM_PROCESSOR "avr")

set(CMAKE_CROSSCOMPILING "TRUE")

set(CMAKE_SYSTEM_LOADED 1)
