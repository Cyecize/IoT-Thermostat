#include <math.h> // Make sure to include this
#include "Arduino.h"


const float TARGET_TEMP = 25.1;
const float TRIGGER_TEMP = 28.0;
const int SWITCH_PIN = 13;

const int THERMISTOR_PIN = 0; // Thermistor input pin
int Vo; // Analog signal read from Thermistor
float R1 = 10000.0; // Fixed resistance value in the circuit
float R2, logR2, T; // Variables for resistance, log value, and temperature
bool isOn = false;

// https://www.thinksrs.com/downloads/programs/therm%20calc/ntccalibrator/ntccalculator.html
const float c1 = 1.328191148e-03, c2 = 2.000112940e-04, c3 = 2.350560518e-07; // Coefficients for Steinhart-Hart equation

void setup() {
    Serial.begin(9600); // Begin serial communication at 9600 baud rate
    Serial.println("Starting...");

    pinMode(SWITCH_PIN, OUTPUT);
}

void loop() {
    Vo = analogRead(THERMISTOR_PIN); // Read the analog signal from the Thermistor

    // Check if Vo is zero
    if (Vo == 0) {
        Serial.println("Vo is zero! Avoiding division by zero.");
        delay(500);
        return;
    }

    R2 = R1 * (1023.0 / (float) Vo - 1.0); // Compute Thermistor resistance from analog read value

    // Print intermediate values for debugging
    // Serial.print("Vo: ");
    // Serial.println(Vo);
    // Serial.print("R2: ");
    // Serial.println(R2);

    logR2 = log(R2); // Take natural log of the resistance

    // Print logged resistance
    // Serial.print("log(R2): ");
    // Serial.println(logR2);

    // Steinhart-Hart equation to compute temperature in Kelvin
    T = c1 + (c2 * logR2) + (c3 * logR2 * logR2 * logR2);
    // Serial.print("Steinhart-Hart raw value: ");
    // Serial.println(T);

    if (T != 0) {
        T = 1.0 / T;

        // Print temperature in Kelvin
        // Serial.print("Temperature in Kelvin: ");
        // Serial.println(T);

        T = T - 273.15; // Convert temperature from Kelvin to Celsius

        // Print temperature in Celsius
        Serial.print("Temperature in Celsius: ");
        Serial.println(T);

        if (T > TRIGGER_TEMP && !isOn) {
            Serial.println("Turning on");
            digitalWrite(SWITCH_PIN, HIGH);
            isOn = true;
        }

        if (T < TARGET_TEMP && isOn) {
            Serial.println("Turning off");
            digitalWrite(SWITCH_PIN, LOW);
            isOn = false;
        }
    } else {
        Serial.println("Steinhart-Hart equation resulted in zero!");
    }

    delay(500); // Wait for 0.5 seconds before the next loop iteration
}
