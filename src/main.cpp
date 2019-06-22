#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

#define LCD_ROWS 4
#define LCD_COLS 20

#define KEY_ROWS 5
#define KEY_COLS 4

#define PEDAL_PIN A7
#define BUZZER_PIN 13

typedef enum
{
  None,
  Running,
  Paused,
  Finished,
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

void watermark();
void readInputs();
void updateScreen();
void printScreen(char lines[LCD_ROWS][LCD_COLS + 1]);
void showMain();
void showLive();
void startWinding();
void keypadEvent(KeypadEvent key);

SoftwareSerial com(11, 12);

void setup()
{
  // put your setup code here, to run once:
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PEDAL_PIN, INPUT_PULLUP);

  digitalWrite(BUZZER_PIN, 0);

  Serial.begin(115200);
  com.begin(115200);

  delay(500);

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
}

char lastKey = '\0';

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
      {
        startWinding();
      } // Enter

      break;
    }

    case OperationState::Running:
    {

      break;
    }
    }
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
}

void showMain()
{
  char lines[LCD_ROWS][LCD_COLS + 1] = {
      {"SPIR:    610        "},
      {"TEL CAP: 03.20mm    "},
      {"F1: MOD: AUTO       "},
      {"          ENT: BASLA"}};

  printScreen(lines);
}

void showLive()
{
  char lines[LCD_ROWS][LCD_COLS + 1] = {
      {"TUR:  0 / 610       "},
      {"HIZ:  30 rpm        "},
      {"                    "},
      {"          ESC: IPTAL"}};

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

void startWinding()
{
  OpState = OperationState::Running;
  screenNeedsUpdate = true;
  //Offset, pedal ve motor devrede
}

// Taking care of some special events.
void keypadEvent(KeypadEvent key)
{
  switch (customKeypad.getState())
  {
    case PRESSED:
      Serial.print("Pressed: ");
      Serial.println(key);

      if (key == 'A')
        com.println("Left");
      else if (key == 'B')
        com.println("Right");

      break;
      
    case RELEASED:
      Serial.print("Released: ");
      Serial.println(key);

      com.println("Stop");
    break;
  }
}