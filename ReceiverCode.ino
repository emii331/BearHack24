#include<Wire.h>
#include <LiquidCrystal.h>
#include <LedControl.h>

#define receiverAddress 0x08

int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int DIN = 10;
int CS = 9;
int CLK = 8;

LedControl lc=LedControl(DIN, CLK, CS,0);

void setup() {
  // put your setup code here, to run once:
  Wire.begin(receiverAddress);
  Wire.onReceive(receiveEvent);

  lcd.begin(16,2);

  lc.shutdown(0,false);
  lc.setIntensity(0,0);
  lc.clearDisplay(0);

}

String receiver[2];

void LCDWriteLines(String line1, String line2) {
  lcd.clear();          
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
  lcd.setCursor(15,1);
}

void receiveEvent(int bytes) { 
  int x;
  char a = '0';
  a -= 48;
  int breakpoint;

  x = Wire.read();
  a += x;

  if (a == '1') {
    a -= x;
    String readIn1 = "";
        
        for(int i = 0; i < bytes; i++) {
          x = Wire.read();
          a += x;
          if (a == '/') {
            a -= x;
            breakpoint = i + 1;
            break;
          }
          readIn1 += a;
          a -=x;
        }
        receiver[0] = readIn1;

        String readIn2 = "";
        for(int i = breakpoint; i < bytes; i++) {
          x = Wire.read();
          a += x;
          readIn2 += a;
          a -= x;
        }

        receiver[1] = readIn2;

        LCDWriteLines(receiver[0], receiver[1]);
  }
  else if (a == '2'){
    a -= x;
    int readIn1 = 0;
    int readIn2 = 0;

    x = Wire.read();
    readIn1 = x;
    x = Wire.read();
    readIn2 = x;

    lc.setLed(0, readIn1, readIn2, 1);
  }
  else if (a == '3') {
    a -= x;
    lc.clearDisplay(0);
  }
 
}

void loop() {
  Wire.onReceive(receiveEvent);
  delay(200);
}