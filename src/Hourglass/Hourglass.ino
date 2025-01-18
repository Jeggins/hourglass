#include <MD_MAX72xx.h>
#include <SPI.h>

#define MAX_DEVICES 8  // 8 Panels insgesamt (2 Reihen à 4 Panels)
#define CLK_PIN 13     // Clock Pin
#define DATA_PIN 11    // Data Pin
#define CS_PIN 10      // Chip Select Pin

MD_MAX72XX mx = MD_MAX72XX(MD_MAX72XX::FC16_HW, CS_PIN, MAX_DEVICES);

//Variables hourglass general
const int hourglassTopRowRight = 0;
const int hourglassTopRowLeft = 63;
const int hourglassMidRow = 12;
const int hourglassBottomRow = 23;
const int hourglassSandGrains = 49;

//Variables for falling sand
const int sandColumn = 0;
const int maxSandConst = 4;
int maxSand = maxSandConst;
int sandBottom = hourglassBottomRow;
int nextSand = 0;

struct Sand
{
  int row;
  bool active = false;
};

//Timer variables
const int hourglassTimerDefaultValue = 3;
const int sandUpdateSpeedMillis = 80;
const int secondMillis = 1000;
const int milliseconds = 1;
int secondsCounter = 0;
int hourglassTimer = hourglassTimerDefaultValue;
int hourglassTimerSeconds = hourglassTimer * 60;
int sandTransitionMillis = hourglassTimerSeconds * 970L / hourglassSandGrains; // calculates the duration of the hourglass in milliseconds but only 97% of the time, to be sure that sand is drained completely
int sandTransitionCounter = 0;
long sandTimer = 0;
long secondsTimer = 0;
long millisTimer = 0;

//Variables for numbers
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

//Variables for sand transition
int topRow = 1;
int topColumn = 0;
int botRow = hourglassBottomRow;
int botColumn = 0;
int[][]
int maxColumnPerRow[10] = {4, 5, 6, 6, 6, 5, 3, 2, 1, 0}; // bottom part max column per row


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
  Serial.println(sandTransitionMillis);
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

void updateSandTransition()
{
  if(sandTransitionCounter >= sandTransitionMillis)
  {
    updateTopTratsition();
    updateBottomTransition();
    sandTransitionCounter = 0;
  }

  sandTransitionCounter++;
}

void updateTopTratsition()
{
  mx.setPoint(topColumn, hourglassTopRowRight + topRow, false);
  mx.setPoint(topColumn, hourglassTopRowLeft - topRow, false);

  if(mx.getPoint(topColumn + 1, topRow))
  {
    topColumn++;
  }
  else
  {
    topColumn = 0;
    topRow++;
  }
}

void updateBottomTransition()
{
  bool sandGrainPlaced = false;


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

  if(currentTimestamp - millisTimer >= milliseconds)
  {
    millisTimer = currentTimestamp;

    if(hourglassTimerSeconds > 0)
    {
      updateSandTransition();
    }
  }

  if(currentTimestamp - sandTimer >= sandUpdateSpeedMillis) //is executed as often as sandUpdateSpeedMillis is set. 
  {
    sandTimer = currentTimestamp;
    
    updateFallingSand();
  }
  
  if(currentTimestamp - secondsTimer >= secondMillis) //is executed every second
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
