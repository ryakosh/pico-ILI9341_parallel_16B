#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "defines.h"

static uint16_t _curWidth = LCD_WIDTH;
static uint16_t _curHeight = LCD_HEIGHT;

void writeStrobe() {
    gpio_put(WRITE_ENABLE_PIN, WRITE_ENABLE_IDLE);
    gpio_put(WRITE_ENABLE_PIN, WRITE_ENABLE_ACTIVE);
}

void readStrobe() {
    gpio_put(READ_ENABLE_PIN, READ_ENABLE_IDLE);
    gpio_put(READ_ENABLE_PIN, READ_ENABLE_ACTIVE);
}

void lcdWrite8B(uint8_t data) {
    gpio_put_masked(LCD_PINS_8B, (data & 0xFF));
    writeStrobe();
}

void lcdWrite16B(uint16_t data) {
    gpio_put_masked(LCD_PINS_16B, (data & 0xFFFF));
    writeStrobe();
}

void lcdWrite8BCMD(uint8_t cmd) {
    gpio_put(REGISTER_SELECT_PIN, REGISTER_SELECT_CMD);
    lcdWrite8B(cmd);
}

void lcdWrite8BData(uint8_t data) {
    gpio_put(REGISTER_SELECT_PIN, REGISTER_SELECT_DATA);
    lcdWrite8B(data);
}

void lcdWrite16BData(uint16_t data) {
    gpio_put(REGISTER_SELECT_PIN, REGISTER_SELECT_DATA);
    lcdWrite16B(data);
}

void initPins() {
    gpio_init_mask(LCD_PINS_MASK);
    gpio_set_dir_all_bits(LCD_PINS_MASK);

    gpio_pull_down(REGISTER_SELECT_PIN);
    gpio_pull_down(CHIP_SELECT_PIN);
    gpio_pull_up(READ_ENABLE_PIN);
    gpio_pull_up(WRITE_ENABLE_PIN);
    gpio_pull_up(RESET_PIN);
}

void lcdDrawWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // Column start and end
    lcdWrite8BCMD(CMD_COL_ADDRESS_SET);
    lcdWrite8BData(x0 >> 8);
    lcdWrite8BData(x0 & 0xFF);
    lcdWrite8BData(x1 >> 8);
    lcdWrite8BData(x1 & 0xFF);

    // Row start and end
    lcdWrite8BCMD(CMD_ROW_ADDRESS_SET);
    lcdWrite8BData(y0 >> 8);
    lcdWrite8BData(y0 & 0xFF);
    lcdWrite8BData(y1 >> 8);
    lcdWrite8BData(y1 & 0xFF);

    // Draw the window
    lcdWrite8BCMD(CMD_MEMORY_WRITE);
}

void lcdDrawPixel(int16_t x, int16_t y, uint16_t color) {
    if((x < 0) || (x >= _curWidth) || (y < 0) || (y >= _curHeight)) return;

    lcdDrawWindow(x, y, x+1, y+1);
    lcdWrite16BData(color);
}

void lcdSetRotation(uint8_t m) {
    lcdWrite8BCMD(CMD_MEMORY_ACCESS_CONTROL);

    switch((m % 4)) {
        case 0:
        lcdWrite8BData(PRM_MEMORY_ACCESS_CONTROL_MX | PRM_MEMORY_ACCESS_CONTROL_BGR);
        _curWidth  = LCD_WIDTH;
        _curHeight = LCD_HEIGHT;
        break;
        case 1:
        lcdWrite8BData(PRM_MEMORY_ACCESS_CONTROL_MV | PRM_MEMORY_ACCESS_CONTROL_BGR);
        _curWidth  = LCD_HEIGHT;
        _curHeight = LCD_WIDTH;
        break;
        case 2:
        lcdWrite8BData(PRM_MEMORY_ACCESS_CONTROL_MY | PRM_MEMORY_ACCESS_CONTROL_BGR);
        _curWidth  = LCD_WIDTH;
        _curHeight = LCD_HEIGHT;
        break;
        case 3:
        lcdWrite8BData(PRM_MEMORY_ACCESS_CONTROL_MX | PRM_MEMORY_ACCESS_CONTROL_MY |
            PRM_MEMORY_ACCESS_CONTROL_MV | PRM_MEMORY_ACCESS_CONTROL_BGR);
        _curWidth  = LCD_HEIGHT;
        _curHeight = LCD_WIDTH;
        break;
    }
}

void initLCD() {
    int i = 0;

    gpio_put(CHIP_SELECT_PIN, CHIP_SELECT_ACTIVE);
    gpio_put(REGISTER_SELECT_PIN, false);
    gpio_put(READ_ENABLE_PIN, true);
    gpio_put(WRITE_ENABLE_PIN, true);

    // Perform LCD reset
    gpio_put(RESET_PIN, true);
    sleep_ms(5);
    gpio_put(RESET_PIN, false);
    sleep_ms(20);
    gpio_put(RESET_PIN, true);
    sleep_ms(150);

    // Use 16bit RGB colors
    lcdWrite8BCMD(CMD_PIXEL_FORMAT);
    lcdWrite8BData(PRM_PIXEL_FORMAT_16B);

    // Set LCD to use BGR instead of RGB and configure rotation
    lcdSetRotation(0);

    // Adjust LCD's brightness level
    lcdWrite8BCMD(CMD_WRITE_DISPLAY_BRIGHTNESS);
    lcdWrite8BData(0xFF); // Brightness level

    // Exit from sleep mode
    lcdWrite8BCMD(0x11); // Sleep out

    // Turn on the display
    lcdWrite8BCMD(CMD_DISPLAY_ON);
}

void lcdDrawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    if(x >= _curWidth || y >= _curHeight || h < 1) return;
    if(y + h - 1 >= _curHeight) {
        h = _curHeight - y;
    }
    if(h < 2) {
        lcdDrawPixel(x, y, color);
        return;
    }

    lcdDrawWindow(x, y, x, y + h - 1);
    while(h--) {
        lcdWrite16BData(color);
    }
}

void lcdDrawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    if(x >= _curWidth || y >= _curHeight || w < 1) return;
    if(x + w - 1 >= _curWidth) {
        w = _curWidth - x;
    }
    if(w < 2) {
        lcdDrawPixel(x, y, color);
        return;
    }

    lcdDrawWindow(x, y, x + w - 1, y);
    while(w--) {
        lcdWrite16BData(color);
    }
}

void lcdDrawFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if(x >= _curWidth || y >= _curHeight || w < 1 || h < 1) return;
    if((x + w - 1) >= _curWidth) {
        w = _curWidth - x;
    }
    if((y + h - 1) >= _curHeight) {
        h = _curWidth - y;
    }
    if(w == 1 || h == 1) {
        lcdDrawPixel(x, y, color);
        return;
    }

    lcdDrawWindow(x, y, x + w - 1, y + h - 1);

    for(y = h; y > 0; y--) {
        for(x = w; x > 0; x--) {
            lcdWrite16BData(color);
        }
    }
}

void lcdDrawFillScreen(uint16_t color) {
    lcdDrawFillRect(0, 0, _curWidth, _curHeight, color);
}

void lcdDrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    bool steep;
    int16_t dx, dy, err, ystep, xbegin;

	if ((y0 < 0 && y1 < 0) || (y0 > _curHeight && y1 > _curHeight)) return;
	if ((x0 < 0 && x1 < 0) || (x0 > _curWidth && x1 > _curWidth)) return;
	if (x0 < 0) x0 = 0;
	if (x1 < 0) x1 = 0;
	if (y0 < 0) y0 = 0;
	if (y1 < 0) y1 = 0;

	if (y0 == y1) {
		if (x1 > x0) {
			lcdDrawFastHLine(x0, y0, x1 - x0 + 1, color);
		}
		else if (x1 < x0) {
			lcdDrawFastHLine(x1, y0, x0 - x1 + 1, color);
		}
		else {
			lcdDrawPixel(x0, y0, color);
		}
		return;
	}
	else if (x0 == x1) {
		if (y1 > y0) {
			lcdDrawFastVLine(x0, y0, y1 - y0 + 1, color);
		}
		else {
			lcdDrawFastVLine(x0, y1, y0 - y1 + 1, color);
		}
		return;
	}

	steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep) {
		swap(x0, y0);
		swap(x1, y1);
	}
	if (x0 > x1) {
		swap(x0, x1);
		swap(y0, y1);
	}

	dx = x1 - x0;
	dy = abs(y1 - y0);

	err = dx / 2;

	if (y0 < y1) {
		ystep = 1;
	}
	else {
		ystep = -1;
	}

	xbegin = x0;

	if (steep) {
		for (; x0 <= x1; x0++) {
			err -= dy;
			if (err < 0) {
				int16_t len = x0 - xbegin;
				if (len) {
					lcdDrawFastVLine(y0, xbegin, len + 1, color);
				}
				else {
					lcdDrawPixel(y0, x0, color);
				}
				xbegin = x0 + 1;
				y0 += ystep;
				err += dx;
			}
		}
		if (x0 > xbegin + 1) {
			lcdDrawFastVLine(y0, xbegin, x0 - xbegin, color);
		}

	}
	else {
		for (; x0 <= x1; x0++) {
			err -= dy;
			if (err < 0) {
				int16_t len = x0 - xbegin;
				if (len) {
					lcdDrawFastHLine(xbegin, y0, len + 1, color);
				}
				else {
					lcdDrawPixel(x0, y0, color);
				}
				xbegin = x0 + 1;
				y0 += ystep;
				err += dx;
			}
		}
		if (x0 > xbegin + 1) {
			lcdDrawFastHLine(xbegin, y0, x0 - xbegin, color);
		}
	}
}

void lcdInvertDisplay(bool i) {
    lcdWrite8BCMD(i ? CMD_INVERSION_ON : CMD_INVERSION_OFF);
}

uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

int main() {
    stdio_init_all();
    initPins();
    initLCD();

    while(true) {}
}