#include <MD_MAX72xx.h>
#include <SPI.h>

// Definition der Hardware
#define MAX_DEVICES 8  // 8 Panels insgesamt (2 Reihen à 4 Panels)
#define CLK_PIN 13     // Clock Pin
#define DATA_PIN 11    // Data Pin
#define CS_PIN 9       // Chip Select Pin

MD_MAX72XX mx = MD_MAX72XX(MD_MAX72XX::FC16_HW, CS_PIN, MAX_DEVICES);

// Variablen für die Sanduhr
int hourglassTimer = 15;        // Duration of the hourglass
int sandHeight = 0;             // Höhe des Sandberges unten
const int sandWidth = 2;        // Breite des Sandstroms (2 Spalten in der Mitte)
unsigned long previousMillis = 0;
const unsigned long sandFallInterval = 200;  // Intervall für das Rieseln des Sands

void setup() {
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 0); // Helligkeit einstellen
  mx.clear();
}

void loop() {
  unsigned long currentMillis = millis();

  // Sand rieseln lassen
  if (currentMillis - previousMillis >= sandFallInterval) {
    previousMillis = currentMillis;
    drawSand();
  }
}

// Funktion zum Zeichnen der Sanduhr
void drawSand() {
  mx.clear();

  // --- Obere Hälfte: Sand rieselt herunter ---
  // Rechte Panels: Sand rieselt von oben nach unten (Spalte 3 und 4)
  for (int row = 0; row < 8; row++) {
    if (random(0, 10) > 6) {
      mx.setPoint(3, row, true); // Spalte 3 auf rechten Panels
      mx.setPoint(4, row, true); // Spalte 4 auf rechten Panels
    }
  }

  // Linke Panels (auf Kopf): Sand rieselt ebenfalls von oben nach unten,
  // aber die Zeilen sind umgekehrt, daher wird die Y-Koordinate invertiert.
  for (int row = 0; row < 8; row++) {
    if (random(0, 10) > 6) {
      mx.setPoint(7 - 3, 7 - row, true); // Spalte 3 auf linken Panels
      mx.setPoint(7 - 4, 7 - row, true); // Spalte 4 auf linken Panels
    }
  }

  // --- Untere Hälfte: Sandberg aufbauen ---
  // Rechte Panels: Sandberg in der Mitte
  for (int row = 7; row > 7 - sandHeight; row--) {
    for (int col = 3; col < 5; col++) {
      mx.setPoint(col, row, true);
    }
  }

  // Linke Panels (auf Kopf): Sandberg in der Mitte (Zeilen invertiert)
  for (int row = 7; row > 7 - sandHeight; row--) {
    for (int col = 7 - 3; col <= 7 - 4; col++) {
      mx.setPoint(col, 7 - row, true);
    }
  }

  // Sandhöhe langsam erhöhen
  if (sandHeight < 7) {
    sandHeight++;
  }

  mx.update();
}
