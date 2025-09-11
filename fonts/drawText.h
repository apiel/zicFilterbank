#pragma once

#include <cstdint>
#include <string>

#include "Sinclair_S.h"
#include "Sinclair_M.h"

#include "../libDaisy/src/dev/oled_ssd130x.h"

void drawChar(daisy::SSD130xI2c64x32Driver& _display, int x, int y, unsigned char character, uint8_t *font, bool on = true, float scale = 1.0)
{
    uint16_t height = font[0];
    uint16_t width = font[1];

    uint8_t mod = width / 8;
    float x0 = x;
    uint16_t len = (mod * height);
    uint16_t temp = ((character - 32) * len) + 2;
    for (uint16_t i = 0; i < len; i++)
    {
        uint8_t ch = font[temp];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (ch & 0x80)
            {
                _display.DrawPixel(x, y, on);
            }
            ch <<= 1;
            x += scale;
            if ((x - x0) == height * scale)
            {
                x = x0;
                y += scale;
                break;
            }
        }
        temp++;
    }
}

int text(daisy::SSD130xI2c64x32Driver& _display, int x, int y, std::string text, uint8_t *font, bool on = true, float scale = 1.0)
{
    uint16_t height = font[0];
    uint16_t width = font[1];
    uint16_t len = text.length();

    float xInc = width * scale;
    for (uint16_t i = 0; i < len; i++)
    {
        if ((x + xInc) > 64)
        {
            break;
        }
        drawChar(_display, x, y, text[i], font, on, scale);
        x += xInc;
    }
    return x;
}