set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

# Wybór kompilatora i narzędzi AVR
set(CMAKE_C_COMPILER avr-gcc)
set(CMAKE_CXX_COMPILER avr-g++)
set(AVR_OBJCOPY avr-objcopy)
set(AVR_SIZE avr-size)

# Pomijamy testowanie kompilatora (niezbędne w cross-kompilacji)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)