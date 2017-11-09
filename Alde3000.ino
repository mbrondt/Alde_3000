#include <dht.h>

//dht DHT11;
dht DHT22;

#define PIN6 6
#define PIN7 7

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

Servo myServo;

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

static byte termo[8] = {
  B00100, B01010, B01010, B01010, B01010, B10101, B10001, B01110};
static byte grad[8] = {
  B11100, B10100, B11100, B00000, B00000, B00000, B00000,};
static byte oe[8] = {
  B00000, B00000, B01110, B10011, B10101, B11001, B01110, B00000};
static byte g[8] = {
  B00000, B00000, B01111, B10001, B10001, B01111, B00001, B01110};
static byte hPunktum[8] = {
  B00000, B00000, B00000, B00000, B00000, B00011, B00011, B00000};
static byte pil[8] = {
  B00000,B00100,B00010,B11111,B00010,B00100,B00000,B00000};

boolean prik = false;
boolean manualControl = false;

static int steps[9] = {
  30,44,62,73,83,93,103,116,130};
//static float trin[8] = {1.0,3.0,5.0,7.0,9.0,11.0,13.0,15.0};

// max temp. for hvert trin 
static float trin1 = 1.0;
static float trin2 = 3.0;
static float trin3 = 5.0;
static float trin4 = 7.0;
static float trin5 = 9.0;
static float trin6 = 11.0;
static float trin7 = 13.0;
static float trin8 = 15.0;
//bumpstop forhindrer servo i at trække min for langt ned
//static int bumpStop = 15;
//static int pumpeFaktor = 30;
static int initPause = 10;
static int pause = initPause;

void setup() {
  Serial.begin(9600);
  lcd.begin(20, 4);
  lcd.backlight();
  lcd.createChar(0, termo);
  lcd.write(byte(0));
  lcd.createChar(1, grad);
  lcd.write(byte(1));
  lcd.createChar(2, oe);
  lcd.write(byte(2));
  lcd.createChar(3, g);
  lcd.write(byte(3));
  lcd.createChar(4, hPunktum);
  lcd.write(byte(4));
  lcd.createChar(5, pil);
  lcd.write(byte(5));

  while (! Serial);
  Serial.println("");
  Serial.println("Arduino Pump Control - RESTARTED");
  Serial.println("Enter position to set Servo: 1 to 9");

  myServo.attach(9);
  //myServo.write(steps[1]); // = trin 7
  justerPumpe(7);

    skrivAutLedetekster();
  lcd.setCursor(13, 3);
  lcd.print(getAktPumpeTrin());

  delay(1000);

}

//******************************** START LOOP ********************************************
void loop() {
  float frem = 0.0;
  float retur = 0.0;
  float diff = 0.0;
  int position = 0;

  heartBeat();

  if (Serial.available()) {
    position = displayManualControl(position);
  } 
  if (!manualControl) {

    //Skriv pausetæller forrest i hver linie
    Serial.println("");
    Serial.print(pause);
    Serial.print(" ");

    // Fremløb
    delay(500);
    int pin6Status = callDHT22(PIN6);
    delay(1000);
    frem = DHT22.temperature;

    Serial.print("Frem ");
    Serial.print(frem, 1);
    if (pin6Status == 0 && isTempValid(frem) && !manualControl) {
      lcd.setCursor(0, 1);
      lcd.print("       ");
      lcd.setCursor(1, 1);
      lcd.print(frem, 1);
      lcd.write(byte(1));
    }

    // Returløb
    int pin7Status = callDHT22(PIN7);
    delay(1000);
    retur = DHT22.temperature;
    Serial.print("\tretur ");
    Serial.print(retur, 1);
    if (pin7Status == 0 && isTempValid(retur) && !manualControl) {
      lcd.setCursor(8, 1);
      lcd.print("       ");
      lcd.setCursor(8, 1);
      lcd.print(retur, 1);
      lcd.write(byte(1));
    }

    //Diff
    //  if (isTempValid(frem) && isTempValid(retur) && !manualControl) {
    diff = (frem - retur);
    Serial.print("\tdiff ");
    Serial.print(diff, 1);
    if (manualControl) {
      Serial.println("");
    } 
    if (pin6Status == 0 && pin7Status == 0 && !manualControl) {
      lcd.setCursor(15, 1);
      lcd.print("     ");
      lcd.setCursor(15, 1);
      lcd.print(diff, 1);
      lcd.write(byte(1));
    }

    // Skal pumpen justeres (spring over ved fejllæsning
    if (pin6Status == 0 && pin7Status == 0 && !manualControl) {
      position = findTempDiff(frem, retur);
      Serial.print(" Beregnet trin ");
      Serial.print(position);
      Serial.print("  --> aktuelt trin ");
      Serial.print(getAktPumpeTrin());
      lcd.setCursor(18, 3);
      lcd.print(position);
      pause--;
      if (pause < 1 && position > 0) {
        justerPumpe(position);
        lcd.setCursor(13, 3);
        lcd.print(getAktPumpeTrin());

        pause = initPause;
      }
      pausePrikker(pause);
    }


    // Ro på loop
  }
  delay(1000);
}

