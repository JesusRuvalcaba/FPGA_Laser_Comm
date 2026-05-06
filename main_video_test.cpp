#include <stdint.h>
#include "chu_init.h"
#include "xadc_core.h"
#include "gpio_cores.h"
#include "ps2_core.h"
#include "vga_core.h"

XadcCore adc(get_slot_addr(BRIDGE_BASE, S5_XDAC));
GpoCore laser(get_slot_addr(BRIDGE_BASE, S2_LED));
GpiCore btn(get_slot_addr(BRIDGE_BASE, S7_BTN));
Ps2Core ps2(get_slot_addr(BRIDGE_BASE, S11_PS2));

FrameCore frame(FRAME_BASE);
OsdCore osd(get_sprite_addr(BRIDGE_BASE, V2_OSD));

const int BLACK = 0x000;
const int RED   = 0xF00;

int ball_x = 320;
int ball_y = 240;
const int BALL_R = 12;
const int MOVE_STEP = 25;

void osd_print(int x, int y, const char *s) {
    int i = 0;
    while (s[i] != '\0') {
        osd.wr_char(x + i, y, s[i]);
        i++;
    }
}

void draw_vga_text() {
    osd.bypass(0);
    osd.clr_screen();
    osd.set_color(0xFFF, 0x000);

    osd_print(2, 2,  "Type a command:");
    osd_print(2, 4,  "UP");
    osd_print(2, 5,  "DOWN");
    osd_print(2, 6,  "LEFT");
    osd_print(2, 7,  "RIGHT");
    osd_print(2, 9,  "Press ENTER to transmit.");
}

void draw_ball_at(int cx, int cy, int color) {
    for (int y = cy - BALL_R; y <= cy + BALL_R; y++) {
        for (int x = cx - BALL_R; x <= cx + BALL_R; x++) {
            if (x >= 0 && x < 640 && y >= 0 && y < 480) {
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
    draw_ball_at(ball_x, ball_y, RED);
}

void erase_ball() {
    draw_ball_at(ball_x, ball_y, BLACK);
}

void redraw_screen() {
    frame.bypass(0);
    frame.clr_screen(BLACK);
    draw_ball();
    draw_vga_text();
}

void move_ball_command(const char *cmd) {
    erase_ball();

    if ((cmd[0] == 'U' || cmd[0] == 'u') &&
        (cmd[1] == 'P' || cmd[1] == 'p') &&
        cmd[2] == '\0') {
        ball_y -= MOVE_STEP;
    }
    else if ((cmd[0] == 'D' || cmd[0] == 'd') &&
             (cmd[1] == 'O' || cmd[1] == 'o') &&
             (cmd[2] == 'W' || cmd[2] == 'w') &&
             (cmd[3] == 'N' || cmd[3] == 'n') &&
             cmd[4] == '\0') {
        ball_y += MOVE_STEP;
    }
    else if ((cmd[0] == 'L' || cmd[0] == 'l') &&
             (cmd[1] == 'E' || cmd[1] == 'e') &&
             (cmd[2] == 'F' || cmd[2] == 'f') &&
             (cmd[3] == 'T' || cmd[3] == 't') &&
             cmd[4] == '\0') {
        ball_x -= MOVE_STEP;
    }
    else if ((cmd[0] == 'R' || cmd[0] == 'r') &&
             (cmd[1] == 'I' || cmd[1] == 'i') &&
             (cmd[2] == 'G' || cmd[2] == 'g') &&
             (cmd[3] == 'H' || cmd[3] == 'h') &&
             (cmd[4] == 'T' || cmd[4] == 't') &&
             cmd[5] == '\0') {
        ball_x += MOVE_STEP;
    }

    if (ball_x < BALL_R) ball_x = BALL_R;
    if (ball_x > 639 - BALL_R) ball_x = 639 - BALL_R;
    if (ball_y < BALL_R) ball_y = BALL_R;
    if (ball_y > 479 - BALL_R) ball_y = 479 - BALL_R;

    draw_ball();
}

void print_bin8(uint8_t value) {
    for (int i = 7; i >= 0; i--) {
        uart.disp((value >> i) & 1);
    }
}

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

void calibrate() {
    uint8_t symbols[4] = {0b00, 0b01, 0b10, 0b11};

    uart.disp("\n\r--- CALIBRATION START ---\n\r");

    for (int i = 0; i < 4; i++) {
        uint16_t avg = pwm_symbol_and_average(symbols[i]);
        uint8_t rx_symbol = brightness_to_symbol(avg);

        uart.disp("TX Symbol: ");
        uart.disp((symbols[i] >> 1) & 1);
        uart.disp(symbols[i] & 1);

        uart.disp("  Avg Brightness: ");
        uart.disp((int)avg);

        uart.disp("  RX Symbol: ");
        uart.disp((rx_symbol >> 1) & 1);
        uart.disp(rx_symbol & 1);

        uart.disp("\n\r");
    }

    uart.disp("--- CALIBRATION END ---\n\r");
}

void button_brightness_test() {
    uint32_t b = btn.read();
    uint8_t symbol = 0xFF;

    if (b & 0x01) {
        symbol = 0b11;
    }
    else if (b & 0x02) {
        symbol = 0b00;
    }
    else if (b & 0x04) {
        symbol = 0b01;
    }
    else if (b & 0x08) {
        symbol = 0b10;
    }
    else {
        return;
    }

    uart.disp("\n\r--- BUTTON TEST ---\n\r");

    uint32_t sum = 0;

    for (int i = 0; i < 300; i++) {
        sum += pwm_symbol_and_average(symbol);
    }

    uint16_t avg = sum / 300;

    uart.disp("TX Symbol: ");
    uart.disp((symbol >> 1) & 1);
    uart.disp(symbol & 1);

    uart.disp("  3-sec Avg Brightness: ");
    uart.disp((int)avg);

    uart.disp("\n\r--- TEST END ---\n\r");
}

void transmitter() {
    char msg[128];
    char rx_msg[128];

    while (1) {
        int index = 0;

        uart.disp("\n\rType command using PS/2 keyboard, then press ENTER:\n\r");
        uart.disp("Commands: UP, DOWN, LEFT, RIGHT\n\r");

        while (1) {
            if (btn.read() & 0x10) {
                calibrate();
                sleep_ms(500);
            }

            button_brightness_test();

            char c;

            if (!ps2.get_kb_ch(&c)) {
                continue;
            }

            if (c == '\r' || c == '\n') {
                msg[index] = '\0';
                break;
            }

            if (c == 8 || c == 127) {
                if (index > 0) {
                    index--;
                    uart.disp("\b \b");
                }
                continue;
            }

            if (index < 127) {
                msg[index++] = c;
                uart.disp(c);
            }
        }

        uart.disp("\n\rTransmitting command...\n\r");

        int rx_index = 0;

        for (int i = 0; msg[i] != '\0'; i++) {
            uint8_t tx_byte = (uint8_t)msg[i];
            uint8_t rx_byte = transmit_and_receive_byte(tx_byte);

            if (rx_index < 127) {
                rx_msg[rx_index++] = (char)rx_byte;
            }

            uart.disp("TX Char: ");
            uart.disp((char)tx_byte);
            uart.disp("  RX Char: ");
            uart.disp((char)rx_byte);
            uart.disp("\n\r");
        }

        rx_msg[rx_index] = '\0';

        uart.disp("Received command: ");
        uart.disp(rx_msg);
        uart.disp("\n\r");

        move_ball_command(rx_msg);

        uart.disp("Done.\n\r");
    }
}

int main() {
    uart.set_baud_rate(9600);

    redraw_screen();

    transmitter();

    return 0;
}
