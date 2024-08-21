#include <math.h> // Make sure to include this
#include "Arduino.h"
#include "HT16K33.h"


const float MIN_TARGET_TEMP = 0;
const float MAX_TARGET_TEMP = 40;
const float TRIGGER_TEMP = 3; // Degrees above target temp

const int SWITCH_PIN = 13;
const int MIN_KNOB_STEP = 25;
const int BAUD_RATE = 9600;

const int THERMISTOR_PIN = 0; // Thermistor input pin
const int KNOB_PIN = 1; // Potentiometer input pin
int thermistorData; // Analog signal read from Thermistor
int knobData; // Analog signal read from Potentiometer
int lastKnownKnobData;
const float R1 = 10000.0; // Fixed resistance value in the circuit
float R2, logR2, temp; // Variables for resistance, log value, and temperature
float targetTemp;
bool isOn = false;

HT16K33 seg(0x70);

// https://www.thinksrs.com/downloads/programs/therm%20calc/ntccalibrator/ntccalculator.html
// Coefficients for Steinhart-Hart equation
const float c1 = 1.328191148e-03, c2 = 2.000112940e-04, c3 = 2.350560518e-07;

void setup() {
    Serial.begin(BAUD_RATE);
    Serial.println("Starting...");

    pinMode(SWITCH_PIN, OUTPUT);

    targetTemp = MIN_TARGET_TEMP;

    Wire.begin();
    Wire.setClock(100000);
    seg.begin();

    seg.displayOn();
}

void display(float f) {
//    seg.displayInt(999);
        seg.displayFloat(f);
}

void processThermistorData() {
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
    display(temp);

    if (temp > targetTemp + TRIGGER_TEMP && !isOn) {
        Serial.println("Turning on");
        digitalWrite(SWITCH_PIN, HIGH);
        isOn = true;
    }

    if (temp < targetTemp && isOn) {
        Serial.println("Turning off");
        digitalWrite(SWITCH_PIN, LOW);
        isOn = false;
    }
}

void processKnobData() {
    knobData = analogRead(KNOB_PIN);

    if (abs(lastKnownKnobData - knobData) <= MIN_KNOB_STEP) {
        return;
    }

    lastKnownKnobData = knobData;
    float normalizedValue = knobData / 1023.0f;

    // Interpolate within the target range
    targetTemp = MIN_TARGET_TEMP + normalizedValue * (MAX_TARGET_TEMP - MIN_TARGET_TEMP);
    Serial.println(targetTemp);

    display(0);
    delay(500);
    display(targetTemp);
    delay(1500); // Give the user a bit of time to see his changes on the display
}

void loop() {
    processThermistorData();
    processKnobData();
    delay(500);
}
