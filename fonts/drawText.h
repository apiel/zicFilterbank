#pragma once

#include <cstdint>
#include <string>

#include "PoppinsLight_8.h"
#include "PoppinsLight_12.h"

#include "../libDaisy/src/dev/oled_ssd130x.h"

int drawChar(daisy::SSD130xI2c64x32Driver &_display, int x, int y, uint8_t *charPtr, int width, int marginTop, int rows, bool on = true, float scale = 1.00f)
{
    for (int row = 0; row < rows; row++)
    {
        for (int col = 0; col < width; col++)
        {
            uint8_t a = charPtr[col + row * width];
            if (a > 80)
            {
                _display.DrawPixel(x + col * scale, y + row * scale + marginTop, on);
            }
        }
    }
    return width * scale;
}

int text(daisy::SSD130xI2c64x32Driver &_display, int x, int y, std::string text, const uint8_t **font, uint8_t size = 0, bool on = true, uint8_t fontSpacing = 1)
{
    uint16_t len = text.length();
    uint8_t height = *font[0];
    float scale = 1;
    if (size > 0)
    {
        scale = size / (float)height;
        scale = scale == 0 ? 1 : scale;
    }
    for (uint16_t i = 0; i < len; i++)
    {
        char c = text[i];
        const uint8_t *charPtr = font[1 + (c - ' ')]; // Get the glyph data for the character
        uint8_t width = charPtr[0];
        uint8_t marginTop = charPtr[1] * scale;
        uint8_t rows = charPtr[2];
        x += drawChar(_display, x, y, (uint8_t *)charPtr + 3, width, marginTop, rows, on, scale) + fontSpacing;
    }
    return x;
}