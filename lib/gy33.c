#include "gy33.h"

// --- Definições do Sensor GY-33 ---
#define GY33_I2C_ADDR 0x29          // Endereço I2C padrão do sensor

// --- Registos do Sensor GY-33 ---
#define ENABLE_REG 0x80             // Habilita o sensor e controla modos de operação
#define ATIME_REG 0x81              // Configura o tempo de integração do ADC
#define CONTROL_REG 0x8F            // Controla o ganho do sensor
#define CDATA_REG 0x94              // Registrador de dados de luz clara (Clear)
#define RDATA_REG 0x96              // Registrador de dados do canal vermelho (Red)
#define GDATA_REG 0x98              // Registrador de dados do canal verde (Green)
#define BDATA_REG 0x9A              // Registrador de dados do canal azul (Blue)

// --- Funções Internas (privadas à biblioteca) ---

// Escreve um valor em um registrador específico
static void gy33_write_register(i2c_inst_t *i2c, uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    i2c_write_blocking(i2c, GY33_I2C_ADDR, buffer, 2, false);
}

// Lê um valor de 16 bits de um registrador específico
static uint16_t gy33_read_register(i2c_inst_t *i2c, uint8_t reg) {
    uint8_t buffer[2];
    i2c_write_blocking(i2c, GY33_I2C_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c, GY33_I2C_ADDR, buffer, 2, false);
    return (buffer[1] << 8) | buffer[0];
}

// --- Funções Públicas (declaradas em gy33.h) ---

// Inicializa o sensor com configurações padrão
void gy33_init(i2c_inst_t *i2c) {
    gy33_write_register(i2c, ENABLE_REG, 0x03);    // Habilita sensor e ADC
    gy33_write_register(i2c, ATIME_REG, 0xF5);      // Define tempo de integração (700ms)
    gy33_write_register(i2c, CONTROL_REG, 0x00);    // Configura ganho 1x
}

// Lê os valores de cor do sensor
void gy33_read_color(i2c_inst_t *i2c, uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c) {
    *c = gy33_read_register(i2c, CDATA_REG);        // Luz clara (intensidade total)
    *r = gy33_read_register(i2c, RDATA_REG);        // Componente vermelho
    *g = gy33_read_register(i2c, GDATA_REG);        // Componente verde
    *b = gy33_read_register(i2c, BDATA_REG);        // Componente azul
}

// Identifica a cor com base nos valores RGB e intensidade
const char* identificar_cor(uint16_t r, uint16_t g, uint16_t b, uint16_t c) {
    if (c < 30) return "---";                       // Ambiente escuro
    
    float total = r + g + b;
    if (total == 0) return "---";                    // Sem dados válidos
    
    // Normalização dos componentes
    float rn = r / total;
    float gn = g / total;
    float bn = b / total;
    float rg_ratio = (g > 0) ? (float)r / (float)g : 99.0;
    
    // Lógica de identificação de cores
    if (rg_ratio > 1.15) {
        return (bn < 0.23) ? "Laranja" : "Vermelho";
    }
    if (rg_ratio > 0.85 && rg_ratio <= 1.15) {
        return (c > 400) ? "Ouro" : "Amarelo";
    }
    if (gn > rn && gn > bn) return "Verde";
    if (bn > rn && bn > gn) return "Azul";
    if (bn > 0.4 && rn > 0.3 && gn < 0.3) return "Violeta";
    if (rg_ratio > 1.2 && c < 80 && c > 30) return "Marrom";
    
    // Detecção de cores neutras (tons de cinza)
    bool is_balanced = (rn > gn - 0.15 && rn < gn + 0.15) && 
                       (gn > bn - 0.15 && gn < bn + 0.15);
    if (is_balanced) {
        if (c > 600) return "Branco";
        if (c > 300) return "Prata";
        if (c > 80) return "Cinza";
    }
    
    return "Desconhecido";
}