#include <stdint.h>
#include "chu_init.h"
#include "xadc_core.h"
#include "gpio_cores.h"
#include "ps2_core.h"
#include "vga_core.h"

/* ===================== HARDWARE CORES ===================== */

XadcCore adc(get_slot_addr(BRIDGE_BASE, S5_XDAC));
GpoCore laser(get_slot_addr(BRIDGE_BASE, S2_LED));
GpiCore btn(get_slot_addr(BRIDGE_BASE, S7_BTN));
Ps2Core ps2(get_slot_addr(BRIDGE_BASE, S11_PS2));

FrameCore frame(FRAME_BASE);
OsdCore osd(get_sprite_addr(BRIDGE_BASE, V2_OSD));

// Bypass ghost layer if it appears from hardware pipeline
GpvCore ghost(get_sprite_addr(BRIDGE_BASE, V3_GHOST));

/* ===================== CONSTANTS ===================== */

const int BLACK = 0x000;
const int RED   = 0xF00;
const int GREEN = 0x0F0;
const int BLUE  = 0x00F;
const int WHITE = 0xFFF;

const int SCREEN_W = 640;
const int SCREEN_H = 480;

const int BALL_R = 12;
const int MOVE_STEP = 25;

const int MAX_MSG_LEN = 128;

/* ===================== GLOBAL STATE ===================== */

int bg_color = BLACK;
int ball_x = 320;
int ball_y = 240;

/* ===================== OSD TEXT HELPERS ===================== */

void osd_print(int x, int y, const char *s) {
    int i = 0;

    while (s[i] != '\0') {
        osd.wr_char(x + i, y, s[i]);
        i++;
    }
}

void clear_osd_area(int x0, int y0, int w, int h) {
    for (int y = y0; y < y0 + h; y++) {
        for (int x = x0; x < x0 + w; x++) {
            osd.wr_char(x, y, OsdCore::NULL_CHAR);
        }
    }
}

void setup_osd_text() {
    osd.bypass(0);
    osd.clr_screen();

    // White text, transparent background
    osd.set_color(WHITE, OsdCore::CHROMA_KEY_COLOR);

    osd_print(2, 3, "TYPE A COMMAND");
    osd_print(2, 5, "UP DOWN LEFT RIGHT");
    osd_print(2, 7, "PRESS ENTER");

    osd_print(50, 0, "INPUT:");
}

void draw_input_area(const char *input_text) {
    clear_osd_area(50, 0, 30, 2);

    osd_print(50, 0, "INPUT:");
    osd_print(57, 0, input_text);
}

/* ===================== BALL DRAWING ===================== */

void draw_ball_at(int cx, int cy, int color) {
    for (int y = cy - BALL_R; y <= cy + BALL_R; y++) {
        for (int x = cx - BALL_R; x <= cx + BALL_R; x++) {
            if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
                int dx = x - cx;
                int dy = y - cy;

                if ((dx * dx + dy * dy) <= (BALL_R * BALL_R)) {
                    frame.wr_pix(x, y, color);
                }
            }
        }
    }
}

void draw_ball() {
    draw_ball_at(ball_x, ball_y, BLUE);
}

/* ===================== SCREEN DRAWING ===================== */

void draw_screen() {
    frame.bypass(0);

    // Bypass hardware ghost layer so it does not appear accidentally
    ghost.bypass(1);

    frame.clr_screen(bg_color);

    draw_ball();
    setup_osd_text();
    draw_input_area("");
}

void redraw_full_screen_with_new_bg(int new_bg_color) {
    bg_color = new_bg_color;
    draw_screen();
}

/* ===================== COMMAND LOGIC ===================== */

bool command_equals(const char *cmd, const char *target) {
    int i = 0;

    while (cmd[i] != '\0' && target[i] != '\0') {
        char a = cmd[i];
        char b = target[i];

        if (a >= 'a' && a <= 'z') {
            a -= 32;
        }

        if (b >= 'a' && b <= 'z') {
            b -= 32;
        }

        if (a != b) {
            return false;
        }

        i++;
    }

    return cmd[i] == '\0' && target[i] == '\0';
}

bool apply_command(const char *cmd) {
    bool recognized = true;

    if (command_equals(cmd, "UP")) {
        ball_y -= MOVE_STEP;
    }
    else if (command_equals(cmd, "DOWN")) {
        ball_y += MOVE_STEP;
    }
    else if (command_equals(cmd, "LEFT")) {
        ball_x -= MOVE_STEP;
    }
    else if (command_equals(cmd, "RIGHT")) {
        ball_x += MOVE_STEP;
    }
    else {
        recognized = false;
    }

    if (ball_x < BALL_R) {
        ball_x = BALL_R;
    }

    if (ball_x > SCREEN_W - 1 - BALL_R) {
        ball_x = SCREEN_W - 1 - BALL_R;
    }

    if (ball_y < BALL_R) {
        ball_y = BALL_R;
    }

    if (ball_y > SCREEN_H - 1 - BALL_R) {
        ball_y = SCREEN_H - 1 - BALL_R;
    }

    if (recognized) {
        redraw_full_screen_with_new_bg(GREEN);
    }
    else {
        redraw_full_screen_with_new_bg(RED);
    }

    return recognized;
}

