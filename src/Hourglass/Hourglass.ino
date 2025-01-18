#include <MD_MAX72xx.h>
#include <SPI.h>

#define MAX_DEVICES 8  // 8 Panels
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
const int hourglassTimerDefaultValue = 3; // Default value of the Hourglass Time. This time will be used when the Hourglass gets started out of standby.
const int sandUpdateSpeedMillis = 80; // Refreshtime in ms that will be used to visualize the falling sand.
const int secondMillis = 1000; //One second im ms.
const int milliseconds = 1; // Variable for one millisecond.
int hourglassTimer = hourglassTimerDefaultValue; // This is the actual Varable that will be used to set the hourglass timer. Can be changed via button.
int hourglassTimerSeconds = hourglassTimer * 60; // The time of the hourglass in seconds.
int sandTransitionMillis = hourglassTimerSeconds * 970L / hourglassSandGrains; // calculates the duration of the hourglass in milliseconds but only takes 97% of the time, to be sure that sand is drained completely.
int secondsCounter = 0;  //Variable to count up the passed seconds.
int sandTransitionCounter = 0; //Variable to count up the sand that already fell down.
long sandTransitionTimer = 0; //Variable to count up the passed time of the sand transition.
long secondsTimer = 0; //Timervariable for seconds.
long millisTimer = 0; //Timervariable for ms.

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
int sandGrainSlide[3][2] = {{0, 0}, {0, 0}, {0, 0}};
int sandGrainsToSlide = 0;
int sandGrainsAlreadySlid = 0;
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

  if(checkSandPile())
  {
    sandGrainPlaced = true;
  }

  while(!sandGrainPlaced)
  {
    if(botColumn <= maxColumnPerRow[hourglassBottomRow - botRow])
    {
      if(!mx.getPoint(botColumn, botRow))
      {
        sandGrainSlide[0][0] = botColumn;
        sandGrainSlide[0][1] = botRow;
        botColumn = 0;
        sandGrainSlide[1][0] = botColumn + 1;
        sandGrainSlide[1][1] = botRow - 1;
        sandGrainSlide[2][0] = botColumn;
        sandGrainSlide[2][1] = botRow - 2;
        
        sandGrainsToSlide = 2;
        sandGrainsAlreadySlid = 2;
        sandGrainPlaced = true;
      }
    }
  }
}

bool checkSandPile()
{
  if(!mx.getPoint(botColumn, botRow))
  {
    sandGrainSlide[0][0] = botColumn;
    sandGrainSlide[0][1] = botRow;
    sandGrainsToSlide = 0;
    sandGrainsAlreadySlid = 0;
    return true;
  }
  if(!mx.getPoint(botColumn + 1, botRow))
  {
    sandGrainSlide[0][0] = botColumn + 1;
    sandGrainSlide[0][1] = botRow;
    sandGrainSlide[1][0] = botColumn;
    sandGrainSlide[1][1] = botRow - 1;
    sandGrainsToSlide = 1;
    sandGrainsAlreadySlid = 1;
    return true;
  }
  if(!mx.getPoint(botColumn, botRow - 1))
  {
    sandGrainSlide[0][0] = botColumn;
    sandGrainSlide[0][1] = botRow - 1;
    sandGrainsToSlide = 0;
    sandGrainsAlreadySlid = 0;
    return true;
  }
  if(!mx.getPoint(botColumn + 2, botRow))
  {
    sandGrainSlide[0][0] = botColumn + 2;
    sandGrainSlide[0][1] = botRow;
    sandGrainSlide[1][0] = botColumn + 1;
    sandGrainSlide[1][1] = botRow - 1;
    sandGrainSlide[2][0] = botColumn;
    sandGrainSlide[2][1] = botRow - 2;
    sandGrainsToSlide = 2;
    sandGrainsAlreadySlid = 2;
    return true;
  }
  if(!mx.getPoint(botColumn + 1, botRow - 1))
  {
    sandGrainSlide[0][0] = botColumn + 1;
    sandGrainSlide[0][1] = botRow - 1;
    sandGrainSlide[1][0] = botColumn;
    sandGrainSlide[1][1] = botRow - 2;
    sandGrainsToSlide = 1;
    sandGrainsAlreadySlid = 1;
    return true;
  }
  if(!mx.getPoint(botColumn, botRow - 2))
  {
    sandGrainSlide[0][0] = botColumn;
    sandGrainSlide[0][1] = botRow - 2;
    sandGrainsToSlide = 0;
    sandGrainsAlreadySlid = 0;
    return true;
  }
}

void updateSandGrainSlides()
{
  if(sandGrainsAlreadySlid >= 0)
  {
    if(sandGrainsAlreadySlid == sandGrainsToSlide)
    {
      mx.setPoint(sandGrainSlide[sandGrainsAlreadySlid][0], hourglassTopRowRight + sandGrainSlide[sandGrainsAlreadySlid][1], true);
      mx.setPoint(sandGrainSlide[sandGrainsAlreadySlid][0], hourglassTopRowLeft - sandGrainSlide[sandGrainsAlreadySlid][1], true);
      sandGrainsAlreadySlid--;
    }
    else if(sandGrainsAlreadySlid < sandGrainsToSlide)
    {
      mx.setPoint(sandGrainSlide[sandGrainsAlreadySlid + 1][0], hourglassTopRowRight + sandGrainSlide[sandGrainsAlreadySlid + 1][1], false);
      mx.setPoint(sandGrainSlide[sandGrainsAlreadySlid + 1][0], hourglassTopRowLeft - sandGrainSlide[sandGrainsAlreadySlid + 1][1], false);
      mx.setPoint(sandGrainSlide[sandGrainsAlreadySlid][0], hourglassTopRowRight + sandGrainSlide[sandGrainsAlreadySlid][1], true);
      mx.setPoint(sandGrainSlide[sandGrainsAlreadySlid][0], hourglassTopRowLeft - sandGrainSlide[sandGrainsAlreadySlid][1], true);
      sandGrainsAlreadySlid--;
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

  if(currentTimestamp - sandTransitionTimer >= sandUpdateSpeedMillis) //is executed as often as sandUpdateSpeedMillis is set. 
  {
    sandTransitionTimer = currentTimestamp;
    
    updateFallingSand();
    updateSandGrainSlides();
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
