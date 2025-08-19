#include <stdio.h>
#include <string.h> // Necessário para sprintf
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "ssd1306.h" // Incluindo a biblioteca do seu display
#include "matriz_led.h" // Incluindo a biblioteca da sua matriz de LEDs

// --- Definições do Sensor GY-33 ---
#define GY33_I2C_ADDR 0x29

// --- Definições do Display SSD1306 ---
#define SSD1306_I2C_ADDR 0x3C // Endereço I2C comum para displays OLED. Verifique o seu!
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64

// --- Definições dos Botões ---
#define BTN_A_PIN 5 // Botão para voltar a tela
#define BTN_B_PIN 6 // Botão para avançar a tela

// --- Configuração dos Barramentos I2C ---
// I2C 0 para o Sensor de Cor
#define I2C0_PORT i2c0
#define I2C0_SDA_PIN 0
#define I2C0_SCL_PIN 1

// I2C 1 para o Display OLED
#define I2C1_PORT i2c1
#define I2C1_SDA_PIN 14 // Pino GP14 para SDA do display
#define I2C1_SCL_PIN 15 // Pino GP15 para SCL do display

// --- Registros do Sensor GY-33 ---
#define ENABLE_REG 0x80
#define ATIME_REG 0x81
#define CONTROL_REG 0x8F
#define CDATA_REG 0x94
#define RDATA_REG 0x96
#define GDATA_REG 0x98
#define BDATA_REG 0x9A

// --- Variáveis de Controle ---
volatile int display_state = 0; // 0 para tela RGB, 1 para tela Normalizada
volatile uint32_t last_press_time = 0; // Para debouncing dos botões

// --- Callback de Interrupção para os Botões ---
void gpio_irq_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_press_time < 250) { // Debounce de 250ms
        return;
    }
    last_press_time = current_time;

    if (gpio == BTN_B_PIN) { // Botão de avançar
        display_state = (display_state + 1) % 2; // Alterna entre 0 e 1
    } else if (gpio == BTN_A_PIN) { // Botão de voltar
        display_state = (display_state - 1 + 2) % 2; // Alterna entre 1 e 0
    }
}

// --- Funções do Sensor GY-33 (sem alterações) ---
void gy33_write_register(uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    i2c_write_blocking(I2C0_PORT, GY33_I2C_ADDR, buffer, 2, false);
}
uint16_t gy33_read_register(uint8_t reg) {
    uint8_t buffer[2];
    i2c_write_blocking(I2C0_PORT, GY33_I2C_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C0_PORT, GY33_I2C_ADDR, buffer, 2, false);
    return (buffer[1] << 8) | buffer[0];
}
void gy33_init() {
    gy33_write_register(ENABLE_REG, 0x03);
    gy33_write_register(ATIME_REG, 0xF5);
    gy33_write_register(CONTROL_REG, 0x00);
}
void gy33_read_color(uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c) {
    *c = gy33_read_register(CDATA_REG);
    *r = gy33_read_register(RDATA_REG);
    *g = gy33_read_register(GDATA_REG);
    *b = gy33_read_register(BDATA_REG);
}

// --- Função para identificar a cor (LÓGICA FINAL RECALIBRADA) ---
const char* identificar_cor(uint16_t r, uint16_t g, uint16_t b, uint16_t c) {
    if (c < 30) return "---";
    float total = r + g + b;
    if (total == 0) return "---";
    float rn = r / total;
    float gn = g / total;
    float bn = b / total;
    float rg_ratio = (g > 0) ? (float)r / (float)g : 99.0; 

    if (rg_ratio > 1.15) {
        if (bn < 0.23) return "Laranja";
        else return "Vermelho";
    }
    if (rg_ratio > 0.85 && rg_ratio <= 1.15) return "Amarelo";
    if (gn > rn && gn > bn) return "Verde";
    if (bn > rn && bn > gn) return "Azul";
    if (r > 80 && g > 80 && b > 80 && c > 200) {
        if ((rn > gn - 0.1 && rn < gn + 0.1) && (gn > bn - 0.1 && gn < bn + 0.1)) return "Branco";
    }
    if (c > 100 && c < 500) {
        if ((rn > gn - 0.1 && rn < gn + 0.1) && (gn > bn - 0.1 && gn < bn + 0.1)) return "Cinza";
    }
    return "Desconhecido";
}

// --- Funções de Desenho para as Telas ---
void draw_screen_rgb(ssd1306_t *disp, uint16_t r, uint16_t g, uint16_t b, const char* nome_cor) {
    char str_cor[32], str_r[16], str_g[16], str_b[16];
    sprintf(str_cor, "Cor: %s", nome_cor);
    sprintf(str_r, "R: %d", r);
    sprintf(str_g, "G: %d", g);
    sprintf(str_b, "B: %d", b);

    ssd1306_fill(disp, false);
    ssd1306_draw_string(disp, "--- Valores RGB ---", 5, 0, false);
    ssd1306_draw_string(disp, str_cor, 2, 12, false);
    ssd1306_draw_string(disp, str_r, 5, 35, false);
    ssd1306_draw_string(disp, str_g, 5, 45, false);
    ssd1306_draw_string(disp, str_b, 5, 55, false);
}

