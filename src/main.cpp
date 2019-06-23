#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

#define LCD_ROWS 4
#define LCD_COLS 20

#define KEY_ROWS 5
#define KEY_COLS 4

#define PEDAL_PIN 12
#define BUZZER_PIN 13
#define SPEED_POT_PIN A0

typedef enum
{
  None,
  Offset,
  Running,
} OperationState;

OperationState OpState;

//define the cymbols on the buttons of the keypads
char hexaKeys[KEY_ROWS][KEY_COLS] = {
    {'G', 'H', '#', '*'},
    {'1', '2', '3', 'F'},
    {'4', '5', '6', 'E'},
    {'7', '8', '9', 'D'},
    {'A', '0', 'B', 'C'}};

byte colPins[KEY_COLS] = {2, 3, 4, 5};     //connect to the row pinouts of the keypad
byte rowPins[KEY_ROWS] = {10, 9, 8, 7, 6}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, KEY_ROWS, KEY_COLS);

LiquidCrystal_I2C lcd(0x3F, 20, 4);

/* region Variables */

char spirInput[5] = {'0', '0', '0', '0'};
char wireDiaInput[4] = {'0', '0', '0'};

uint8_t spirInputIndex = 0;
uint8_t wireDiaInputIndex = 0;
bool isInSecondRow = false;

float karkasFirstPos = -1.0f;
float karkasSecondPos = -1.0f;

uint16_t totalSpir = 0;
uint16_t currentSpir = 0;
uint16_t currentCycle = 0;
uint16_t speed = 0;
float wireDiameter = 0.0f;

/* endregion */

void watermark();
void readInputs();
void updateScreen();
void printScreen(char lines[LCD_ROWS][LCD_COLS + 1]);
void showMain();
void showLive();
void startWinding();
void keypadEvent(KeypadEvent key);
void playTone(uint8_t cycles, uint16_t tOn, uint16_t tOff);
void parseMessage(String *message);
void readComm();
void printInputValues();
void startOffset();
void showOffset();
void updateOffsetParameters();
uint16_t readSpeed();
void updateRunningPartial();
void continueWork();
void pauseWork();

SoftwareSerial com(A3, 11);

void setup()
{
  // put your setup code here, to run once:
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PEDAL_PIN, INPUT_PULLUP);
  pinMode(SPEED_POT_PIN, INPUT);

  digitalWrite(BUZZER_PIN, LOW);

  Serial.begin(115200);
  com.begin(115200);

  while (!com)
  {
    ;
  }

  lcd.init();
  lcd.backlight();
  watermark();

  customKeypad.addEventListener(keypadEvent); // Add an event listener for this keypad
}

void watermark()
{
  char line[4][LCD_COLS + 1] =
      {
          {"        KARA        "},
          {"      ELEKTRONIK    "},
          {"    SES SISTEMLERI  "},
          {"      v20/06/19     "}};

  lcd.clear();

  for (uint8_t i = 0; i < LCD_ROWS; i++)
  {
    lcd.setCursor(0, i);

    for (uint8_t j = 0; j < LCD_COLS; j++)
    {
      lcd.print(line[i][j]);
      delay(30);
    }
  }

  delay(1000);
}

void loop()
{
  readInputs();
  updateScreen();
  readComm();
}

String inputMessage = "";

void readComm()
{
  if (com.available())
  {
    char c = com.read();

    if (c != '\n')
    {
      inputMessage += c;
    }
    else
      parseMessage(&inputMessage);
  }
}

void updateOffsetParameters()
{
  lcd.setCursor(0, 0);

  if (karkasSecondPos >= 0)
  {
    char line[21];
    lcd.print("                    ");
    String rep = String(karkasSecondPos);
    sprintf(line, "GENISLIK: %smm", rep.c_str());

    lcd.setCursor(0, 0);
    lcd.print(line);
  }
}

void parseMessage(String *message)
{
  if (message->startsWith("OFD: "))
  {
    playTone(1, 200, 100);

    message->replace("OFD: ", "");

    float firstPos = message->toFloat();

    karkasFirstPos = firstPos;
    updateOffsetParameters();
  }
  else if (message->startsWith("OSD: "))
  {
    playTone(2, 150, 50);

    message->replace("OSD: ", "");

    float secondPos = message->toFloat();
    karkasSecondPos = secondPos;
    updateOffsetParameters();
  }

  *message = "";
}

