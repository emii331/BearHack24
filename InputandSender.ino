#include "LedControl.h"
#include "LiquidCrystal.h"
#include <RTClib.h>
#include <Keypad.h>
#include <Wire.h>

#define receiverAddress 0x08

//Real time clock module set up
RTC_DS3231 rtc;
//End RTC step up

//Start input keypad membrane set up
const byte ROWS = 4;  //four rows
const byte COLS = 3;  //three columns

char keys[ROWS][COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};
byte rowPins[ROWS] = { 9, 8, 7, 6 };
byte colPins[COLS] = { 5, 4, 3 };

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
//End keypad setup

//Start LCD output setup
int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
//End LCD setup

//Start LED output setup
LedControl lc = LedControl(12, 11, 10, 1);
//End LED setup

//Start various pin initialization
int motionSensor = 1;
int buzzerPin = 13;
//End pin initialization

//Start general global variables/flags
char keypadInput = '!';
int menuDisplayFlag = 1;
int menuSelect = 1;
int inputSleepHours = 0;
//End general global variables/flags

//Start Database
int data[7][24] = { { 17, 18, 15, 14, 19, 22, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
                    { 11, 9, 12, 9, 10, 11, 10, 11, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
                    { 11, 13, 14, 14, 14, 13, 15, 15, 16, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
                    { 11, 10, 9, 10, 12, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
                    { 16, 16, 15, 14, 14, 15, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
                    { 16, 17, 17, 17, 17, 16, 17, 17, 17, 17, 18, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
                    { 17, 18, 15, 14, 19, 22, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 } };
int dataSize = 0;
//End Database

void printDataSet() {
  for (int i = 0; i < 7; i++) {
    for (int j = 0; j < 24; j++) {
      Serial.print(data[i][j]);
      Serial.print(" ");
    }
    Serial.print("\n");
  }
}

void LCDWriteLines(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void transmitLCD(String line1, String line2) {
  Wire.beginTransmission(receiverAddress);
  Wire.write('1');
  for (int i = 0; i < line1.length(); i++) {
    Wire.write(line1.charAt(i));
  }
  Wire.write('/');
  for (int i = 0; i < line2.length(); i++) {
    Wire.write(line2.charAt(i));
  }
  Wire.endTransmission();
}

void transmitLED(int x, int y) {
  Wire.beginTransmission(receiverAddress);
  Wire.write('2');
  Wire.write(x);
  Wire.write(y);
  Wire.endTransmission();
}

void clearLED() {
  Wire.beginTransmission(receiverAddress);
  Wire.write('3');
  Wire.endTransmission();
}

// String receiver[2];
// void receiveEvent(int bytes) {
//   for(int i = 0; i < bytes; i++) {
//     receiver[i] = Wire.read();
//   }
// }

void outputMainMenu() {
  String line1 = "Error line 1";
  String line2 = "Error line 2";
  switch (menuSelect) {
    case 1:
      line1 = "1. Start Sleep";
      line2 = "Enter:*  Next:#";
      break;
    case 2:
      line1 = "2. Analyze Sleep";
      line2 = "Enter:*  Next:#";
      break;
    case 3:
      line1 = "3. Week Summary";
      line2 = "Enter:*  Next:#";
      break;
    case 4:
      line1 = "4. Sleep Tips";
      line2 = "Enter:*  Next:#";
      break;
    default:
      line1 = "Error line 1";
      line2 = "Error line 2";
  }

  transmitLCD(line1, line2);
}

void getMainMenuInput() {
  keypadInput = keypad.getKey();
  if (keypadInput) {
    if (keypadInput == '*') {
      menuDisplayFlag = 0;
    } else if (keypadInput == '#') {
      if (menuSelect == 4) {
        menuSelect = 1;
      } else {
        menuSelect++;
      }
    }
  }
  keypadInput = '!';
}

enum O1_States { O1_Init,
                 O1_Select,
                 O1_Wait,
                 O1_Update } O1_State;
int wait1Counter = 0;
bool motionSensed = false;
int secSinceStart = 0;
int timesMovedinHours[24];
int choice = 0;
void option1_Tick() {
  if (choice == 0) {
    //Serial.println("init");
    inputSleepHours = 0;
    wait1Counter = 0;
    motionSensed = false;
    secSinceStart = 0;
    for (int i = 0; i < 24; i++) {
      timesMovedinHours[i] = -1;
    }

    choice = 1;
  } else if (choice == 1) {
    //Serial.println("select");
    String line1 = "Enter the number";
    String line2 = "of hours (1-9):";
    transmitLCD(line1, line2);

    keypadInput = keypad.getKey();
    if (keypadInput) {
      //Serial.println(keypadInput);
      if (keypadInput != '!' && keypadInput != '*' && keypadInput != '#') {
        inputSleepHours = keypadInput - 48;
        //Serial.println("Yay");
        keypadInput = '!';
        choice = 2;
      } else {
        choice = 1;
      }
    } else {
      choice = 1;
    }
  } else if (choice == 2) {
    //Serial.println("wait");
    if (digitalRead(motionSensor) == HIGH) {
      motionSensed = true;
    }
    wait1Counter++;
    String line3 = "Now sleeping";
    String line4 = "Sweet dreams";
    transmitLCD(line3, line4);

    if (wait1Counter > 4) {
      choice = 3;
      wait1Counter = 0;
    } else {
      choice = 2;
    }
  } else if (choice == 3) {
    //Serial.println("update");
    secSinceStart++;
    int numHours = (secSinceStart / 60) / 60;
    if (motionSensed) {
      timesMovedinHours[numHours] += 1;
    }
    //output to "clock"

    keypadInput = keypad.getKey();
    if (keypadInput) {
      if (keypadInput == '*') {
        DateTime time = rtc.now();
        int dayOfWeek = time.dayOfTheWeek();
        int counter = 0;
        while (timesMovedinHours[counter] != -1) {
          data[dayOfWeek - 1][counter] = timesMovedinHours[counter];
          counter++;
        }
        dataSize++;
        menuDisplayFlag = 1;
        choice = 0;
        keypadInput = '!';
        noTone(buzzerPin);
        printDataSet();
      }
    } 
    else if(numHours >= inputSleepHours) {
      tone(buzzerPin, 1047, 9999999999);
    }
    else {
      choice = 2;
    }
  } else {
    //Serial.println("default");
    choice = 0;
  }
}


enum O2_States { O2_Init,
                 O2_Display } O2_State;
void option2_Tick() {
  switch (O2_State) {
    case O2_Init:
      O2_State = O2_Display;
      break;
    case O2_Display:
      keypadInput = keypad.getKey();
      if (keypadInput) {
        if (keypadInput == '#') {
          menuDisplayFlag = 1;
          //lc.clearDisplay(0);
          clearLED();
        }
        keypadInput = '!';
        O2_State = O2_Init;
      }
      break;
    default:
      O2_State = O2_Init;
      break;
  }

  switch (O2_State) {
    case O2_Init:
      clearLED();
      break;
    case O2_Display:
      DateTime time = rtc.now();
      int currentDay = time.dayOfTheWeek();
      int hoursOfSleep = 0;
      int sumOfMovement = 0;
      while (data[currentDay][hoursOfSleep] > 0) {

        double tempColumn = data[currentDay][hoursOfSleep] / 26.0;
        int column = tempColumn * 8;

        for (int i = 0; i < column; i++) {
          //lc.setLed(0, hoursOfSleep, i, 1);
          transmitLED(hoursOfSleep, i);
        }

        sumOfMovement += data[currentDay][hoursOfSleep];
        hoursOfSleep++;
      }
      double avgOfMovement = static_cast<double>(sumOfMovement) / static_cast<double>(hoursOfSleep);
      double rating = 0.65 * (hoursOfSleep / 10.0) + ((26 - avgOfMovement) / 26) * 0.35;
      String sleepScore = "0: Error";
      if (rating < 0.2) {
        sleepScore = "1: Danger Sleep";
      } else if (rating < 0.4) {
        sleepScore = "2: Bad Sleep";
      } else if (rating < 0.6) {
        sleepScore = "3: Okay Sleep";
      } else if (rating < 0.8) {
        sleepScore = "4: Good Sleep";
      } else {
        sleepScore = "5: Amazing Sleep";
      }
      String hoursSlept = "Hours Slept: " + static_cast<String>(hoursOfSleep);
      //LCDWriteLines(sleepScore, hoursSlept);
      transmitLCD(sleepScore, hoursSlept);
      break;
  }
}

enum O3_States { O3_Init,
                 O3_Display } O3_State;
void option3_Tick() {
  switch (O3_State) {
    case O3_Init:
      O3_State = O3_Display;
      break;
    case O3_Display:
      keypadInput = keypad.getKey();
      if (keypadInput) {
        if (keypadInput == '#') {
          menuDisplayFlag = 1;
          //lc.clearDisplay(0);
          clearLED();
        }
        keypadInput = '!';
        O3_State = O3_Init;
      }
      break;
    default:
      O3_State = O3_Init;
      break;
  }

  switch (O3_State) {
    case O3_Init:
      clearLED();
      break;
    case O3_Display:
      int hoursOfSleepPerDay[7];
      for (int i = 0; i < 7; i++) {
        int hoursOfSleep = 0;
        int sumOfMovement = 0;
        while (data[i][hoursOfSleep] > 0) {
          sumOfMovement += data[i][hoursOfSleep];
          hoursOfSleep++;
        }
        double avgOfMovement = static_cast<double>(sumOfMovement) / static_cast<double>(hoursOfSleep);
        double rating = 0.65 * (hoursOfSleep / 10.0) + ((26 - avgOfMovement) / 26) * 0.35;
        int totalScore = rating * 5;
        for (int j = 0; j < totalScore; j++) {
          //lc.setLed(0, i, j, 1);
          transmitLED(i, j);
        }
        hoursOfSleepPerDay[i] = hoursOfSleep;
      }
      String line1 = "M:" + static_cast<String>(hoursOfSleepPerDay[0]) + " T:" + static_cast<String>(hoursOfSleepPerDay[1]) + " W:" + static_cast<String>(hoursOfSleepPerDay[2]) + " R:" + static_cast<String>(hoursOfSleepPerDay[3]);
      String line2 = "F:" + static_cast<String>(hoursOfSleepPerDay[4]) + " S:" + static_cast<String>(hoursOfSleepPerDay[5]) + " U:" + static_cast<String>(hoursOfSleepPerDay[6]);
      transmitLCD(line1, line2);
      break;
  }
}

int tipMenuSlider = 1;
void option4_Tick() {
  keypadInput = keypad.getKey();
  if (keypadInput) {
    if (keypadInput != '*' && keypadInput != '#' && keypadInput != '0') {
      tipMenuSlider = keypadInput - 48;
    } else if (keypadInput == '#') {
      menuDisplayFlag = 1;
    }
  }
  keypadInput = '!';

  String line1 = "Error line 1";
  String line2 = "Error line 2";
  switch (tipMenuSlider) {
    case 1:
      line1 = "Keep consistent";
      line2 = "sleep routine";
      break;
    case 2:
      line1 = "Turn off";
      line2 = "electronics";
      break;
    case 3:
      line1 = "Exercise during";
      line2 = "the day";
      break;
    case 4:
      line1 = "Avoid large";
      line2 = "meals before bed";
      break;
    case 5:
      line1 = "Avoid caffeine";
      line2 = "and alcohol";
      break;
    case 6:
      line1 = "Read a good";
      line2 = "book before bed";
      break;
    case 7:
      line1 = "Have a dark,";
      line2 = "quiet room";
      break;
    case 8:
      line1 = "Get comfy";
      line2 = ":)";
      break;
    case 9:
      line1 = "Get a comfy";
      line2 = "plushie";
      break;
    default:
      line1 = "Error line 1";
      line2 = "Error line 2";
  }

  transmitLCD(line1, line2);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Wire.begin();

  pinMode(motionSensor, INPUT);
  pinMode(buzzerPin, OUTPUT);

  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);

  rtc.begin();
  rtc.adjust(DateTime(2024, 4, 28, 12, 0, 0));

  bool demoMode = true;

  if (!demoMode) {
    for (int i = 0; i < 7; i++) {
      for (int j = 0; j < 24; j++) {
        data[i][j] = -1;
      }
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:

  //Serial.println(menuDisplayFlag);
  if (menuDisplayFlag) {
    outputMainMenu();
    getMainMenuInput();
  } else {
    if (menuSelect == 1) {
      option1_Tick();
    } else if (menuSelect == 2) {
      option2_Tick();
    } else if (menuSelect == 3) {
      option3_Tick();
    } else if (menuSelect == 4) {
      option4_Tick();
    } else {
      Serial.println("Menu Error: " + (String)menuSelect);
    }
  }
  delay(200);
}