void justerPumpe(int pos) {
  if (pos >= 1 && pos <= 9) {
    int nuvPos = myServo.read();
    int nyPos = steps[pos-1];
    //    int nyPos = ((pos - 1) * pumpeFaktor) + bumpStop;
    //   Serial.println(nyPos);
    if (nyPos > nuvPos) {
      for (int idx = nuvPos; idx <= nyPos; idx++) {
        myServo.write(idx);
        delay(50);
      }
    } 
    else {
      for (int idx = nuvPos; idx >= nyPos; idx--) {
        myServo.write(idx);
        delay(50);
      }
    }

    Serial.print("\nServo sat til ");
    Serial.print(myServo.read());
    Serial.print(" gr.");
    Serial.print(" - trin ");
    Serial.print(pos);
    Serial.print("\tTrin temps: ");
    Serial.print(trin1,1);
    Serial.print(", ");
    Serial.print(trin2,1);
    Serial.print(", ");
    Serial.print(trin3,1);
    Serial.print(", ");
    Serial.print(trin4,1);
    Serial.print(", ");
    Serial.print(trin5,1);
    Serial.print(", ");
    Serial.print(trin6,1);
    Serial.print(", ");
    Serial.print(trin7,1);
    Serial.print(", ");
    Serial.print(trin8,1);
    Serial.println("");
  }
}

char manualOverwrite() {
  char ch = Serial.read();
  Serial.write(ch);
  if (ch >= '1' && ch <= '9')
  {
    Serial.print("Servo saettes til ");
    return ch;
  }
  if (ch == 'x')
  {
    Serial.println("\nManual overwrite exited");
    return ch;
  }
}

int findTempDiff(float frem, float retur) {
  float diff = (frem - retur);
  if (diff <= trin1) {
    return 1;
  }
  if (diff <= trin2)  {
    return 2;
  }
  if (diff <= trin3)  {
    return 3;
  }
  if (diff <= trin4)  {
    return 4;
  }
  if (diff <= trin5)  {
    return 5;
  }
  if (diff <= trin6)  {
    return 6;
  }
  if (diff <= trin7)  {
    return 7;
  }
  if (diff <= trin8)  {
    return 8;
  }
  if (diff > trin8) {
    return 9;
  }
  //  Serial.println("");
  //  Serial.print("Diff: ");
  //  Serial.print(diff);
  return -1;
}

void skrivAutLedetekster() {
  lcd.clear();
  //  lcd.setCursor(1, 0);
  //  lcd.print("Alde Compact 3000");

  lcd.setCursor(1, 0);
  lcd.print("Frem");

  lcd.setCursor(8, 0);
  lcd.print("Retur");

  lcd.setCursor(15, 0);
  lcd.print("Diff");

  lcd.setCursor(1, 2);
  lcd.print("Pumpetrin: akt");
  lcd.print(" -");
  lcd.write(byte(5));
  lcd.print("*");

  //  lcd.setCursor(12, 2);
  //  lcd.print("Akt.trin");
}

int callDHT22(int pin) {
  int chk = DHT22.read22(pin);
  if (chk != DHTLIB_OK) {
    Serial.println("");
    if (pin == 6) {
      Serial.print("Frem: ");
    } 
    else {
      Serial.print("Retur: ");
    }      
  }
  switch (chk)
  {
  case DHTLIB_OK:
    //   Serial.print(" OK,\t");
    return chk;
  case DHTLIB_ERROR_CHECKSUM:
    Serial.print(" Checksum error,\t");
    return chk;
  case DHTLIB_ERROR_TIMEOUT:
    Serial.print(" Time out error,\t");
    return chk;
  default:
    Serial.print(" Unknown error,\t");
    return chk;
  }
}

void heartBeat() {
  //   Serial.print(prik);
  if (prik == false) {
    lcd.setCursor(0, 0);
    lcd.print(".");
    lcd.setCursor(19, 0);
    lcd.print(" ");
    prik = true;
  } 
  else {
    lcd.setCursor(19, 0);
    lcd.write(byte(4));
    lcd.setCursor(0, 0);
    lcd.print(" ");
    prik = false;
  }
}

boolean isTempValid(float temp) {
  if (temp >= -20.00 && temp <= 99.00) {
    return true;
  }
  return false;
}

int getAktPumpeTrin() {
  for (int idx = 0; idx < 9; idx++) {
    if (myServo.read() == steps[idx])
      return idx+1;
  }
  //  return (myServo.read() / 30) + 1;
}

void pausePrikker(int pause) {
  lcd.setCursor(0, 3);
  lcd.print("          ");
  lcd.setCursor(0, 3);
  for (int i = 0; i < (pause - 1); i++) {
    lcd.print("-");
  }
}

int displayManualControl(int position) {
  char ch = manualOverwrite();
  Serial.write(ch);
  if (ch >= '1' && ch <= '9') {
    position = ch - '0';
    manualControl = true;
    Serial.println("\nManualControl on");
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Arduino Contrl 2.0");

    lcd.setCursor(1, 2);
    lcd.print("Pumpe (manuel) ");
    lcd.write(ch);
    lcd.setCursor(7, 3);
    lcd.print(position);
    lcd.print(" ");
    lcd.write(byte(3));
    lcd.print("r.");
    justerPumpe(position);
  }
  if (ch == 'x') {
    manualControl = false;
    Serial.println("\nManualControl off");
    skrivAutLedetekster();
    pause = 0;
  }
  return position;
}







