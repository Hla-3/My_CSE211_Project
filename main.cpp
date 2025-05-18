#include "mbed.h"

DigitalOut latchPin(D4);    // ST_CP (latch)
DigitalOut clockPin(D7);    // SH_CP (clock)
DigitalOut dataPin(D8);     // DS (data)

DigitalIn s1(A1);           // Reset Button (S1)
DigitalIn s3(A3);           // Voltage Display Button (S3)
AnalogIn pot(A0);           // Potentiometer input (ADC)

const uint8_t digitPattern[10] = {
    0xC0, 0xF9, 0xA4, 0xB0,
    0x99, 0x92, 0x82, 0xF8,
    0x80, 0x90
};

const uint8_t digitPos[4] = { 0x01, 0x02, 0x04, 0x08 };

volatile int elapsedSeconds = 0, elapsedMinutes = 0;
Ticker timerTicker;

void updateTime() {
    elapsedSeconds++;
    if (elapsedSeconds >= 60) {
        elapsedSeconds = 0;
        elapsedMinutes = (elapsedMinutes + 1) % 100;
    }
}

void shiftOutMSBFirst(uint8_t value) {
    for (int i = 7; i >= 0; i--) {
        dataPin = (value >> i) & 0x01;
        clockPin = 1;
        clockPin = 0;
    }
}

void writeToShiftRegister(uint8_t segments, uint8_t digit) {
    latchPin = 0;
    shiftOutMSBFirst(0xFF); // Turn off all segments
    shiftOutMSBFirst(0x00); // Disable all digits
    latchPin = 1;
    wait_us(10);

    latchPin = 0;
    shiftOutMSBFirst(segments);
    shiftOutMSBFirst(digit);
    latchPin = 1;
}

void displayNumber(int number, bool showDecimal = false, int decimalPos = -1) {
    int digits[4] = {
        (number / 1000) % 10,
        (number / 100) % 10,
        (number / 10) % 10,
        number % 10
    };

    for (int i = 0; i < 4; i++) {
        uint8_t pattern = digitPattern[digits[i]];
        if (showDecimal && i == decimalPos) {
            pattern &= 0x7F;  // Enable decimal point
        }
        writeToShiftRegister(pattern, digitPos[i]);
        ThisThread::sleep_for(2ms);
    }
}

int main() {
    s1.mode(PullUp);
    s3.mode(PullUp);
    timerTicker.attach(&updateTime, std::chrono::seconds(1));

    while (true) {
        if (!s1) {
            ThisThread::sleep_for(50ms);
            if (!s1) {
                elapsedSeconds = 0;
                elapsedMinutes = 0;
                while (!s1);
                ThisThread::sleep_for(100ms);
            }
        }

        float voltage = pot.read() * 3.3f;

        if (!s3) {
            ThisThread::sleep_for(20ms);
            if (!s3) {
                int voltageScaled = static_cast<int>(voltage * 100);  // e.g. 3.26 V → 326
                displayNumber(voltageScaled, true, 1);  // Show as X.XX
                continue;
            }
        }

        int displayValue = elapsedMinutes * 100 + elapsedSeconds;
        displayNumber(displayValue);
    }
}
