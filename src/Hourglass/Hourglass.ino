#include <MD_MAX72xx.h>
#include <SPI.h>

// Definition der Hardware
#define MAX_DEVICES 8  // 8 Panels insgesamt (2 Reihen à 4 Panels)
#define CLK_PIN 13     // Clock Pin
#define DATA_PIN 11    // Data Pin
#define CS_PIN 9       // Chip Select Pin

MD_MAX72XX mx = MD_MAX72XX(MD_MAX72XX::FC16_HW, CS_PIN, MAX_DEVICES);

unsigned int hourglassTimer = 15;
unsigned long lastTime = 0;

void setup() 
{
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 1); // Helligkeit einstellen
  mx.clear();

  // LED auf Panel 2 einschalten (2. Panel von rechts in der oberen Reihe)
  mx.setPoint(0, 0, true); // Panel 2 beginnt bei Spalte 8, Spalte 1, Zeile 0

  // LED auf Panel 7 einschalten (3. Panel von rechts in der unteren Reihe)
  mx.setPoint(0, 6, true); // Panel 7 beginnt bei Spalte 48, Spalte 1, Zeile 0

  mx.clear();
  setInitialHourglass();
}

void setInitialHourglass()
{
  mx.setRow(1, 1, 255);
}

void firstSand()
{
  int maxParticles = 6;
  int gap = 2;
  
  int rowOfRightParticle = 
  
}

void loop() 
{
  if (millis() - lastTime >= 200) 
  {
    lastTime = millis();
    
    if(hourglassTimer > 0)
    {

    }
  }
}
