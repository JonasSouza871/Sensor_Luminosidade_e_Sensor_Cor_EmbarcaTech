#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/bootrom.h"

#define botaoB 6 // Utilizado para o BOOTSEL

// Definições do sensor GY-33
#define GY33_I2C_ADDR 0x29
#define I2C_PORT i2c0
#define SDA_PIN 0
#define SCL_PIN 1

// Definições dos Pinos
const uint BTN_A_PIN = 5;
const uint RED_PIN = 13;
const uint GREEN_PIN = 11;
const uint BLUE_PIN = 12;

// Variáveis de Controle
volatile int led_state = 0;
volatile uint32_t last_press_time = 0;

// Função de Callback da Interrupção
void gpio_irq_handler(uint gpio, uint32_t events) {
    if (gpio == botaoB) {
        reset_usb_boot(0, 0);
    }
    else if (gpio == BTN_A_PIN) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if (current_time - last_press_time > 250) {
            last_press_time = current_time;
            led_state = (led_state + 1) % 4;
        }
    }
}

// Registros do sensor
#define ENABLE_REG 0x80
#define ATIME_REG 0x81
#define CONTROL_REG 0x8F
#define ID_REG 0x92
#define STATUS_REG 0x93
#define CDATA_REG 0x94
#define RDATA_REG 0x96
#define GDATA_REG 0x98
#define BDATA_REG 0x9A

// Funções do sensor GY-33
void gy33_write_register(uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    i2c_write_blocking(I2C_PORT, GY33_I2C_ADDR, buffer, 2, false);
}

uint16_t gy33_read_register(uint8_t reg) {
    uint8_t buffer[2];
    i2c_write_blocking(I2C_PORT, GY33_I2C_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, GY33_I2C_ADDR, buffer, 2, false);
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

// Função para identificar a cor com limiares ajustados
const char* identificar_cor(uint16_t r, uint16_t g, uint16_t b, uint16_t c) {
    // Se não houver luz suficiente, retorna sem cor
    if (c < 40) return "---";
    
    // Calcula as proporções relativas
    float total = r + g + b;
    if (total == 0) return "---";
    
    float rn = r / total;
    float gn = g / total;
    float bn = b / total;
    
    // --- Lógica de detecção de cores ajustada ---
    // A ordem das verificações é importante para evitar sobreposições.

    // Vermelho: componente vermelho é o mais dominante e significativamente maior que o verde.
    if (rn > 0.42 && r > (g * 1.3) && rn > bn) {
        return "Vermelho";
    }
    // Amarelo: vermelho e verde altos e próximos, azul baixo
    if (rn > 0.38 && gn > 0.30 && bn < 0.28 && r < (g * 1.4)) {
        return "Amarelo";
    }
    // Verde: componente verde dominante
    if (gn > rn && gn > bn && g > 20) {
        return "Verde";
    }
    // Azul: componente azul dominante
    if (bn > rn && bn > gn && b > 20) {
        return "Azul";
    }
    // Laranja: vermelho mais alto que verde, azul baixo
    if (rn > 0.45 && gn > 0.25 && gn < rn && bn < 0.2 && r > 20) {
        return "Laranja";
    }
    // Branco: todos os componentes altos e equilibrados
    if (r > 80 && g > 80 && b > 80 && c > 200) {
        // Verifica se as proporções estão próximas
        if ( (rn > gn - 0.1 && rn < gn + 0.1) && (gn > bn - 0.1 && gn < bn + 0.1) ) {
            return "Branco";
        }
    }
     // Cinza/Prata: componentes equilibrados com brilho médio
    if (c > 100 && c < 500) {
        if ( (rn > gn - 0.1 && rn < gn + 0.1) && (gn > bn - 0.1 && gn < bn + 0.1) ) {
            return "Cinza/Prata";
        }
    }
    // Marrom: vermelho e verde moderados, azul baixo
    if (rn > 0.4 && gn > 0.3 && bn < 0.3 && r > 30 && g > 20 && c < 200) {
        return "Marrom";
    }
    // Violeta: vermelho e azul altos, verde baixo
    if (rn > 0.3 && bn > 0.3 && gn < 0.3 && r > 15 && b > 15) {
        return "Violeta";
    }
    // Ouro: vermelho e verde altos, azul baixo, e alto brilho
    if (rn > 0.4 && gn > 0.3 && bn < 0.2 && c > 500) {
        return "Ouro";
    }
    
    return "Desconhecido";
}

int main() {
    // Configuração do BOOTSEL
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    
    stdio_init_all();
    sleep_ms(2000);
    
    // Configuração dos LEDs
    gpio_init(RED_PIN);
    gpio_init(GREEN_PIN);
    gpio_init(BLUE_PIN);
    gpio_set_dir(RED_PIN, GPIO_OUT);
    gpio_set_dir(GREEN_PIN, GPIO_OUT);
    gpio_set_dir(BLUE_PIN, GPIO_OUT);
    
    // Configuração do Botão A
    gpio_init(BTN_A_PIN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_pull_up(BTN_A_PIN);
    gpio_set_irq_enabled_with_callback(BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    
    // Inicialização do sensor
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    printf("Iniciando GY-33...\n");
    gy33_init();
    
    while (1) {
        uint16_t r, g, b, c;
        gy33_read_color(&r, &g, &b, &c);
        
        // Identifica a cor
        const char* nome_cor = identificar_cor(r, g, b, c);
        
        // Imprime informações no console serial
        printf("Cor detectada: %s\n", nome_cor);
        printf("Valores RGB - R: %d, G: %d, B: %d, Clear: %d\n", r, g, b, c);

        // Evita divisão por zero se todos os valores forem 0
        if ((r + g + b) > 0) {
            printf("Proporcoes - R: %.2f, G: %.2f, B: %.2f\n", 
               r/(float)(r+g+b), g/(float)(r+g+b), b/(float)(r+g+b));
        }
        
        printf("--------------------------------\n");
        
        // Controla o LED RGB conforme o estado
        if (led_state == 0) { // Vermelho
            gpio_put(RED_PIN, 1);
            gpio_put(GREEN_PIN, 0);
            gpio_put(BLUE_PIN, 0);
        }
        else if (led_state == 1) { // Amarelo
            gpio_put(RED_PIN, 1);
            gpio_put(GREEN_PIN, 1);
            gpio_put(BLUE_PIN, 0);
        }
        else if (led_state == 2) { // Verde
            gpio_put(RED_PIN, 0);
            gpio_put(GREEN_PIN, 1);
            gpio_put(BLUE_PIN, 0);
        }
        else { // Azul
            gpio_put(RED_PIN, 0);
            gpio_put(GREEN_PIN, 0);
            gpio_put(BLUE_PIN, 1);
        }
        
        sleep_ms(1000);
    }
}
