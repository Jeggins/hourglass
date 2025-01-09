#include <MD_MAX72xx.h>
#include <SPI.h>

#define MAX_DEVICES 8  // 8 Panels insgesamt (2 Reihen à 4 Panels)
#define CLK_PIN 13     // Clock Pin
#define DATA_PIN 11    // Data Pin
#define CS_PIN 9       // Chip Select Pin

MD_MAX72XX mx = MD_MAX72XX(MD_MAX72XX::FC16_HW, CS_PIN, MAX_DEVICES);

//Variablen Sanduhr allgemein
const int hourglassTopRowRight = 0;
const int hourglassTopRowLeft = 63;
const int hourglassMidRow = 12;
const int hourglassBottomRow = 23;
int iteration = 0;

//Variablen für fallende Sandkörner
const int sandColumn = 0;
const int maxSandConst = 4;
int maxSand = maxSandConst;
int sandBottom = hourglassBottomRow;
int nextSand = 0;

//Timer Variablen
unsigned int hourglassTimer = 15;
unsigned long lastTime = 0;
const int updateSpeedMillis = 80;

struct Sand
{
  int row;
  bool active = false;
};

Sand sand[maxSandConst];

void setup() 
{
  Serial.begin(9600);  // Serielle Schnittstelle initialisieren
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 1); // Helligkeit einstellen
  mx.clear();

  // Sandkörner StartReihe und Status initialisieren
  for(int i = 0; i < maxSand; i++)
  {
    sand[i].row = hourglassMidRow;
    sand[i].active = false;
  }

  sand[nextSand].active = true;
  activateSandLed(sand[nextSand]);

  // LED auf Panel 2 einschalten (2. Panel von rechts in der oberen Reihe)
  //mx.setPoint(1, 0, true); // Panel 2 beginnt bei Spalte 8, Spalte 1, Zeile 0

  // LED auf Panel 7 einschalten (3. Panel von rechts in der unteren Reihe)
  //mx.setPoint(0, 6, true); // Panel 7 beginnt bei Spalte 48, Spalte 1, Zeile 0

  //mx.clear();
  setInitialHourglass();
}

void setInitialHourglass()
{
  byte startValue[4][8] = 
  {
    0x00,0x3f,0x7f,0x7f,0x7f,0x3f,0x1f,0x0f, //Panel 1
    0x07,0x03,0x01,0x01,0x00,0x00,0x00,0x00, //Panel 2
    0x00,0x00,0x00,0x00,0x01,0x01,0x03,0x07, //Panel 7
    0x0f,0x1f,0x3f,0x7f,0x7f,0x7f,0x3f,0x00  //Panel 8
  };
  
  byte panelValue[4] = {0, 1, 6, 7};

  for(int panel = 0; panel < sizeof(panelValue); panel++)
  {
    for(int col = 0; col < 8; col++)
    {
      mx.setColumn(panelValue[panel], col, startValue[panel][col]);
    }
  }
}

void updateFallingSand()
{
  for(int i = 0; i < maxSand; i++)
  {
    if(sand[i].active == true)
    {
      deactivateSandLed(sand[i]);
      if(sand[i].row < sandBottom)
      {
        sand[i].row++;
        activateSandLed(sand[i]);
      }
      else
      {
        sand[i].row = hourglassMidRow;
        sand[i].active = false;
      }
    }

    if(i == 0)
    {
      if(sand[maxSand - 1].row == 14 && hourglassTimer > 0)
      {
        sand[i].active = true;
        activateSandLed(sand[i]);
      }
    }
    else
    {
      if(sand[i - 1].row == 15 && hourglassTimer > 0)
      {
        sand[i].active = true;
        activateSandLed(sand[i]);
      }
    }
  }
}

void activateSandLed(Sand sand)
{
  mx.setPoint(sandColumn, hourglassTopRowRight + sand.row, true);
  mx.setPoint(sandColumn, hourglassTopRowLeft - sand.row, true);
}

void deactivateSandLed(Sand sand)
{
  mx.setPoint(sandColumn, hourglassTopRowRight + sand.row, false);
  mx.setPoint(sandColumn, hourglassTopRowLeft - sand.row, false);
}

void calculateMaxSand()
{
  maxSand = (sandBottom - hourglassMidRow + 1) / 3;
}

void loop() 
{
  if (millis() - lastTime >= updateSpeedMillis) 
  {
    lastTime = millis();
    iteration++;

    // if(iteration % 20 == 0 && sandBottom > 15)
    // {
    //   iteration = 0;
    //   Serial.print("sandBottom = ");
    //   Serial.println(sandBottom);
    //   mx.setPoint(0, sandBottom, true);
    //   sandBottom--;
    // }
    
    updateFallingSand();
    
    //calculateMaxSand();
  }
}
