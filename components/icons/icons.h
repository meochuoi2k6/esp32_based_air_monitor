#ifndef ICONS_H
#define ICONS_H

#include <stdint.h>

extern const uint8_t wifi_on[32];
extern const uint8_t wifi_off[32];
extern const uint8_t sd_ok[32];
extern const uint8_t sd_fail[32];

void draw_bitmap(int x, int y, const uint8_t *bitmap);

#endif