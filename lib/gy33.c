#include "gy33.h"

// --- Definições do Sensor GY-33 ---
#define GY33_I2C_ADDR 0x29

// --- Registos do Sensor GY-33 ---
#define ENABLE_REG 0x80
#define ATIME_REG 0x81
#define CONTROL_REG 0x8F
#define CDATA_REG 0x94
#define RDATA_REG 0x96
#define GDATA_REG 0x98
#define BDATA_REG 0x9A

// --- Funções Internas (privadas à biblioteca) ---
static void gy33_write_register(i2c_inst_t *i2c, uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    i2c_write_blocking(i2c, GY33_I2C_ADDR, buffer, 2, false);
}

static uint16_t gy33_read_register(i2c_inst_t *i2c, uint8_t reg) {
    uint8_t buffer[2];
    i2c_write_blocking(i2c, GY33_I2C_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c, GY33_I2C_ADDR, buffer, 2, false);
    return (buffer[1] << 8) | buffer[0];
}

// --- Funções Públicas (declaradas em gy33.h) ---
void gy33_init(i2c_inst_t *i2c) {
    gy33_write_register(i2c, ENABLE_REG, 0x03);
    gy33_write_register(i2c, ATIME_REG, 0xF5);
    gy33_write_register(i2c, CONTROL_REG, 0x00);
}

void gy33_read_color(i2c_inst_t *i2c, uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c) {
    *c = gy33_read_register(i2c, CDATA_REG);
    *r = gy33_read_register(i2c, RDATA_REG);
    *g = gy33_read_register(i2c, GDATA_REG);
    *b = gy33_read_register(i2c, BDATA_REG);
}

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
    if (rg_ratio > 0.85 && rg_ratio <= 1.15) {
        if (c > 400) return "Ouro";
        else return "Amarelo";
    }
    if (gn > rn && gn > bn) return "Verde";
    if (bn > rn && bn > gn) return "Azul";
    if (bn > 0.4 && rn > 0.3 && gn < 0.3) return "Violeta";
    if (rg_ratio > 1.2 && c < 80 && c > 30) return "Marrom";
    bool is_balanced = (rn > gn - 0.15 && rn < gn + 0.15) && (gn > bn - 0.15 && gn < bn + 0.15);
    if (is_balanced) {
        if (c > 600) return "Branco";
        if (c > 300) return "Prata";
        if (c > 80) return "Cinza";
    }
    return "Desconhecido";
}