char lastKey = '\0';
int8_t oldPedalState = -1;

void readInputs()
{
  char key = customKeypad.getKey();

  if (key)
  {
    switch (OpState)
    {
    case OperationState::None:
    {
      if (key == 'C')
      { // Enter
        startOffset();
      }

      else if (key == 'E')
      { //Aşağı
        isInSecondRow = true;
        printInputValues();
      }
      else if (key == 'F')
      { //Yukarı
        isInSecondRow = false;
        printInputValues();
      }
      else if (isDigit(key))
      {
        if (!isInSecondRow)
        {
          uint8_t pos = strlen(spirInput);

          if (spirInputIndex == pos)
            spirInputIndex--;

          if (spirInputIndex < pos)
          {
            spirInput[spirInputIndex] = key;
            spirInputIndex++;
          }
        }
        else if (isInSecondRow)
        {
          uint8_t pos = strlen(wireDiaInput);

          if (wireDiaInputIndex == pos)
            wireDiaInputIndex--;

          if (wireDiaInputIndex < pos)
          {
            wireDiaInput[wireDiaInputIndex] = key;
            wireDiaInputIndex++;
          }
        }

        printInputValues();
      }
      else if (key == 'D')
      {
        if (!isInSecondRow)
        {
          uint8_t len = strlen(spirInput);

          if (spirInputIndex >= len)
            spirInputIndex--;

          spirInput[spirInputIndex] = '0';

          if (spirInputIndex > 0)
            spirInputIndex--;
        }
        else if (isInSecondRow)
        {
          uint8_t len = strlen(wireDiaInput);

          if (wireDiaInputIndex >= len)
            wireDiaInputIndex--;

          wireDiaInput[wireDiaInputIndex] = '0';

          if (wireDiaInputIndex > 0)
            wireDiaInputIndex--;
        }

        printInputValues();
      }

      break;
    }

    case OperationState::Offset:
    {
      if (key == 'C') // Enter
        startWinding();

      else if (key == 'G') // F1
        com.println("Offset-First");
      else if (key == 'H') // F2
        com.println("Offset-Second");

      break;
    }
    }
  }

  if (OpState == OperationState::Running)
  {
    uint8_t pedalState = digitalRead(PEDAL_PIN);

    if (oldPedalState != pedalState && pedalState == LOW)
    {
      Serial.println("Pedal: Low");
      oldPedalState = pedalState;

      continueWork();
    }
    else if (oldPedalState != pedalState && pedalState == HIGH)
    {
      Serial.println("Pedal: High");
      oldPedalState = pedalState;

      pauseWork();
    }
  }
}

void continueWork()
{
  String message = "Work: ";
  message += currentCycle;
  message += "|";
  message += wireDiameter;
  message += "|";
  message += speed;

  com.println(message);
  Serial.println(message);
}

void pauseWork()
{
  com.println("Pause");
  Serial.println("Pause");
}

void printInputValues()
{
  lcd.blink();

  if (isInSecondRow == false)
  {
    lcd.setCursor(9, 0);
    lcd.print(spirInput);

    uint8_t len = strlen(spirInput);
    lcd.setCursor(9 + (spirInputIndex < len ? spirInputIndex : len - 1), 0);
  }
  else
  {
    lcd.setCursor(9, 1);

    char line[5];
    sprintf(line, "%c.%c%c", wireDiaInput[0], wireDiaInput[1], wireDiaInput[2]);
    lcd.print(line);

    uint8_t len = strlen(wireDiaInput);

    uint8_t cursorPos = wireDiaInputIndex;

    if (wireDiaInputIndex > 0)
      cursorPos++;

    if (wireDiaInputIndex >= len)
      cursorPos--;

    lcd.setCursor(9 + cursorPos, 1);
  }
}

bool screenNeedsUpdate = true;

void updateScreen()
{
  if (screenNeedsUpdate)
  {
    switch (OpState)
    {
    case OperationState::None:
    {
      showMain();
      break;
    }

    case OperationState::Offset:
    {
      showOffset();
      break;
    }

    case OperationState::Running:
    {
      showLive();
      break;
    }
    default:
      break;
    }

    screenNeedsUpdate = false;
  }

  if (OpState == OperationState::Running)
  {
    updateRunningPartial();
  }
}

