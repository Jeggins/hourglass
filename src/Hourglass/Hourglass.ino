#include <MD_MAX72xx.h>
#include <SPI.h>

#define MAX_DEVICES 8  // 8 Panels
#define CLK_PIN 13     // Clock Pin
#define DATA_PIN 11    // Data Pin
#define CS_PIN 10      // Chip Select Pin
#define BTN1_PIN 2     // Button1 Pin
#define BTN2_PIN 3     // Button2 Pin

MD_MAX72XX mx = MD_MAX72XX(MD_MAX72XX::FC16_HW, CS_PIN, MAX_DEVICES);

//Variables hourglass general
const int hourglassTopRowRight = 0;
const int hourglassTopRowLeft = 63;
const int hourglassMidRow = 12;
const int hourglassBottomRow = 23;
const int hourglassSandGrains = 43;

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
const int hourglassTimerDefaultValue = 15; // Default value of the Hourglass Time. This time will be used when the Hourglass gets started out of standby.
const int sandUpdateSpeedMillis = 80; // Refreshtime in ms that will be used to visualize the falling sand.
const int secondMillis = 1000; //One second im ms.
const int milliseconds = 1; // Variable for one millisecond.
const int defaultStandbyTimer = 5;// Minutes until device turns to standby mode after the hourglass time has passed.
int standbyTimer = defaultStandbyTimer;
int hourglassTimer = hourglassTimerDefaultValue; // This is the actual Varable that will be used to set the hourglass timer. Can be changed via button.
int hourglassTimerSeconds = hourglassTimer * 60; // The time of the hourglass in seconds.
int sandTransitionMillis = hourglassTimerSeconds * 955L / hourglassSandGrains; // calculates the time in ms that one sandgrain need to move from the top part to the bottom part.
int secondsCounter = 0;  //Variable to count up the passed seconds.
int sandTransitionCounter = 0; //Variable to count up the sand that already fell down.
long sandTransitionTimer = 0; //Variable to count up the passed time of the sand transition.
long blinkTimer = 0; //Variable to count up the time for the blink animation in time change.
long secondsTimer = 0; //Timervariable for seconds.
long millisTimer = 0; //Timervariable for ms.

//Variables for numbers
const byte numbersLeft[11][8] = 
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

const byte numbersRight[11][8] = 
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
int topRow = 2;
int topColumn = 0;
int botRow = hourglassBottomRow;
int botColumn = 0;
int sandGrainSlide[3][2] = {{0, 0}, {0, 0}, {0, 0}};
int sandGrainsToSlide = -1;
int sandGrainsAlreadySlid = -1;
const int maxColumnPerRow[10] = {4, 5, 6, 6, 6, 4, 3, 2, 1, 0}; // bottom part max column per row

Sand sand[maxSandConst];

//Variabled for time change functionality
bool isTimeChangeActive = false;
bool numbersShown = true;
int timeChangeDuration = 8; //Duration how long the time change should be active
int blinkTimerMillis = 500;
int hourglassChangedTime = 15;
const int emptyValue = -1;


//Variables for buttons
int button1State = 0;
int button2State = 0;
int lastButton1State = HIGH;
int lastButton2State = HIGH;
unsigned long lastDebounceTime1 = 0;
unsigned long lastDebounceTime2 = 0;
const unsigned long debounceDelay = 150;

void setup() 
{
  Serial.begin(9600);  // Initialize serial interface
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 5); // Brightness
  mx.clear();
  pinMode(BTN1_PIN, INPUT_PULLUP);
  pinMode(BTN2_PIN, INPUT_PULLUP);

  for(int i = 0; i < maxSand; i++)
  {
    sand[i].row = hourglassMidRow;
    sand[i].active = false;
  }

  sand[nextSand].active = true;
  activateSandLed(sand[nextSand]);
  setInitialHourglass();
  setInitialNumbers(hourglassTimer);
}

//InitializeHourglass
void initializeHourglass(int hourglassStartTime)
{
  resetVariables(hourglassStartTime);

  for(int i = 0; i < maxSand; i++)
  {
    sand[i].row = hourglassMidRow;
    sand[i].active = false;
  }

  sand[nextSand].active = true;
  activateSandLed(sand[nextSand]);

  setInitialHourglass();
  setInitialNumbers(hourglassStartTime);
}

void resetVariables(int hourglassStartTime)
{
  standbyTimer = defaultStandbyTimer;
  hourglassTimer = hourglassStartTime;
  hourglassTimerSeconds = hourglassTimer * 60;
  sandTransitionMillis = hourglassTimerSeconds * 955L / hourglassSandGrains; 
  secondsCounter = 0;
  sandTransitionCounter = 0;
  sandTransitionTimer = 0;
  secondsTimer = 0;
  millisTimer = 0;
  topRow = 2;
  topColumn = 0;
  botRow = hourglassBottomRow;
  botColumn = 0;
  sandGrainsToSlide = -1;
  sandGrainsAlreadySlid = -1;
  maxSand = maxSandConst;
  sandBottom = hourglassBottomRow;
  nextSand = 0;
}

