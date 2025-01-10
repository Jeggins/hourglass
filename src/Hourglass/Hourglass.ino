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

//Variablen für fallende Sandkörner
const int sandColumn = 0;
const int maxSandConst = 4;
int maxSand = maxSandConst;
int sandBottom = hourglassBottomRow;
int nextSand = 0;

//Timer Variablen
    //calculateMaxSand();
const int hourglassTimerDefaultValue = 15;
const int sandUpdateSpeedMillis = 80;
const int secondMillis = 1000;
int secondsCounter = 0;
int hourglassTimer = hourglassTimerDefaultValue;
int hourglassTimerSeconds = hourglassTimer * 60;
long sandTimer = 0;
long secondsTimer = 0;

//Variablen für Zahlen
byte numbersLeft[11][8] = 
  {
    0x00,0x0e,0x0a,0x0a,0x0a,0x0e,0x00,0x00, //0 left
    0x00,0x02,0x02,0x02,0x06,0x02,0x00,0x00, //1 left
    0x00,0x0e,0x08,0x0e,0x02,0x0e,0x00,0x00, //2 left
    0x00,0x0e,0x02,0x06,0x02,0x0e,0x00,0x00, //3 left
    0x00,0x02,0x02,0x0e,0x0a,0x0a,0x00,0x00, //4 left
    0x00,0x0e,0x02,0x0e,0x08,0x0e,0x00,0x00, //5 left
    0x00,0x0e,0x0a,0x0e,0x08,0x0e,0x00,0x00, //6 left
    0x00,0x02,0x02,0x02,0x02,0x0e,0x00,0x00, //7 left
    0x00,0x0e,0x0a,0x0e,0x0a,0x0e,0x00,0x00, //8 left
    0x00,0x0e,0x02,0x0e,0x0a,0x0e,0x00,0x00, //9 left
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00  //empty
  };

  byte numbersRight[11][8] = 
  {
    0x00,0x00,0x0e,0x0a,0x0a,0x0a,0x0e,0x00, //0 right
    0x00,0x00,0x04,0x06,0x04,0x04,0x04,0x00, //1 right
    0x00,0x00,0x0e,0x08,0x0e,0x02,0x0e,0x00, //2 right
    0x00,0x00,0x0e,0x08,0x0c,0x08,0x0e,0x00, //3 right
    0x00,0x00,0x0a,0x0a,0x0e,0x08,0x08,0x00, //4 right
    0x00,0x00,0x0e,0x02,0x0e,0x08,0x0e,0x00, //5 right
    0x00,0x00,0x0e,0x02,0x0e,0x0a,0x0e,0x00, //6 right
    0x00,0x00,0x0e,0x08,0x08,0x08,0x08,0x00, //7 right
    0x00,0x00,0x0e,0x0a,0x0e,0x0a,0x0e,0x00, //8 right
    0x00,0x00,0x0e,0x0a,0x0e,0x08,0x0e,0x00, //9 right
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00  //empty
  };

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
  setInitialHourglass();
  setInitialNumbers();
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

void setInitialNumbers()
{
  setTimeOnDisplay();
}

void updateMinutes()
{
  if(hourglassTimer > 0)
  {
    hourglassTimer--;
  }
}

void setTimeOnDisplay()
{
  int leftNumber = 0;
  int rightNumber = 0;
  if(hourglassTimer < 10)
  {
    leftNumber = 10; //select the empty slot
    rightNumber = hourglassTimer;
  }
  else
  {
    leftNumber = hourglassTimer / 10;
    rightNumber = hourglassTimer % 10;
  }
  
  for(int col = 0; col < 8; col++)
  {
    mx.setColumn(4, col, numbersLeft[leftNumber][col]);
    mx.setColumn(3, col, numbersRight[rightNumber][col]);
  }
}

void loop() 
{
  unsigned long currentTimestamp = millis();

  if (currentTimestamp - sandTimer >= sandUpdateSpeedMillis) 
  {
    sandTimer = currentTimestamp;
    
    updateFallingSand();
  }
  
  if(currentTimestamp - secondsTimer >= secondMillis)
  {
    secondsTimer = currentTimestamp;

    if(hourglassTimerSeconds > 0)
    {
      hourglassTimerSeconds--;
      secondsCounter++;

      if(secondsCounter >= 60)
      {
        secondsCounter = 0;
        updateMinutes();
        setTimeOnDisplay();
      }
    }
  }
}
