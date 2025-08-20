# 🚀 Pico Color & Light Sensor – Identificador Inteligente de Cores

<div align="center">

![Linguagem](https://img.shields.io/badge/Linguagem-C%2FC%2B%2B-blue?style=for-the-badge)
![Hardware](https://img.shields.io/badge/Hardware-Raspberry%20Pi%20Pico-E01244?style=for-the-badge)
![Sensor](https://img.shields.io/badge/Sensor-GY--33%20%26%20BH1750-555555?style=for-the-badge)
![Licença](https://img.shields.io/badge/Licen%C3%A7a-MIT-yellow?style=for-the-badge)

</div>

Um dispositivo inteligente baseado no Raspberry Pi Pico que funciona como um detector de frutas, fornecendo feedback visual e sonoro contextual.

---

### 📝 Descrição Breve

Este projeto transforma o Raspberry Pi Pico em um **detector de frutas inteligente**. A aplicação principal é identificar tipos de frutas com base em sua cor característica. Para isso, o sistema foi **calibrado utilizando amostras de Maçã (vermelho), Laranja (laranja) e Banana (amarelo)**.

Utilizando um sensor de cor GY-33 e um sensor de luminosidade BH1750, o dispositivo analisa o objeto à sua frente. As informações são processadas e exibidas em um display OLED, enquanto uma matriz de LEDs e um buzzer fornecem feedback interativo sobre a cor e a luminosidade do ambiente.

---

### ✨ Funcionalidades Principais

-   **✅ Detector de Frutas (GY-33):** Lê os valores RGB para identificar cores. A lógica foi calibrada para reconhecer e associar cores a frutas específicas, como **Maçã, Laranja e Banana**.
-   **✅ Medição de Luminosidade (BH1750):** Mede a intensidade da luz ambiente em Lux, permitindo que o sistema se adapte e alerte sobre as condições de iluminação.
-   **✅ Múltiplas Telas de Exibição:** O usuário pode alternar entre três telas informativas no display OLED usando botões:
    1.  **Valores RGB:** Mostra os dados brutos do sensor de cor.
    2.  **Valores Normalizados:** Exibe a contribuição percentual de cada canal de cor (R, G, B).
    3.  **Luminosidade:** Apresenta o valor em Lux e um status (OK, MUITO BAIXO, MUITO ALTO).
-   **✅ Feedback Visual Adaptativo:** Uma matriz de LEDs 5x5 exibe a cor identificada, ajustando automaticamente seu brilho com base na leitura do sensor de luminosidade para garantir boa visibilidade sem ofuscar.
-   **✅ Alertas Sonoros Contextuais:** Um buzzer fornece feedback sonoro inteligente:
    -   Toca notas distintas para as cores associadas às frutas (Vermelho, Laranja, Amarelo).
    -   Emite um alerta especial e chamativo se a luminosidade estiver fora dos limites seguros (muito escuro ou muito claro).

---

### 🖼 Galeria do Projeto

| Display OLED em Operação | Matriz de LEDs Adaptativa |
| :---: | :---: |
| *[INSERIR FOTO DO DISPLAY MOSTRANDO DADOS]* | *[INSERIR FOTO DA MATRIZ DE LEDS ACESA]* |
| Exibição dos dados de cor e luz em tempo real. | Matriz de LEDs mostrando a cor identificada. |

---

### ⚙ Hardware Necessário

| Componente | Quant. | Observações |
| :--- | :---: | :--- |
| Raspberry Pi Pico | 1 | O microcontrolador do projeto. |
| Sensor de Cor GY-33 (TCS34725) | 1 | Para identificação de cores. |
| Sensor de Luminosidade BH1750 | 1 | Para medir a luz ambiente em Lux. |
| Display OLED 128x64 | 1 | Para a interface visual (I2C, SSD1306). |
| Matriz de LEDs 5x5 (WS2812) | 1 | Para feedback visual colorido. |
| Botões Momentâneos | 2 | Para navegação entre as telas. |
| Buzzer Passivo | 1 | Para os alertas sonoros. |
| Protoboard e Jumpers | - | Para montagem do circuito. |

---

### 🔌 Conexões e Configuração

O projeto utiliza dois barramentos I2C para evitar conflitos de endereço e otimizar a comunicação.

**Barramento I2C0 (Sensores):**
-   `GY-33 & BH1750 SDA` -> `GPIO 0`
-   `GY-33 & BH1750 SCL` -> `GPIO 1`

**Barramento I2C1 (Display):**
-   `Display OLED SDA` -> `GPIO 14`
-   `Display OLED SCL` -> `GPIO 15`

**Controles e Atuadores:**
-   `Botão A (Anterior)` -> `GPIO 5`
-   `Botão B (Próximo)` -> `GPIO 6`
-   `Buzzer` -> `GPIO 10`
-   `Matriz WS2812 DIN` -> Conectado ao pino definido em `matriz_led.h` (PINO_WS2812).

> **⚠ Importante:** Garanta um `GND` comum entre todos os componentes. A matriz de LEDs WS2812 pode consumir uma corrente considerável; alimente-a com uma fonte externa de 5V se necessário.

---

### 🚀 Começando

#### Pré-requisitos de Software

-   **SDK:** Raspberry Pi Pico SDK
-   **Linguagem:** C/C++
-   **Build System:** CMake
-   **IDE Recomendada:** VS Code com a extensão "CMake Tools"

#### Configuração e Compilação

Siga estes passos para configurar, compilar e carregar o firmware no seu Pico.

```bash
# 1. Clone o repositório do projeto
git clone [URL_DO_SEU_REPOSITORIO]
cd [NOME_DO_DIRETORIO]

# 2. Configure o ambiente de build com CMake
# (Certifique-se de que o PICO_SDK_PATH está definido como variável de ambiente)
mkdir build
cd build
cmake ..

# 3. Compile o projeto
make -j$(nproc)

# 4. Carregue o firmware
# Pressione e segure o botão BOOTSEL no Pico enquanto o conecta ao USB.
# Copie o arquivo .uf2 gerado para o drive RPI-RP2.
cp nome_do_projeto.uf2 /media/user/RPI-RP2
```

---

### 📁 Estrutura do Projeto

```
.
├── build/
├── lib/
│   ├── Display_Bibliotecas/
│   │   ├── font.h
│   │   ├── ssd1306.c
│   │   └── ssd1306.h
│   ├── Matriz_Bibliotecas/
│   │   ├── generated/
│   │   ├── matriz_led.c
│   │   ├── matriz_led.h
│   │   └── ws2812.pio
│   ├── bh1750_light_sensor.c
│   ├── bh1750_light_sensor.h
│   ├── gy33.c
│   └── gy33.h
├── .gitignore
├── CMakeLists.txt
├── main.c
└── pico_sdk_import.cmake
```
*(Estrutura baseada na imagem fornecida.)*

---

### 🐛 Solução de Problemas

-   **Sensores não encontrados ou display em branco:**
    -   Verifique as conexões I2C (SDA e SCL) para cada barramento. Certifique-se de que os sensores estão no I2C0 e o display no I2C1.
    -   Confirme os endereços I2C dos dispositivos.
-   **Matriz de LEDs não acende ou mostra cores erradas:**
    -   Verifique o pino de dados (DIN) e a alimentação da matriz (5V e GND).
    -   O formato da cor no código é GRB (Verde, Vermelho, Azul), que é o padrão para WS2812.
-   **Botões não respondem:**
    -   Certifique-se de que os botões estão conectados corretamente aos pinos GPIO e ao GND. O código usa pull-ups internos.
