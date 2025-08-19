#ifndef GY33_H
#define GY33_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

//Inicializa o sensor de cor GY-33 (TCS34725).
void gy33_init(i2c_inst_t *i2c);

//Lê os valores de cor brutos do sensor.
void gy33_read_color(i2c_inst_t *i2c, uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c);

//Analisa os valores RGB e retorna o nome da cor mais provável.
const char* identificar_cor(uint16_t r, uint16_t g, uint16_t b, uint16_t c);

#endif // GY33_H
