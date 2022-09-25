#include <Arduino.h>
#include <SoftwareSerial.h>
#include <arduino_homekit_server.h>
#include <ESP8266WiFi.h>

const char *ssid = "netis_2.4G_85C441";
const char *password = "66027733";

const uint8_t rf_payload[] = {0xFD, 0x03, 0x65, 0x5B, 0x28, 0x41, 0xDF};

SoftwareSerial Serial2(D0, D1);

void wifi_connect() {
	WiFi.persistent(false);
	WiFi.mode(WIFI_STA);
	WiFi.setAutoReconnect(true);
	WiFi.begin(ssid, password);
	Serial.println("WiFi connecting...");
	while (!WiFi.isConnected()) {
		delay(100);
		Serial.print(".");
	}
	Serial.print("\n");
	Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
}

#define LOG_D(fmt, ...)   printf_P(PSTR(fmt "\n") , ##__VA_ARGS__);

//==============================
// HomeKit setup and loop
//==============================

extern "C" homekit_server_config_t config;
void my_homekit_setup() {
	arduino_homekit_setup(&config);
}

void my_homekit_loop() {
	arduino_homekit_loop();
}

void setup() {
	Serial.begin(115200);
	Serial2.begin(9600);
	Serial1.begin(9600);
	wifi_connect();
	// homekit_storage_reset(); // to remove the previous HomeKit pairing storage when you first run this new HomeKit example
	my_homekit_setup();
}

void loop() {
	my_homekit_loop();
	delay(10);
}

bool calc_len = false;
int ir_com_len = 0;
uint8_t ir_com_sum = 0;

void ac_ir_send_number(uint16_t n) {
    uint8_t t;
    n >>= 3; // / 8
    while (n > 127) {
        t = 0x80 | (n & 0x7F);
        n >>= 7;
        if (calc_len) {
            ir_com_len++;
        } else {
            ir_com_sum += t;
            Serial1.write(t);
        }
    }
    t = n;
    if (calc_len) {
        ir_com_len++;
    } else {
        ir_com_sum += t;
        Serial1.write(t);
    }
}

void ac_ir_send_abc(uint32_t a, uint32_t b, uint32_t c) {
    // A
    for (int i = 7; i >= 0; i--) {
        if (a & (1 << i)) {
            ac_ir_send_number(500);
            ac_ir_send_number(1600);
        } else {
            ac_ir_send_number(500);
            ac_ir_send_number(500);
        }
    }

    // A'
    for (int i = 7; i >= 0; i--) {
        if (!(a & (1 << i))) {
            ac_ir_send_number(500);
            ac_ir_send_number(1600);
        } else {
            ac_ir_send_number(500);
            ac_ir_send_number(500);
        }
    }

    // B
    for (int i = 7; i >= 0; i--) {
        if (b & (1 << i)) {
            ac_ir_send_number(500);
            ac_ir_send_number(1600);
        } else {
            ac_ir_send_number(500);
            ac_ir_send_number(500);
        }
    }

    // B'
    for (int i = 7; i >= 0; i--) {
        if (!(b & (1 << i))) {
            ac_ir_send_number(500);
            ac_ir_send_number(1600);
        } else {
            ac_ir_send_number(500);
            ac_ir_send_number(500);
        }
    }

    // C
    for (int i = 7; i >= 0; i--) {
        if (c & (1 << i)) {
            ac_ir_send_number(500);
            ac_ir_send_number(1600);
        } else {
            ac_ir_send_number(500);
            ac_ir_send_number(500);
        }
    }

    // C'
    for (int i = 7; i >= 0; i--) {
        if (!(c & (1 << i))) {
            ac_ir_send_number(500);
            ac_ir_send_number(1600);
        } else {
            ac_ir_send_number(500);
            ac_ir_send_number(500);
        }
    }
}

void ac_ir_send_frame(uint32_t a, uint32_t b, uint32_t c) {
    // L
    ac_ir_send_number(4500);
    ac_ir_send_number(4500);

    ac_ir_send_abc(a, b, c);

    // S
    ac_ir_send_number(550);
    ac_ir_send_number(5200);

    // L
    ac_ir_send_number(4500);
    ac_ir_send_number(4500);

    ac_ir_send_abc(a, b, c);
}

extern "C" void ac_ir_send(bool on, uint8_t temp, uint8_t fan, bool swing) {
    uint8_t a = 0xB2, b = 0x1F, c = 0x00;

    if (on) {
        // fan speed
        switch (fan) {
            case 1:
                b |= (0b101 << 5);
                break;
            case 2:
                b |= (0b100 << 5);
                break;
            case 3:
                b |= (0b010 << 5);
                break;
            case 4:
                b |= (0b001 << 5);
                break;
            default:
                break;
        }

        // fan speed
        switch (temp) {
            case 17:
                c |= (0b0000 << 4);
                break;
            case 18:
                c |= (0b0001 << 4);
                break;
            case 19:
                c |= (0b0011 << 4);
                break;
            case 20:
                c |= (0b0010 << 4);
                break;
            case 21:
                c |= (0b0110 << 4);
                break;
            case 22:
                c |= (0b0111 << 4);
                break;
            case 23:
                c |= (0b0101 << 4);
                break;
            case 24:
                c |= (0b0100 << 4);
                break;
            case 25:
                c |= (0b1100 << 4);
                break;
            case 26:
                c |= (0b1101 << 4);
                break;
            case 27:
                c |= (0b1001 << 4);
                break;
            case 28:
                c |= (0b1000 << 4);
                break;
            case 29:
                c |= (0b1010 << 4);
                break;
            case 30:
                c |= (0b1011 << 4);
                break;
            default:
                break;
        }
    } else {
        b = 0x7B;
        c = 0xE0;
    }

    uint8_t t = 0;

    t = 0x68;
    Serial1.write(t);
    calc_len = true;
    ir_com_len = 7;
    ac_ir_send_frame(a, b, c);
    t = ir_com_len & 0xFF;
    Serial1.write(t);
    t = (ir_com_len >> 8) & 0xFF;
    Serial1.write(t);
    t = 0xFF;
    Serial1.write(t);
    t = 0x22;
    Serial1.write(t);
    calc_len = false;
    ir_com_sum = 0xFF + 0x22;
    ac_ir_send_frame(a, b, c);
    t = ir_com_sum;
    Serial1.write(t);
    t = 0x16;
    Serial1.write(t);
}

extern "C" void ac_disp_ir() {
    uint8_t a = 0xB9, b = 0xF5, c = 0x09;

    uint8_t t = 0;

    t = 0x68;
    Serial1.write(t);
    calc_len = true;
    ir_com_len = 7;
    ac_ir_send_frame(a, b, c);
    t = ir_com_len & 0xFF;
    Serial1.write(t);
    t = (ir_com_len >> 8) & 0xFF;
    Serial1.write(t);
    t = 0xFF;
    Serial1.write(t);
    t = 0x22;
    Serial1.write(t);
    calc_len = false;
    ir_com_sum = 0xFF + 0x22;
    ac_ir_send_frame(a, b, c);
    t = ir_com_sum;
    Serial1.write(t);
    t = 0x16;
    Serial1.write(t);
}

extern "C" void light_rf_send() {
	Serial2.write(rf_payload, sizeof(rf_payload));
}
