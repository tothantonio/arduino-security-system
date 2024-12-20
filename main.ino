#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

#define RST_PIN 9
#define SS_PIN 10

MFRC522 mfrc522(SS_PIN, RST_PIN);
String MasterTag = "F7 27 FA 00";  // Enter you tag UID which we get it from first code.
String UIDCard = "";

LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo servo;

#define BlueLED 3
#define GreenLED 4
#define RedLED 2

#define Buzzer 5

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  SPI.begin();
  mfrc522.PCD_Init();

  servo.attach(6);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  pinMode(GreenLED, OUTPUT);
  pinMode(BlueLED, OUTPUT);
  pinMode(RedLED, OUTPUT);

  pinMode(Buzzer, OUTPUT);

  digitalWrite(BlueLED, HIGH);

  lcd.clear();
  lcd.print(" Access Control ");
  lcd.setCursor(0, 1);
  lcd.print("Scan Your Card>>");
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(BlueLED, HIGH);
  digitalWrite(RedLED, LOW);
  digitalWrite(GreenLED, LOW);
  noTone(Buzzer);

  //servo.write(100);
  //Wait until new tag is available
  while (getUID()) {
    Serial.print("UID: ");
    Serial.println(UIDCard);
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Permission");
    lcd.setCursor(0, 1);

    if (UIDCard == MasterTag) {
      servo.attach(6);  // Attach the servo
      lcd.print(" Access Granted!");
      digitalWrite(GreenLED, HIGH);
      digitalWrite(BlueLED, LOW);
      digitalWrite(RedLED, LOW);
      
      servo.write(180);

      for (int i = 0; i < 2; i++) {
        tone(Buzzer, 3500);
        delay(250);
        noTone(Buzzer);
        delay(250);
      }

      servo.detach();  // Detach the servo to save power and avoid jitter
    }

    else {
      lcd.print(" Access Denied!");
      digitalWrite(BlueLED, LOW);
      digitalWrite(GreenLED, LOW);
      tone(Buzzer, 3500);
      for (int i = 0; i < 10; i++) {
        digitalWrite(RedLED, HIGH);
        delay(250);
        digitalWrite(RedLED, LOW);
        delay(250);
      }
      noTone(Buzzer);
    }

    delay(2000);

    lcd.clear();
    lcd.print(" Access Control ");
    lcd.setCursor(0, 1);
    lcd.print("Scan Your Card>>");
  }
}
boolean getUID() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    //Serial.println("card Not found");
    return false;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    //Serial.println("Not able to read the card");
    return false;
  }
  UIDCard = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    UIDCard.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    UIDCard.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  UIDCard.toUpperCase();
  UIDCard = UIDCard.substring(1);

  mfrc522.PICC_HaltA();
  return true;
}