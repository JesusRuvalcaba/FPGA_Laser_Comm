
#include <stdint.h>
#include "chu_init.h"
#include "xadc_core.h"
#include "gpio_cores.h"

XadcCore adc(get_slot_addr(BRIDGE_BASE, S5_XDAC));
GpoCore laser(get_slot_addr(BRIDGE_BASE, S2_LED));
GpiCore btn(get_slot_addr(BRIDGE_BASE, S7_BTN));

void print_bin8(uint8_t value) {
    for (int i = 7; i >= 0; i--) uart.disp((value >> i) & 1);
}

uint16_t read_brightness() {
    uint16_t raw = adc.read_raw(2);
    return raw >> 4;
}

uint16_t pwm_symbol_and_average(uint8_t symbol) {
    const int period_us = 10000;
    int duty_us;

    if (symbol == 0b00) duty_us = 0;
    else if (symbol == 0b01) duty_us = period_us / 3;
    else if (symbol == 0b10) duty_us = (period_us * 2) / 3;
    else duty_us = period_us;

    uint32_t sum = 0;
    int samples = 0;

    for (int t = 0; t < period_us; t += 100) {
        if (t < duty_us) laser.write(1);
        else laser.write(0);

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

void receiver_from_brightness(uint16_t brightness) {
    static uint8_t symbols[4];
    static int count = 0;

    uint8_t symbol = brightness_to_symbol(brightness);

    symbols[count++] = symbol;

    if (count == 4) {
        uint8_t byte = (symbols[0] << 6) |
                       (symbols[1] << 4) |
                       (symbols[2] << 2) |
                       symbols[3];

        uart.disp("RX Symbols: ");
        for (int i = 0; i < 4; i++) {
            uart.disp((symbols[i] >> 1) & 1);
            uart.disp(symbols[i] & 1);
            if (i < 3) uart.disp(" ");
        }

        uart.disp("  RX Byte: ");
        print_bin8(byte);

        uart.disp("  Char: ");
        uart.disp((char)byte);

        uart.disp("\n\r");

        count = 0;
    }
}

void button_brightness_test() {
    uint32_t b = btn.read();
    uint8_t symbol = 0xFF;

    if (b & 0x01) symbol = 0b11;      // M18 = 100%
    else if (b & 0x02) symbol = 0b00; // M17 = 0%
    else if (b & 0x04) symbol = 0b01; // P18 = 33%
    else if (b & 0x08) symbol = 0b10; // P17 = 66%
    else return;

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

    while (1) {
        int index = 0;

        uart.disp("\n\rType message, then press ENTER:\n\r");
        uart.disp("N17 = calibration\n\r");
        uart.disp("M18=100%, M17=0%, P18=33%, P17=66%\n\r");

        while (1) {
            if (btn.read() & 0x10) {
                calibrate();
                sleep_ms(500);
            }

            button_brightness_test();

            uint8_t c = uart.rx_byte();

            if (c == 0xFF) continue;

            if (c == '\r' || c == '\n') {
                msg[index] = '\0';
                break;
            }

            if (index < 127) {
                msg[index++] = (char)c;
                uart.disp((char)c);
            }
        }

        uart.disp("\n\rTransmitting message...\n\r");

        for (int i = 0; msg[i] != '\0'; i++) {
            uint8_t byte = (uint8_t)msg[i];

            for (int j = 3; j >= 0; j--) {
                uint8_t tx_symbol = (byte >> (j * 2)) & 0x03;
                uint16_t avg_brightness = pwm_symbol_and_average(tx_symbol);
                receiver_from_brightness(avg_brightness);
            }
        }

        uart.disp("Done.\n\r");
    }
}

int main() {
    uart.set_baud_rate(9600);
    transmitter();
    return 0;
}
