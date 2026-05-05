#include <stdint.h>
#include "chu_init.h"
#include "xadc_core.h"
#include "gpio_cores.h"
#include "ps2_core.h"

Ps2Core ps2(get_slot_addr(BRIDGE_BASE, S11_PS2));
XadcCore adc(get_slot_addr(BRIDGE_BASE, S5_XDAC));
GpoCore laser(get_slot_addr(BRIDGE_BASE, S2_LED));

const uint8_t START_KEY = 0xE7;
const uint8_t STOP_KEY  = 0x1D;

/* ===================== BASIC UTILS ===================== */

uint16_t read_brightness() {
    uint16_t raw = adc.read_raw(2);
    return raw >> 4;
}

uint16_t pwm_symbol_and_average(uint8_t symbol) {
    const int period_us = 10000;
    int duty_us;

    if (symbol == 0b00) 
        duty_us = 0;    
    else if (symbol == 0b01) 
        duty_us = period_us / 3;
    else if (symbol == 0b10) 
        duty_us = (period_us * 2) / 3;
    else 
        duty_us = period_us;

    uint32_t sum = 0;
    int samples = 0;

    for (int t = 0; t < period_us; t += 100) {
        laser.write(t < duty_us ? 1 : 0);
        sleep_us(100);
        sum += read_brightness();
        samples++;
    }

    laser.write(0);
    return sum / samples;
}

uint8_t brightness_to_symbol(uint16_t brightness) {
    if (brightness > 3000) return 0b00;
    else if (brightness > 1700) return 0b01;
    else if (brightness > 400) return 0b10;
    else return 0b11;
}

/* ===================== TRANSMIT ===================== */

void transmit_byte(uint8_t byte) {
    for (int j = 3; j >= 0; j--) {
        uint8_t symbol = (byte >> (j * 2)) & 0x03;
        pwm_symbol_and_average(symbol);
    }
}

void transmit_message(char msg[]) {
    uart.disp("\n\rTX: ");

    transmit_byte(START_KEY);

    for (int i = 0; msg[i] != '\0'; i++) {
        uart.disp(msg[i]);
        transmit_byte((uint8_t)msg[i]);
    }

    transmit_byte(STOP_KEY);

    uart.disp("\n\rTX done.\n\r");
}

/* ===================== RECEIVE ===================== */

uint16_t receive_symbol_average() {
    const int symbol_time_us = 10000;
    const int step = 100;

    uint32_t sum = 0;
    int samples = 0;

    for (int t = 0; t < symbol_time_us; t += step) {
        sleep_us(step);
        sum += read_brightness();
        samples++;
    }

    return sum / samples;
}

uint8_t receive_byte() {
    uint8_t byte = 0;

    for (int i = 0; i < 4; i++) {
        uint16_t avg = receive_symbol_average();
        uint8_t symbol = brightness_to_symbol(avg);
        byte = (byte << 2) | symbol;
    }

    return byte;
}

void process_received_byte(uint8_t byte) {
    static bool receiving = false;

    if (!receiving) {
        if (byte == START_KEY) {
            receiving = true;
            uart.disp("\n\rRX: ");
        }
    }
    else {
        if (byte == STOP_KEY) {
            receiving = false;
            uart.disp("\n\rRX done.\n\r");
        }
        else {
            uart.disp((char)byte);
        }
    }
}

/* ===================== PS2 INPUT ===================== */

bool get_keyboard_char(char *c) {
    return ps2.get_kb_ch(c);
}

/* ===================== MAIN LOOP ===================== */

void chat_loop() {
    char msg[128];
    int index = 0;

    uart.disp("\n\r--- Optical Chat Ready ---\n\r");
    uart.disp("Type on PS/2 keyboard. ENTER = send\n\r");

    while (1) {
        char ch;

        /* ---- KEYBOARD INPUT ---- */
        if (get_keyboard_char(&ch)) {

            if (ch == '\r' || ch == '\n') {
                msg[index] = '\0';

                if (index > 0) {
                    transmit_message(msg);
                    index = 0;
                }
            }
            else if (ch == 8 || ch == 127) {
                if (index > 0) {
                    index--;
                    uart.disp("\b \b");
                }
            }
            else if (index < 127) {
                msg[index++] = ch;
                uart.disp(ch);
            }
        }

        /* ---- RECEIVER ---- */
        uint8_t byte = receive_byte();
        process_received_byte(byte);
    }
}

/* ===================== MAIN ===================== */

int main() {
    uart.set_baud_rate(9600);
    chat_loop();
    return 0;
}
