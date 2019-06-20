#include <Arduino.h>
#include <Wire.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

#define LCD_ROWS 4
#define LCD_COLS 20

#define KEY_ROWS 5
#define KEY_COLS 4

#define PEDAL_PIN A7
#define BUZZER_PIN 13

//define the cymbols on the buttons of the keypads
char hexaKeys[KEY_ROWS][KEY_COLS] = {
  {'G', 'H', '#', '*'},
  {'1', '2', '3', '˄'},
  {'4', '5', '6', '˅'},
  {'7', '8', '9', 'E'},
  {'<', '0', '>', 'R'}
};

byte rowPins[KEY_ROWS] = {6, 7, 8, 9, 10}; //connect to the row pinouts of the keypad
byte colPins[KEY_COLS] = {2, 3, 4, 5}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, KEY_ROWS, KEY_COLS); 

LiquidCrystal_I2C lcd(0x3F, 20, 4);

void watermark();
void readInputs();

void setup() {
  // put your setup code here, to run once:
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PEDAL_PIN, INPUT_PULLUP);

  digitalWrite(BUZZER_PIN, 0);

  Serial.begin(115200);
  delay(500);

  lcd.init();
  lcd.backlight();
  watermark();
}

void watermark() {
  char line[4][LCD_COLS + 1] =
  {
    {"        KARA        "},
    {"      ELEKTRONIK    "},
    {"    SES SISTEMLERI  "},
    {"      v20/06/19     "}
  };

  lcd.clear();

  for(uint8_t i = 0; i < LCD_ROWS; i++) {
    lcd.setCursor(0, i);

    for(uint8_t j = 0; j < LCD_COLS; j++) {
      lcd.print(line[i][j]);
      delay(30);
    }
  }

  delay(1000);
}

void loop() {
  readInputs();
  updateScreen();
}

void readInputs() {
  char key = customKeypad.getKey();

  if (key) {
    Serial.print(F("Pressed: "));
    Serial.println(key);
  }
}

void updateScreen() {

}