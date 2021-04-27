#include <stdio.h>
#include "pico/stdlib.h"
#include "defines.h"

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

void lcdDrawPixel(uint32_t x, uint32_t y, uint16_t color) {
    if(x >= LCD_WIDTH || y >= LCD_HEIGHT) return;

    lcdDrawWindow(x, y, x+1, y+1);
    lcdWrite16BData(color);
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

    // Set LCD to use RGB instead of BGR
    lcdWrite8BCMD(CMD_MEMORY_ACCESS_CONTROL);
    lcdWrite8BData(0x8);

    // Adjust LCD's brightness level
    lcdWrite8BCMD(CMD_WRITE_DISPLAY_BRIGHTNESS);
    lcdWrite8BData(0xFF); // Brightness level

    // Exit from sleep mode
    lcdWrite8BCMD(0x11); // Sleep out

    // Turn on the display
    lcdWrite8BCMD(CMD_DISPLAY_ON);
}

void lcdDrawFastVLine(uint16_t x, uint16_t y, uint16_t h, uint16_t color) {
    if(x >= LCD_WIDTH || y >= LCD_HEIGHT || h < 1) return;
    if(y + h - 1 >= LCD_HEIGHT) {
        h = LCD_HEIGHT - y;
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

int main() {
    stdio_init_all();
    initPins();
    initLCD();
    
    while(true) {}
}