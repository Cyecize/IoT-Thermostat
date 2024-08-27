#include <math.h> // Make sure to include this
#include "Arduino.h"
#include "HT16K33.h"
#include "util/TaskScheduler.h"

// Configurable constants
const float MIN_TARGET_TEMP = 0;
const float MAX_TARGET_TEMP = 40;
const float TRIGGER_TEMP = 3; // Degrees above target temp
const int SWITCH_PIN = 13;
const int THERMISTOR_POWER_PIN = 12; // Digital pin used to power the thermistor
const int MIN_KNOB_STEP = 25;
const int BAUD_RATE = 9600;
const int THERMISTOR_PIN = 0; // Thermistor input pin
const int KNOB_PIN = 1; // Potentiometer input pin
const int KNOB_POWER_PIN = 11;
// https://www.thinksrs.com/downloads/programs/therm%20calc/ntccalibrator/ntccalculator.html
// Coefficients for Steinhart-Hart equation
const float c1 = 1.328191148e-03, c2 = 2.000112940e-04, c3 = 2.350560518e-07;

// Variables used for temp reading
int thermistorData; // Analog signal read from Thermistor
int knobData; // Analog signal read from Potentiometer
int lastKnownKnobData;
const float R1 = 10000.0; // Fixed resistance value in the circuit
float R2, logR2, temp; // Variables for resistance, log value, and temperature
float targetTemp;
bool isOn = false;

// Rolling average configuration
const int ROLLING_AVG_SIZE = 32;
float readings[ROLLING_AVG_SIZE]; // Array to store the readings
int readIndex = 0; // Current index in the readings array
float total = 0; // Sum of the readings
float average = 0; // Computed average

// Display
HT16K33 seg(0x70);
const int DISPLAY_LOCK_MS = 1500;
bool displayIsLocked = false;
TaskScheduler displayLockScheduler;

void display(float f, bool prioritize) {
    if (!prioritize && displayIsLocked) {
        return;
    }

    if (prioritize) {
        // Lock the display and set a scheduler to release it.
        if (!displayIsLocked) {
            displayIsLocked = true;
        }

        displayLockScheduler.reset();
        displayLockScheduler.start();
    }

    seg.displayFloat(f);
}

void processThermistorData() {
    // Power the thermistor
    digitalWrite(THERMISTOR_POWER_PIN, HIGH);
    delay(15); // Give the thermistor some time to stabilize

    thermistorData = analogRead(THERMISTOR_PIN); // Read the analog signal from the Thermistor

    // Check if thermistorData is zero
    if (thermistorData == 0) {
        Serial.println("thermistorData is zero! Avoiding division by zero.");
        digitalWrite(THERMISTOR_POWER_PIN, LOW); // Turn off the thermistor power
        return;
    }

    R2 = R1 * (1023.0 / (float) thermistorData - 1.0); // Compute Thermistor resistance from analog read value
    logR2 = log(R2); // Take natural log of the resistance

    // Steinhart-Hart equation to compute temperature in Kelvin
    temp = c1 + (c2 * logR2) + (c3 * logR2 * logR2 * logR2);
    temp = 1.0 / temp;
    temp = temp - 273.15; // Convert temperature from Kelvin to Celsius

    // Update the rolling average
    total -= readings[readIndex];
    readings[readIndex] = temp;
    total += readings[readIndex];
    readIndex = (readIndex + 1) % ROLLING_AVG_SIZE;
    average = total / ROLLING_AVG_SIZE;

    Serial.print("Filtered AVG Temperature in Celsius: ");
    Serial.println(average);

    float avgForDisplay = (int) (average * 10) / 10.0f;
    display(avgForDisplay, false);

    if (average > targetTemp + TRIGGER_TEMP && !isOn) {
        Serial.println("Turning relay on");
        digitalWrite(SWITCH_PIN, HIGH);
        isOn = true;
    }

    if (average < targetTemp && isOn) {
        Serial.println("Turning relay off");
        digitalWrite(SWITCH_PIN, LOW);
        isOn = false;
    }

    // Turn off the thermistor power
    digitalWrite(THERMISTOR_POWER_PIN, LOW);
}

void setup() {
    Serial.begin(BAUD_RATE);
    Serial.println("Starting...");

    pinMode(SWITCH_PIN, OUTPUT);
    pinMode(THERMISTOR_POWER_PIN, OUTPUT);
    pinMode(KNOB_POWER_PIN, OUTPUT);
    targetTemp = MIN_TARGET_TEMP;
    Wire.begin();
    Wire.setClock(100000);
    seg.begin();
    seg.displayOn();

    // Initialize the readings array with zeros
    for (int i = 0; i < ROLLING_AVG_SIZE; i++) {
        readings[i] = 0;
    }

    for (int i = 0; i < ROLLING_AVG_SIZE; ++i) {
        processThermistorData();
        delay(25);
    }

    displayLockScheduler.init(DISPLAY_LOCK_MS, false, [] {
        displayIsLocked = false;
    });
}

void processKnobData() {
    // Power the potentiometer
    digitalWrite(KNOB_POWER_PIN, HIGH);
    delay(15); // Give the potentiometer some time to stabilize

    knobData = analogRead(KNOB_PIN);

    if (abs(lastKnownKnobData - knobData) <= MIN_KNOB_STEP) {
        digitalWrite(KNOB_POWER_PIN, LOW);
        return;
    }

    lastKnownKnobData = knobData;
    float normalizedValue = knobData / 1023.0f;

    // Interpolate within the target range
    targetTemp = MIN_TARGET_TEMP + normalizedValue * (MAX_TARGET_TEMP - MIN_TARGET_TEMP);
    Serial.println(targetTemp);
    display(targetTemp, true);
    digitalWrite(KNOB_POWER_PIN, LOW);
}

void loop() {
    displayLockScheduler.tick();
    processThermistorData();
    processKnobData();
    delay(500);
}
