// Bibliotecas padrão e do Pico SDK
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

// Nossas bibliotecas de hardware
#include "ssd1306.h"
#include "matriz_led.h"
#include "gy33.h"

// Definições do Hardware
#define SSD1306_I2C_ADDR 0x3C        // Endereço I2C do display OLED
#define SSD1306_WIDTH 128             // Largura do display
#define SSD1306_HEIGHT 64             // Altura do display
#define BOTAO_A_PIN 5                 // Pino do botão A
#define BOTAO_B_PIN 6                 // Pino do botão B
#define BUZZER_PIN 10                 // Pino do buzzer
#define I2C0_PORT i2c0                // Barramento I2C 0 (Sensor de Cor)
#define I2C0_SDA_PIN 0                // Pino SDA do I2C0
#define I2C0_SCL_PIN 1                // Pino SCL do I2C0
#define I2C1_PORT i2c1                // Barramento I2C 1 (Display OLED)
#define I2C1_SDA_PIN 14               // Pino SDA do I2C1
#define I2C1_SCL_PIN 15               // Pino SCL do I2C1

// Variáveis Globais
volatile int estado_display = 0;      // 0 = RGB, 1 = Normalizado
volatile uint32_t ultimo_tempo_clique = 0; // Debounce dos botões

// Função de interrupção dos botões
void tratar_interrupcao_gpio(uint gpio, uint32_t events) {
    uint32_t tempo_atual = to_ms_since_boot(get_absolute_time());
    if (tempo_atual - ultimo_tempo_clique < 250) return; // Ignora cliques rápidos
    ultimo_tempo_clique = tempo_atual;
    
    if (gpio == BOTAO_B_PIN) {
        estado_display = (estado_display + 1) % 2; // Próxima tela
    } else if (gpio == BOTAO_A_PIN) {
        estado_display = (estado_display - 1 + 2) % 2; // Tela anterior
    }
}

// Inicializa o pino do buzzer para PWM
void inicializar_buzzer() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_init(slice_num, &config, true);
}

// Toca um som com frequência (Hz) e duração (ms)
void tocar_nota(uint frequencia, uint duracao_ms) {
    if (frequencia == 0) { sleep_ms(duracao_ms); return; }
    uint num_slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint32_t clock_sistema = 125000000;
    uint32_t divisor16 = clock_sistema / frequencia / 4096 + (clock_sistema % (frequencia * 4096) != 0);
    if (divisor16 / 16 == 0) divisor16 = 16;
    uint32_t limite_wrap = clock_sistema * 16 / divisor16 / frequencia - 1;
    pwm_set_clkdiv_int_frac(num_slice, divisor16 / 16, divisor16 & 0xF);
    pwm_set_wrap(num_slice, limite_wrap);
    pwm_set_gpio_level(BUZZER_PIN, limite_wrap / 2); // 50% ciclo de trabalho
    sleep_ms(duracao_ms);
    pwm_set_gpio_level(BUZZER_PIN, 0); // Desliga o som
}

// Toca um alerta sonoro para cada cor
void tocar_alerta(const char* nome_da_cor) {
    if (strcmp(nome_da_cor, "Vermelho") == 0) { tocar_nota(1500, 100); sleep_ms(50); tocar_nota(1500, 100); }
    else if (strcmp(nome_da_cor, "Laranja") == 0) { tocar_nota(1200, 200); }
    else if (strcmp(nome_da_cor, "Amarelo") == 0) { tocar_nota(1000, 75); sleep_ms(50); tocar_nota(1000, 75); }
    else if (strcmp(nome_da_cor, "Verde") == 0) { tocar_nota(1200, 100); tocar_nota(1600, 150); }
    else if (strcmp(nome_da_cor, "Azul") == 0) { tocar_nota(800, 100); tocar_nota(1200, 100); }
    else if (strcmp(nome_da_cor, "Branco") == 0) { tocar_nota(1000, 80); tocar_nota(1200, 80); tocar_nota(1500, 80); }
    else if (strcmp(nome_da_cor, "Desconhecido") == 0) { tocar_nota(400, 500); }
}

// Desenha a tela com valores RGB brutos
void desenhar_tela_rgb(ssd1306_t *display, uint16_t r, uint16_t g, uint16_t b, const char* nome_da_cor) {
    char str_cor[32], str_r[16], str_g[16], str_b[16];
    sprintf(str_cor, "Cor: %s", nome_da_cor);
    sprintf(str_r, "R: %d", r); sprintf(str_g, "G: %d", g); sprintf(str_b, "B: %d", b);
    ssd1306_fill(display, false);
    ssd1306_draw_string(display, "--- Valores RGB ---", 5, 0, false);
    ssd1306_draw_string(display, str_cor, 2, 12, false);
    ssd1306_draw_string(display, str_r, 5, 35, false);
    ssd1306_draw_string(display, str_g, 5, 45, false);
    ssd1306_draw_string(display, str_b, 5, 55, false);
}