unsigned long lastUpdatedAt = 0;
uint16_t oldCurrentSpir, oldTotalSpir, oldSpeed = 0;

void updateRunningPartial()
{
  if (millis() - lastUpdatedAt > 30)
  {
    lastUpdatedAt = millis();

    if (currentSpir != oldCurrentSpir || oldTotalSpir != totalSpir)
    {
      lcd.setCursor(6, 1);
      lcd.print("              ");
      lcd.setCursor(6, 1);

      char rest[15];
      sprintf(rest, "%d / %d", currentSpir, totalSpir);
      lcd.print(rest);

      oldCurrentSpir = currentSpir;
      oldTotalSpir = totalSpir;
    }

    speed = readSpeed();

    if (oldSpeed != speed)
    {
      lcd.setCursor(6, 2);
      lcd.print("              ");
      lcd.setCursor(6, 2);

      char rest[15];
      sprintf(rest, "%d", speed);

      lcd.print(rest);
      oldSpeed = speed;
    }
  }
}

void showMain()
{
  char lines[LCD_ROWS][LCD_COLS + 1] = {
      {"SPIR:    0000       "},
      {"TEL CAP: 0.00mm     "},
      {"#: MOD:  MANUEL     "},
      {"          ENT: BASLA"}};

  printScreen(lines);

  printInputValues();
}

void showLive()
{
  char lines[LCD_ROWS][LCD_COLS + 1] = {
      {"    ----HAZIR----   "},
      {"TUR:  0 / 0       "},
      {"HIZ:  0 rpm        "},
      {"          ESC: IPTAL"}};

  printScreen(lines);
}

void showOffset()
{
  char lines[LCD_ROWS][LCD_COLS + 1] = {
      {"GENISLIK: -         "},
      {"                    "},
      {"                    "},
      {"          ENT: BASLA"}};

  printScreen(lines);
}

void printScreen(char lines[LCD_ROWS][LCD_COLS + 1])
{
  lcd.clear();
  lcd.setCursor(0, 0);

  for (uint8_t i = 0; i < LCD_ROWS; i++)
  {
    lcd.setCursor(0, i);

    for (uint8_t j = 0; j < LCD_COLS; j++)
    {
      lcd.print(lines[i][j]);
    }
  }
}

void startOffset()
{
  lcd.noBlink();

  totalSpir = (uint16_t)atol(spirInput);
  currentSpir = 0;

  wireDiameter = ((float)atol(wireDiaInput) / 100.0f);
  speed = readSpeed();

  OpState = OperationState::Offset;
  screenNeedsUpdate = true;

  //Offset, pedal ve motor devrede
}
void startWinding()
{
  lcd.noBlink();
  OpState = OperationState::Running;
  screenNeedsUpdate = true;
}

void playTone(uint8_t cycles, uint16_t tOn, uint16_t tOff)
{
  for (uint8_t i = 0; i < cycles; i++)
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(tOn);
    digitalWrite(BUZZER_PIN, LOW);
    delay(tOff);
  }
}

// Taking care of some special events.
void keypadEvent(KeypadEvent key)
{
  switch (customKeypad.getState())
  {
  case PRESSED:
    if (OpState == OperationState::Offset)
    {
      if (key == 'A')
      {
        com.println("Left");
        Serial.println("Left");
      }
      else if (key == 'B')
      {
        com.println("Right");
        Serial.println("Right");
      }
      /*else if (key == '5')
        com.println("Work: 2.30|60|300");*/
    }

    break;

  case RELEASED:
    if (OpState == OperationState::Offset)
    {
      if (key == 'A' || key == 'B')
      {
        Serial.println("Stop");
        com.println("Stop");
      }
    }

    break;
  }
}

const uint8_t numReadings = 10;

uint16_t readings[numReadings]; // the readings from the analog input
uint16_t readIndex = 0;         // the index of the current reading
uint16_t total = 0;             // the running total
uint16_t average = 0;           // the average

uint16_t readSpeed()
{
  uint8_t value = analogRead(SPEED_POT_PIN);

  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = value;
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex++;

  // if we're at the end of the array...
  if (readIndex >= numReadings)
  {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
  average = total / numReadings;

  uint16_t result = (uint16_t)(map(average, 0, 255, 1, 400));

  return result;
}