void draw_screen_normalized(ssd1306_t *disp, uint16_t r, uint16_t g, uint16_t b, const char* nome_cor) {
    char str_cor[32], str_rn[16], str_gn[16], str_bn[16];
    sprintf(str_cor, "Cor: %s", nome_cor);
    float total = r + g + b;
    if (total > 0) {
        sprintf(str_rn, "R: %.1f%%", (r/total) * 100.0f);
        sprintf(str_gn, "G: %.1f%%", (g/total) * 100.0f);
        sprintf(str_bn, "B: %.1f%%", (b/total) * 100.0f);
    } else {
        sprintf(str_rn, "R: 0.0%%");
        sprintf(str_gn, "G: 0.0%%");
        sprintf(str_bn, "B: 0.0%%");
    }

    ssd1306_fill(disp, false);
    ssd1306_draw_string(disp, "- Normalizados (%) -", 5, 0, false);
    ssd1306_draw_string(disp, str_cor, 2, 12, false);
    ssd1306_draw_string(disp, str_rn, 5, 35, false);
    ssd1306_draw_string(disp, str_gn, 5, 45, false);
    ssd1306_draw_string(disp, str_bn, 5, 55, false);
}

// --- Função Auxiliar para a Matriz de LEDs ---
// Procura o nome da cor na paleta e retorna o valor GRB correspondente.
uint32_t get_grb_from_name(const char* nome_cor) {
    // A sua biblioteca não define o tamanho da paleta, então calculamos aqui.
    int num_cores = 12; // Número de cores na sua PALETA_CORES
    for (int i = 0; i < num_cores; i++) {
        if (strcmp(nome_cor, PALETA_CORES[i].nome) == 0) {
            return GRB(PALETA_CORES[i].r, PALETA_CORES[i].g, PALETA_CORES[i].b);
        }
    }
    return COR_OFF; // Retorna preto se a cor não for encontrada
}

// --- Função Principal ---
int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    // Inicialização do I2C0 para o Sensor
    i2c_init(I2C0_PORT, 100 * 1000);
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA_PIN);
    gpio_pull_up(I2C0_SCL_PIN);
    
    // Inicialização do I2C1 para o Display
    i2c_init(I2C1_PORT, 400 * 1000);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SDA_PIN);
    gpio_pull_up(I2C1_SCL_PIN);

    // Configuração dos Botões
    gpio_init(BTN_A_PIN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_pull_up(BTN_A_PIN);
    gpio_init(BTN_B_PIN);
    gpio_set_dir(BTN_B_PIN, GPIO_IN);
    gpio_pull_up(BTN_B_PIN);

    // Configuração das Interrupções dos Botões
    gpio_set_irq_enabled_with_callback(BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BTN_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    
    // Inicializa o sensor de cor
    printf("Iniciando sensor de cor GY-33 no I2C0...\n");
    gy33_init();

    // Inicializa o display SSD1306
    printf("Iniciando display SSD1306 no I2C1...\n");
    ssd1306_t disp;
    ssd1306_init(&disp, SSD1306_WIDTH, SSD1306_HEIGHT, false, SSD1306_I2C_ADDR, I2C1_PORT);
    ssd1306_config(&disp);
    
    // Inicializa a Matriz de LEDs
    printf("Iniciando Matriz de LEDs WS2812...\n");
    inicializar_matriz_led();

    ssd1306_fill(&disp, false);
    ssd1306_draw_string(&disp, "Iniciando...", 25, 25, false);
    ssd1306_send_data(&disp);
    sleep_ms(1500);
    
    while (1) {
        uint16_t r, g, b, c;
        gy33_read_color(&r, &g, &b, &c);
        const char* nome_cor = identificar_cor(r, g, b, c);
        
        // --- ATUALIZA A MATRIZ DE LEDS ---
        uint32_t cor_matriz = get_grb_from_name(nome_cor);
        for (int i = 0; i < NUM_PIXELS; ++i) {
            pio_sm_put_blocking(pio0, 0, cor_matriz << 8u);
        }
        
        // Desenha a tela apropriada no display com base no estado
        switch (display_state) {
            case 0:
                draw_screen_rgb(&disp, r, g, b, nome_cor);
                break;
            case 1:
                draw_screen_normalized(&disp, r, g, b, nome_cor);
                break;
        }

        // Atualiza a tela
        ssd1306_send_data(&disp);
        sleep_ms(200);
    }
}
