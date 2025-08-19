# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

# Versão mínima do CMake requerida
cmake_minimum_required(VERSION 3.13)

# Inclui o SDK do Pico para encontrar as bibliotecas e funções necessárias
include(pico_sdk_import.cmake)

# Define o nome do projeto
project(pico_sensores_luz_cor C CXX ASM)

# Configurações de padrões de linguagem
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Inicializa o SDK do Pico
pico_sdk_init()

# Diretórios de inclusão para headers do projeto
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/Display_Bibliotecas
    ${CMAKE_SOURCE_DIR}/lib/Matriz_Bibliotecas
    ${CMAKE_SOURCE_DIR}/lib/GY33_Biblioteca # Adicionado para a nova biblioteca
)

# Cria o executável com os arquivos fonte
add_executable(pico_sensores_luz_cor
    main.c
    lib/Display_Bibliotecas/ssd1306.c
    lib/Matriz_Bibliotecas/matriz_led.c
    lib/gy33.c # Adicionado o ficheiro .c da nova biblioteca
)

# Vincula as bibliotecas necessárias ao executável
target_link_libraries(pico_sensores_luz_cor
    pico_stdlib       # Biblioteca padrão do Pico
    hardware_i2c      # Driver I2C do Pico SDK
    hardware_pwm      # Driver PWM do Pico SDK
    hardware_pio      # Driver PIO do Pico SDK
    hardware_adc      # Driver ADC do Pico SDK
)

# Habilita saída padrão (printf) via USB e UART
pico_enable_stdio_usb(pico_sensores_luz_cor 1)
pico_enable_stdio_uart(pico_sensores_luz_cor 1)

# Gera arquivos adicionais (binário, UF2, etc.) para a placa
pico_add_extra_outputs(pico_sensores_luz_cor)
