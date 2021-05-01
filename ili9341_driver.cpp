#include "ili9341_driver.h"
#include "pico/stdlib.h"

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

ILI9341Driver::ILI9341Driver(int16_t w, int16_t h) : Adafruit_GFX(w, h) {}

void ILI9341Driver::begin() {
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
    setRotation(0);

    // Adjust LCD's brightness level
    lcdWrite8BCMD(CMD_WRITE_DISPLAY_BRIGHTNESS);
    lcdWrite8BData(0xFF); // Brightness level

    // Exit from sleep mode
    lcdWrite8BCMD(0x11); // Sleep out

    // Turn on the display
    lcdWrite8BCMD(CMD_DISPLAY_ON);

    _width = LCD_WIDTH;
    _height = LCD_HEIGHT;
}

void ILI9341Driver::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return;

    setAddrWindow(x, y, x+1, y+1);
    lcdWrite16BData(color);
}

void ILI9341Driver::setRotation(uint8_t r) {
    rotation = r;

    lcdWrite8BCMD(CMD_MEMORY_ACCESS_CONTROL);
    switch((r % 4)) {
        case 0:
        lcdWrite8BData(PRM_MEMORY_ACCESS_CONTROL_MX | PRM_MEMORY_ACCESS_CONTROL_BGR);
        _width  = LCD_WIDTH;
        _height = LCD_HEIGHT;
        break;
        case 1:
        lcdWrite8BData(PRM_MEMORY_ACCESS_CONTROL_MV | PRM_MEMORY_ACCESS_CONTROL_BGR);
        _width  = LCD_HEIGHT;
        _height = LCD_WIDTH;
        break;
        case 2:
        lcdWrite8BData(PRM_MEMORY_ACCESS_CONTROL_MY | PRM_MEMORY_ACCESS_CONTROL_BGR);
        _width  = LCD_WIDTH;
        _height = LCD_HEIGHT;
        break;
        case 3:
        lcdWrite8BData(PRM_MEMORY_ACCESS_CONTROL_MX | PRM_MEMORY_ACCESS_CONTROL_MY |
            PRM_MEMORY_ACCESS_CONTROL_MV | PRM_MEMORY_ACCESS_CONTROL_BGR);
        _width  = LCD_HEIGHT;
        _height = LCD_WIDTH;
        break;
    }
}

void ILI9341Driver::invertDisplay(bool i) {
    lcdWrite8BCMD(i ? CMD_INVERSION_ON : CMD_INVERSION_OFF);
}

void ILI9341Driver::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
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

void ILI9341Driver::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    if(x >= _width || y >= _height || h < 1) return;
    if(y + h - 1 >= _height) {
        h = _height - y;
    }
    if(h < 2) {
        drawPixel(x, y, color);
        return;
    }

    setAddrWindow(x, y, x, y + h - 1);
    while(h--) {
        lcdWrite16BData(color);
    }
}

void ILI9341Driver::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    if(x >= _width || y >= _height || w < 1) return;
    if(x + w - 1 >= _width) {
        w = _width - x;
    }
    if(w < 2) {
        drawPixel(x, y, color);
        return;
    }

    setAddrWindow(x, y, x + w - 1, y);
    while(w--) {
        lcdWrite16BData(color);
    }
}

void ILI9341Driver::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if(x >= _width || y >= _height || w < 1 || h < 1) return;
    if((x + w - 1) >= _width) {
        w = _width - x;
    }
    if((y + h - 1) >= _height) {
        h = _width - y;
    }
    if(w == 1 || h == 1) {
        drawPixel(x, y, color);
        return;
    }

    setAddrWindow(x, y, x + w - 1, y + h - 1);

    for(y = h; y > 0; y--) {
        for(x = w; x > 0; x--) {
            lcdWrite16BData(color);
        }
    }
}

void ILI9341Driver::fillScreen(uint16_t color) {
    fillRect(0, 0, _width, _height, color);
}

size_t ILI9341Driver::print(const uint8_t *buf, size_t s) {
    size_t n = 0;
    while (s--) {
        if (write(*buf++)) n++;
        else break;
    }
    return n;
}

size_t ILI9341Driver::print(const char *str, size_t s) {
    return print((uint8_t *) str, s);
}