//Hourglass Code
void setInitialHourglass()
{
  byte startValue[4][8] = 
  {
    0x00,0x00,0x7f,0x7f,0x7f,0x3f,0x1f,0x0f, //Panel 1
    0x07,0x03,0x01,0x01,0x00,0x00,0x00,0x00, //Panel 2
    0x00,0x00,0x00,0x00,0x01,0x01,0x03,0x07, //Panel 7
    0x0f,0x1f,0x3f,0x7f,0x7f,0x7f,0x00,0x00  //Panel 8
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
        sandGrainSlide[1][0] = 2;
        sandGrainSlide[1][1] = botRow - 1;
        sandGrainSlide[2][0] = 1;
        sandGrainSlide[2][1] = botRow - 2;
        
        sandGrainsToSlide = 2;
        sandGrainsAlreadySlid = 2;
        sandGrainPlaced = true;
      }
      else
      {
        if(botColumn >= maxColumnPerRow[hourglassBottomRow - botRow])
        {
          botColumn = 0;
          botRow--;
          if(checkSandPile())
          {
            sandGrainPlaced = true;
          }
        }
        else
        {
          botColumn++;
        }
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
    sandBottom--;
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
    sandBottom--;
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
    sandBottom--;
    return true;
  }

  return false;
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

void setInitialNumbers(int hourglassStartTime)
{
  setTimeOnDisplay(hourglassStartTime);
}

void updateMinutes()
{
  if(hourglassTimer > 0)
  {
    hourglassTimer--;
  }
}

void setTimeOnDisplay(int value)
{
  int leftNumber = 0;
  int rightNumber = 0;
  if(value < 10 && value >= 0)
  {
    leftNumber = 10; //select the empty slot
    rightNumber = value;
  }
  else if(value >= 10)
  {
    leftNumber = value / 10;
    rightNumber = value % 10;
  }
  else
  {
    leftNumber = 10; //select the empty slot
    rightNumber = 10; //select the empty slot
  }
  
  for(int col = 0; col < 8; col++)
  {
    mx.setColumn(4, col, numbersLeft[leftNumber][col]);
    mx.setColumn(3, col, numbersRight[rightNumber][col]);
  }
}

void turnOnDevice()
{
  mx.control(MD_MAX72XX::SHUTDOWN, false);
}

void turnOffDevice()
{
  mx.control(MD_MAX72XX::SHUTDOWN, true);
}

void loop() 
{
  unsigned long currentTimestamp = millis();
  
  int reading1 = digitalRead(BTN1_PIN);
  int reading2 = digitalRead(BTN2_PIN);

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

  if(isTimeChangeActive)
  {
    if(currentTimestamp - blinkTimer >= blinkTimerMillis) // is executed as often as blinkTimerMillis is set.
    {
      blinkTimer = currentTimestamp;

      if(numbersShown)
      {
        setTimeOnDisplay(emptyValue);
        numbersShown = false;
      }
      else
      {
        setTimeOnDisplay(hourglassChangedTime);
        numbersShown = true;
      }
    }
  }
  
  if(currentTimestamp - secondsTimer >= secondMillis) //is executed every second
  {
    secondsTimer = currentTimestamp;
    secondsCounter++;

    if(hourglassTimerSeconds > 0)
    {
      hourglassTimerSeconds--;

      if(secondsCounter >= 60)
      {
        secondsCounter = 0;
        updateMinutes();
        setTimeOnDisplay(hourglassTimer);
      }
    }
    else
    {
      if(secondsCounter >= 60)
      {
        secondsCounter = 0;
        standbyTimer--;
      }
    }

    if(standbyTimer <= 0)
    {
      turnOffDevice();
    }

    if(isTimeChangeActive)
    {
      if(timeChangeDuration > 0)
      {
        timeChangeDuration--;
      }
      else
      {
        isTimeChangeActive = false;
        hourglassChangedTime = 15;
        setTimeOnDisplay(hourglassTimer);
      }
    }
  }

  if(reading1 != lastButton1State)
  {
    lastDebounceTime1 = currentTimestamp;
  }

  if(currentTimestamp - lastDebounceTime1 > debounceDelay)
  {
    if(reading1 != button1State)
    {
      button1State = reading1;
      if(button1State == LOW)
      {
        mx.clear();
        turnOnDevice();

        if(isTimeChangeActive)
        {
          initializeHourglass(hourglassChangedTime);
          hourglassChangedTime = 15;
        }
        else
        {
          initializeHourglass(hourglassTimerDefaultValue);
          hourglassChangedTime = 15;
        }

        isTimeChangeActive = false;
      }
    }
  }

  if(reading2 != lastButton2State)
  {
    lastDebounceTime2 = currentTimestamp;
  }

  if(currentTimestamp - lastDebounceTime2 > debounceDelay)
  {
    if(reading2 != button2State)
    {
      button2State = reading2;
      if(button2State == LOW)
      {
        timeChangeDuration = 8;
        standbyTimer = defaultStandbyTimer;
        turnOnDevice();

        if(!isTimeChangeActive)
        {
          isTimeChangeActive = true;
        }
        else
        {
          if(hourglassChangedTime < 20)
          {
            hourglassChangedTime++;
          }
          else
          {
            hourglassChangedTime = 5;
          }
        }
      }
    }
  }

  lastButton1State = reading1;
  lastButton2State = reading2;
}
