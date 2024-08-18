#include <math.h> // Make sure to include this
#include "Arduino.h"


const float TARGET_TEMP = 25.1;
const float TRIGGER_TEMP = 28.0;
const int SWITCH_PIN = 13;
const int BAUD_RATE = 9600;

const int THERMISTOR_PIN = 0; // Thermistor input pin
int thermistorData; // Analog signal read from Thermistor
const float R1 = 10000.0; // Fixed resistance value in the circuit
float R2, logR2, temp; // Variables for resistance, log value, and temperature
bool isOn = false;

// https://www.thinksrs.com/downloads/programs/therm%20calc/ntccalibrator/ntccalculator.html
// Coefficients for Steinhart-Hart equation
const float c1 = 1.328191148e-03, c2 = 2.000112940e-04, c3 = 2.350560518e-07;

void setup() {
    Serial.begin(BAUD_RATE);
    Serial.println("Starting...");

    pinMode(SWITCH_PIN, OUTPUT);
}

void processData() {
    thermistorData = analogRead(THERMISTOR_PIN); // Read the analog signal from the Thermistor

    // Check if thermistorData is zero
    if (thermistorData == 0) {
        Serial.println("thermistorData is zero! Avoiding division by zero.");
        return;
    }

    R2 = R1 * (1023.0 / (float) thermistorData - 1.0); // Compute Thermistor resistance from analog read value

    logR2 = log(R2); // Take natural log of the resistance

    // Steinhart-Hart equation to compute temperature in Kelvin
    temp = c1 + (c2 * logR2) + (c3 * logR2 * logR2 * logR2);
    temp = 1.0 / temp;

    // Serial.print("Temperature in Kelvin: ");
    // Serial.println(temp);

    temp = temp - 273.15; // Convert temperature from Kelvin to Celsius

    Serial.print("Temperature in Celsius: ");
    Serial.println(temp);

    if (temp > TRIGGER_TEMP && !isOn) {
        Serial.println("Turning on");
        digitalWrite(SWITCH_PIN, HIGH);
        isOn = true;
    }

    if (temp < TARGET_TEMP && isOn) {
        Serial.println("Turning off");
        digitalWrite(SWITCH_PIN, LOW);
        isOn = false;
    }
}

void loop() {
    processData();
    delay(500);
}