/* ===================== OPTICAL TX/RX ===================== */

uint16_t read_brightness() {
    uint16_t raw = adc.read_raw(2);
    return raw >> 4;
}

uint16_t pwm_symbol_and_average(uint8_t symbol) {
    const int period_us = 10000;
    int duty_us;

    if (symbol == 0b00) {
        duty_us = 0;
    }
    else if (symbol == 0b01) {
        duty_us = period_us / 3;
    }
    else if (symbol == 0b10) {
        duty_us = (period_us * 2) / 3;
    }
    else {
        duty_us = period_us;
    }

    uint32_t sum = 0;
    int samples = 0;

    for (int t = 0; t < period_us; t += 100) {
        if (t < duty_us) {
            laser.write(1);
        }
        else {
            laser.write(0);
        }

        sleep_us(100);

        sum += read_brightness();
        samples++;
    }

    laser.write(0);

    return sum / samples;
}

uint8_t brightness_to_symbol(uint16_t brightness) {
    if (brightness > 3000) {
        return 0b00;
    }
    else if (brightness > 1700) {
        return 0b01;
    }
    else if (brightness > 400) {
        return 0b10;
    }
    else {
        return 0b11;
    }
}

uint8_t transmit_and_receive_byte(uint8_t byte) {
    uint8_t rx_byte = 0;

    for (int j = 3; j >= 0; j--) {
        uint8_t tx_symbol = (byte >> (j * 2)) & 0x03;

        uint16_t avg_brightness = pwm_symbol_and_average(tx_symbol);
        uint8_t rx_symbol = brightness_to_symbol(avg_brightness);

        rx_byte = (rx_byte << 2) | rx_symbol;
    }

    return rx_byte;
}

void transmit_receive_message(const char *tx_msg, char *rx_msg) {
    int rx_index = 0;

    for (int i = 0; tx_msg[i] != '\0'; i++) {
        uint8_t tx_byte = (uint8_t)tx_msg[i];
        uint8_t rx_byte = transmit_and_receive_byte(tx_byte);

        if (rx_index < MAX_MSG_LEN - 1) {
            rx_msg[rx_index++] = (char)rx_byte;
        }
    }

    rx_msg[rx_index] = '\0';
}

/* ===================== CALIBRATION ===================== */

void calibrate() {
    uint8_t symbols[4] = {0b00, 0b01, 0b10, 0b11};

    uart.disp("\n\r--- CALIBRATION START ---\n\r");

    for (int i = 0; i < 4; i++) {
        uint16_t avg = pwm_symbol_and_average(symbols[i]);
        uint8_t rx_symbol = brightness_to_symbol(avg);

        uart.disp("TX Symbol: ");
        uart.disp((symbols[i] >> 1) & 1);
        uart.disp(symbols[i] & 1);

        uart.disp(" Avg: ");
        uart.disp((int)avg);

        uart.disp(" RX Symbol: ");
        uart.disp((rx_symbol >> 1) & 1);
        uart.disp(rx_symbol & 1);

        uart.disp("\n\r");
    }

    uart.disp("--- CALIBRATION END ---\n\r");
}

/* ===================== KEYBOARD INPUT ===================== */

void read_keyboard_command(char *msg) {
    int index = 0;
    msg[0] = '\0';

    draw_input_area("");

    while (1) {
        if (btn.read() & 0x10) {
            calibrate();
            sleep_ms(500);
        }

        char c;

        if (!ps2.get_kb_ch(&c)) {
            continue;
        }

        if (c == '\r' || c == '\n') {
            msg[index] = '\0';
            draw_input_area("");
            break;
        }

        if (c == 8 || c == 127) {
            if (index > 0) {
                index--;
                msg[index] = '\0';
                draw_input_area(msg);
            }

            continue;
        }

        if (index < MAX_MSG_LEN - 1) {
            msg[index++] = c;
            msg[index] = '\0';
            draw_input_area(msg);
        }
    }
}

/* ===================== MAIN LOOP ===================== */

void main_loop() {
    char tx_msg[MAX_MSG_LEN];
    char rx_msg[MAX_MSG_LEN];

    while (1) {
        read_keyboard_command(tx_msg);
        transmit_receive_message(tx_msg, rx_msg);
        apply_command(rx_msg);
    }
}

/* ===================== MAIN ===================== */

int main() {
    uart.set_baud_rate(9600);

    draw_screen();

    main_loop();

    return 0;
}
