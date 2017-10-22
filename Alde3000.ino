#include <dht.h>

dht DHT11;
dht DHT22;

#define DHT22_PIN 6
#define DHT11_PIN 7

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

Servo myServo;

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

byte termo[8] = {B00100, B01010, B01010, B01010, B01010, B10101, B10001, B01110};
byte grad[8] = {B11100, B10100, B11100, B00000, B00000, B00000, B00000,};
byte oe[8] = {B00000, B00000, B01110, B10011, B10101, B11001, B01110, B00000};
byte g[8] = {B00000, B00000, B01111, B10001, B10001, B01111, B00001, B01110};
byte hPunktum[8] = {B00000, B00000, B00000, B00000, B00000, B00011, B00011, B00000};

boolean prik = false;
boolean manualControl = false;
static int trin1 = 3;
static int trin2 = 5;
static int trin3 = 10;
static int trin4 = 16;
static int pumpeFaktor = 30;
int initPause = 10;
int pause = initPause;

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

  while (! Serial);
  Serial.println("Enter position to set Servo: 1 to 5");

  myServo.attach(9);
  myServo.write(60);

  skrivAutLedetekster();

  delay(1000);

}


void loop() {
  float frem = 0.0;
  float retur = 0.0;
  int position = 0;

  heartBeat();

  if (Serial.available()) {
    char ch = manualOverwrite();
    Serial.write(ch);
    if (ch >= '1' && ch <= '5') {
      position = ch - '0';
      manualControl = true;
      Serial.println("manualControl on");
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("Alde Compact 3000");

      lcd.setCursor(1, 2);
      lcd.print("Pumpe (manuel) ");
      lcd.write(ch);
      lcd.setCursor(7, 3);
      lcd.print((position - 1) * pumpeFaktor);
      lcd.print(" ");
      lcd.write(byte(3));
      lcd.print("r.");
      justerPumpe(position);
    }
    if (ch == 'x') {
      manualControl = false;
      Serial.println("manualControl off");
      skrivAutLedetekster();
      pause = 0;
    }
  }

  // Fremløb
  callDHT11();
  delay(1000);
  frem = DHT11.temperature;
  Serial.print("frem ");
  Serial.print(frem);
  if (isTempValid(frem) && !manualControl) {
    lcd.setCursor(2, 1);
    lcd.print(frem, 0);
    lcd.write(byte(1));
  }

  // Returløb
  callDHT22();
  delay(1000);
  retur = DHT22.temperature;
  Serial.print("\tretur ");
  Serial.print(retur);
  int diff = (frem - retur);
  Serial.print("\tdiff ");
  Serial.println(diff);
  if (isTempValid(retur) && !manualControl) {
    lcd.setCursor(8, 1);
    lcd.print(retur, 0);
    lcd.write(byte(1));
    //Diff
    lcd.setCursor(13, 1);
    lcd.print(diff, 0);
    lcd.write(byte(1));
  }

  lcd.setCursor(10,2);
  lcd.print(getAktPumpeTrin());
  
  // Skal pumpen justeres (spring over ved fejllæsning
  if (isTempValid(frem) && isTempValid(retur) && !manualControl) {
    position = findTempDiff(frem, retur);
    lcd.setCursor(17, 1);
    lcd.print(position);
    pause--;
    if (pause < 1) {
      justerPumpe(position);
      pause = initPause;
    }
  }


  // Ro på loop
  delay(2000);
}


void justerPumpe(int pos) {
  if (pos >= 1 && pos <= 5) {
    int nuvPos = myServo.read();
    int nyPos = (pos - 1) * pumpeFaktor;
    if (nyPos > nuvPos) {
      for (int idx = nuvPos; idx <= nyPos; idx++) {
        myServo.write(idx);
        delay(50);
      }
    } else {
      for (int idx = nuvPos; idx >= nyPos; idx--) {
        myServo.write(idx);
        delay(50);
      }
    }

    Serial.print("Servo sat til ");
    Serial.print(myServo.read());
    Serial.println(" gr.");
    Serial.print(" - trin ");
    Serial.println(pos);
  }
}

int getAktPumpeTrin() {
    return (myServo.read() / 30) + 1;
}

char manualOverwrite() {
  char ch = Serial.read();
  Serial.write(ch);
  if (ch >= '1' && ch <= '5')
  {
    Serial.print("Servo saettes til ");
    return ch;
  }
  if (ch == 'x')
  {
    Serial.println("Manual overwrite exited");
    return ch;
  }
}

int findTempDiff(int frem, int retur) {
  int diff = (frem - retur);
  if (diff < trin1) {
    return 1;
  }
  if (diff > trin1  && diff <= trin2)  {
    return 2;
  }
  if (diff > trin2  && diff <= trin3)  {
    return 3;
  }
  if (diff > trin3  && diff <= trin4)  {
    return 4;
  }
  if (diff > trin4) {
    return 5;
  }
  return 0;
}

void skrivAutLedetekster() {
  lcd.clear();
  //  lcd.setCursor(1, 0);
  //  lcd.print("Alde Compact 3000");

  lcd.setCursor(1, 0);
  lcd.print("Frem");

  lcd.setCursor(6, 0);
  lcd.print("Retur");

  lcd.setCursor(12, 0);
  lcd.print("Dif");

  lcd.setCursor(16, 0);
  lcd.print("pmp");

  lcd.setCursor(2,2);
  lcd.print("Akt.Pmp");
}

void callDHT22() {
  int chk = DHT22.read22(DHT22_PIN);
  switch (chk)
  {
    case DHTLIB_OK:
      Serial.print("DHT22: OK,\t");
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.print("DHT22: Checksum error,\t");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      Serial.print("DHT22: Time out error,\t");
      break;
    default:
      Serial.print("DHT22: Unknown error,\t");
      break;
  }

}

void callDHT11() {
  int chk = DHT11.read11(DHT11_PIN);
  switch (chk)
  {
    case DHTLIB_OK:
      Serial.print("DHT11: OK,\t");
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.print("DHT11: Checksum error,\t");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      Serial.print("DHT11: Time out error,\t");
      break;
    default:
      Serial.print("DHT11: Unknown error,\t");
      break;
  }
}

void heartBeat() {
  //   Serial.print(prik);
  if (prik == false) {
    lcd.setCursor(0, 0);
    lcd.print(".");
    lcd.setCursor(18, 0);
    lcd.print(" ");
    prik = true;
  } else {
    lcd.setCursor(18, 0);
    lcd.write(byte(4));
    lcd.setCursor(0, 0);
    lcd.print(" ");
    prik = false;
  }
}

boolean isTempValid(float temp) {
  if (temp >= 0.00 && temp <= 99.00) {
    return true;
  }
  return false;
}




