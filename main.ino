#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>
#include <DHT.h>

#define RST_PIN 9
#define SS_PIN 10

#define DHTPIN 7         // DHT11 connected to pin 7
#define DHTTYPE DHT11    // Defining the type of DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// OLED display
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MFRC522 mfrc522(SS_PIN, RST_PIN);
String MasterTag = "F7 27 FA 00";  // Enter your tag UID
String UIDCard = "";

LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD with I2C address 0x27

Servo servo;  // Servo object

#define BlueLED 3
#define GreenLED 4
#define RedLED 2
#define Buzzer 5

int MQ135Pin = A0;  // MQ135 air quality sensor connected to A0 pin

unsigned long lastSensorReadTime = 0;  // Stores last sensor reading time
unsigned long sensorInterval = 2000;   // Interval between sensor reads (2 seconds)

void setup() {
  Serial.begin(9600);

  // Initialize DHT sensor
  dht.begin();

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  delay(2000);  // Pause for 2 seconds
  display.clearDisplay();

  // Initialize RFID reader
  SPI.begin();
  mfrc522.PCD_Init();

  // Initialize servo
  servo.attach(6);

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print(" Access Control ");
  lcd.setCursor(0, 1);
  lcd.print("Scan Your Card>>");

  // Initialize LEDs and Buzzer
  pinMode(GreenLED, OUTPUT);
  pinMode(BlueLED, OUTPUT);
  pinMode(RedLED, OUTPUT);
  pinMode(Buzzer, OUTPUT);

  // Set Blue LED to HIGH and others to LOW for standby mode
  digitalWrite(BlueLED, HIGH);
  digitalWrite(GreenLED, LOW);
  digitalWrite(RedLED, LOW);
}

void loop() {
  unsigned long currentMillis = millis();  // Get current time

  // Check if it's time to update the sensor readings (every 2 seconds)
  if (currentMillis - lastSensorReadTime >= sensorInterval) {
    lastSensorReadTime = currentMillis;

    // Read temperature and humidity from DHT11 sensor
    float temperature = dht.readTemperature(); // Celsius
    float humidity = dht.readHumidity(); // %

    // Check if the readings are valid
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      temperature = 0;
      humidity = 0;
    }

    // Read air quality from MQ135 sensor
    int rawAirQuality = analogRead(MQ135Pin);  // Raw MQ135 sensor value (0-1023)

    // Average multiple readings to smooth out noise
    int numReadings = 10;
    long totalAirQuality = 0;
    for (int i = 0; i < numReadings; i++) {
      totalAirQuality += analogRead(MQ135Pin);
      delay(50);  // Short delay to give sensor time to stabilize
    }
    int averageAirQuality = totalAirQuality / numReadings;  // Calculate average

    // Determine air quality status
    String airQualityStatus;
    if (averageAirQuality < 300) {
      airQualityStatus = "Poor";
    } else if (averageAirQuality >= 300 && averageAirQuality <= 700) {
      airQualityStatus = "Moderate";
    } else {
      airQualityStatus = "Good";
    }

    // Update the OLED display with temperature, humidity, air quality value, and category
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Temp: ");
    display.print(temperature);
    display.println("C");

    display.print("Humidity: ");
    display.print(humidity);
    display.println("%");

    display.print("Air Quality: ");
    display.print(averageAirQuality);  // Display raw air quality value
    display.setCursor(0, 24);  // Move to next line for category
    display.print("Category: ");
    display.println(airQualityStatus);  // Display air quality category (Poor/Moderate/Good)
    display.display();  // Update the OLED screen
  }

  // Continuously check for RFID card presence
  if (getUID()) {
    Serial.print("UID: ");
    Serial.println(UIDCard);
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Permission");
    lcd.setCursor(0, 1);

    // Check if the detected UID matches the master tag
    if (UIDCard == MasterTag) {
      servo.attach(6);  // Attach the servo
      lcd.print(" Access Granted!");
      digitalWrite(GreenLED, HIGH);   // Green LED ON
      digitalWrite(BlueLED, LOW);     // Blue LED OFF
      digitalWrite(RedLED, LOW);      // Red LED OFF
      
      servo.write(70);  // Open door

      for (int i = 0; i < 2; i++) {
        tone(Buzzer, 3500);
        delay(250);
        noTone(Buzzer);
        delay(250);
      }

      servo.detach();  // Detach the servo to save power and avoid jitter
    } else {
      lcd.print(" Access Denied!");
      digitalWrite(BlueLED, LOW);  // Blue LED OFF
      digitalWrite(GreenLED, LOW); // Green LED OFF
      tone(Buzzer, 3500);
      for (int i = 0; i < 10; i++) {
        digitalWrite(RedLED, HIGH); // Red LED ON
        delay(250);
        digitalWrite(RedLED, LOW);  // Red LED OFF
        delay(250);
      }
      noTone(Buzzer);
    }

    // After processing, clear the LCD and go back to the waiting screen
    lcd.clear();
    lcd.print(" Access Control ");
    lcd.setCursor(0, 1);
    lcd.print("Scan Your Card>>");

    // Set Blue LED to HIGH for standby mode again
    digitalWrite(BlueLED, HIGH);
    digitalWrite(GreenLED, LOW);
    digitalWrite(RedLED, LOW);
  }
}

boolean getUID() {
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return false;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
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