// Desenha a tela com valores normalizados em percentagem
void desenhar_tela_normalizada(ssd1306_t *display, uint16_t r, uint16_t g, uint16_t b, const char* nome_da_cor) {
    char str_cor[32], str_rn[16], str_gn[16], str_bn[16];
    sprintf(str_cor, "Cor: %s", nome_da_cor);
    float soma_total = r + g + b;
    if (soma_total > 0) {
        sprintf(str_rn, "R: %.1f%%", (r/soma_total) * 100.0f);
        sprintf(str_gn, "G: %.1f%%", (g/soma_total) * 100.0f);
        sprintf(str_bn, "B: %.1f%%", (b/soma_total) * 100.0f);
    } else {
        sprintf(str_rn, "R: 0.0%%"); sprintf(str_gn, "G: 0.0%%"); sprintf(str_bn, "B: 0.0%%");
    }
    ssd1306_fill(display, false);
    ssd1306_draw_string(display, "- Normalizados (%) -", 5, 0, false);
    ssd1306_draw_string(display, str_cor, 2, 12, false);
    ssd1306_draw_string(display, str_rn, 5, 35, false);
    ssd1306_draw_string(display, str_gn, 5, 45, false);
    ssd1306_draw_string(display, str_bn, 5, 55, false);
}

// Converte nome da cor para formato GRB da matriz com brilho reduzido
uint32_t obter_grb_pelo_nome(const char* nome_da_cor) {
    const int DIVISOR_DE_BRILHO = 4;
    int numero_de_cores = 12;
    for (int i = 0; i < numero_de_cores; i++) {
        if (strcmp(nome_da_cor, PALETA_CORES[i].nome) == 0) {
            uint8_t r = PALETA_CORES[i].r / DIVISOR_DE_BRILHO;
            uint8_t g = PALETA_CORES[i].g / DIVISOR_DE_BRILHO;
            uint8_t b = PALETA_CORES[i].b / DIVISOR_DE_BRILHO;
            return GRB(r, g, b);
        }
    }
    return COR_OFF;
}

// Função Principal
int main() {
    // Inicialização da comunicação serial
    stdio_init_all();
    sleep_ms(2000);
    
    // Inicialização do Hardware
    // Barramento I2C 0 para o sensor
    i2c_init(I2C0_PORT, 100 * 1000);
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA_PIN);
    gpio_pull_up(I2C0_SCL_PIN);
    
    // Barramento I2C 1 para o display
    i2c_init(I2C1_PORT, 400 * 1000);
    gpio_set_function(I2C1_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C1_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C1_SDA_PIN);
    gpio_pull_up(I2C1_SCL_PIN);
    
    // Botões
    gpio_init(BOTAO_A_PIN); gpio_set_dir(BOTAO_A_PIN, GPIO_IN); gpio_pull_up(BOTAO_A_PIN);
    gpio_init(BOTAO_B_PIN); gpio_set_dir(BOTAO_B_PIN, GPIO_IN); gpio_pull_up(BOTAO_B_PIN);
    gpio_set_irq_enabled_with_callback(BOTAO_A_PIN, GPIO_IRQ_EDGE_FALL, true, &tratar_interrupcao_gpio);
    gpio_set_irq_enabled_with_callback(BOTAO_B_PIN, GPIO_IRQ_EDGE_FALL, true, &tratar_interrupcao_gpio);
    
    // Inicialização dos Módulos
    gy33_init(I2C0_PORT);
    ssd1306_t display;
    ssd1306_init(&display, SSD1306_WIDTH, SSD1306_HEIGHT, false, SSD1306_I2C_ADDR, I2C1_PORT);
    ssd1306_config(&display);
    inicializar_matriz_led();
    inicializar_buzzer();
    
    // Tela de boas-vindas
    ssd1306_fill(&display, false);
    ssd1306_draw_string(&display, "Iniciando...", 25, 25, false);
    ssd1306_send_data(&display);
    sleep_ms(1500);
    
    // Loop Infinito
    while (1) {
        // Lê os dados do sensor
        uint16_t r, g, b, c;
        gy33_read_color(I2C0_PORT, &r, &g, &b, &c);
        const char* nome_da_cor = identificar_cor(r, g, b, c);
        
        // Atualiza a matriz de LEDs
        uint32_t cor_da_matriz = obter_grb_pelo_nome(nome_da_cor);
        for (int i = 0; i < NUM_PIXELS; ++i) {
            pio_sm_put_blocking(pio0, 0, cor_da_matriz << 8u);
        }
        
        // Toca o alerta sonoro
        tocar_alerta(nome_da_cor);
        
        // Desenha a tela correta no display
        switch (estado_display) {
            case 0: desenhar_tela_rgb(&display, r, g, b, nome_da_cor); break;
            case 1: desenhar_tela_normalizada(&display, r, g, b, nome_da_cor); break;
        }
        ssd1306_send_data(&display);
        
        // Pausa para evitar som contínuo
        sleep_ms(300); 
    }
}