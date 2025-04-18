//Smart_Home
#define BLYNK_TEMPLATE_ID "TMPL6gP7fXMxS"
#define BLYNK_TEMPLATE_NAME "Smart Home"
#define BLYNK_AUTH_TOKEN "lkM5hQg2XQjv_kwpEneU5fyOTvr-zHdW"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <DHT.h>  

// ==== Blynk ảo & phần cứng ====
#define VIRTUAL_TEMP V0
#define VIRTUAL_HUMID V1
#define VIRTUAL_RAIN V2
#define VIRTUAL_FLAME V3
#define VIRTUAL_GAS V4
#define VIRTUAL_DOOR V5
#define VIRTUAL_GARAGE_OPEN V6
#define VIRTUAL_GARAGE_CLOSE V7 

#define SS_PIN 5
#define RST_PIN 16
#define BUZZER_PIN 12
#define PIN_SG90 27
#define PIN_SG90_2 13
#define RAIN_SENSOR_PIN 34
#define FLAME_SENSOR_PIN 33
#define GAS_SENSOR_PIN 35
#define BUTTON1_PIN 26
#define BUTTON2_PIN 14      
#define DHTPIN 15             
#define DHTTYPE DHT11 
#define SERVO_PIN 25

WidgetTerminal terminal(V8);

// ==== BLE setup ====
BLECharacteristic *pBLECharacteristic;
std::string bleStatus = "⏳ Đang chờ dữ liệu từ BLE...";
bool newBLEDataReceived = false;
String bleReceivedData = "";
bool wifiReady = false;

class BLECallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      bleReceivedData = String(value.c_str());
      bleReceivedData.trim();
      newBLEDataReceived = true;

      pCharacteristic->setValue(bleStatus);
      pCharacteristic->notify();
    }
  }

  void onRead(BLECharacteristic *pCharacteristic) override {
    pCharacteristic->setValue(bleStatus);
  }
};

void setupBLE() {
  BLEDevice::init("SmartHome");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService("12345678-1234-1234-1234-1234567890ab");

  pBLECharacteristic = pService->createCharacteristic(
    "abcdefab-1234-5678-90ab-abcdefabcdef",
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  pBLECharacteristic->setCallbacks(new BLECallback());
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();
}

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo sg90, sg90_2, sg360;
DHT dht(DHTPIN, DHTTYPE); 

byte validUID1[4] = {0x13, 0xA2, 0x1A, 0x2D};
byte validUID2[4] = {0x5A, 0xB2, 0xB5, 0x02};

bool doorOpen = false;
String ssid_input = "";
String password_input = "";
bool reverseDirection = false;

bool garageForward = false;
bool garageReverse = false;

void setup_wifi() {
  WiFi.begin(ssid_input.c_str(), password_input.c_str());
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiReady = true;
    bleStatus = "✅ Kết nối WiFi thành công!";
    terminal.println("✅ Kết nối WiFi thành công!");
  } else {
    bleStatus = "❌ Kết nối WiFi thất bại!";
    terminal.println("❌ Kết nối WiFi thất bại!");
  }

  pBLECharacteristic->setValue(bleStatus);
  pBLECharacteristic->notify();
}

bool compareUID(byte *cardUID, byte *targetUID) {
  for (byte i = 0; i < 4; i++) {
    if (cardUID[i] != targetUID[i]) return false;
  }
  return true;
}

void beep(int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

void saveDoorState(bool state) { doorOpen = state; }

void restoreDoorState() {
  if (doorOpen) {
    sg90.write(90);
    doorOpen = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Door closed");
    terminal.println("Door closed");
  }
}

void checkRFID() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    lcd.setCursor(0, 0);
    lcd.print("Scan card...     ");
    terminal.println("Scan card...     ");
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("UID: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    lcd.print(mfrc522.uid.uidByte[i], HEX);
    lcd.print(" ");
  }

  if (compareUID(mfrc522.uid.uidByte, validUID1) || compareUID(mfrc522.uid.uidByte, validUID2)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Granted");
    terminal.println("Access Granted");
    Blynk.virtualWrite(VIRTUAL_DOOR, HIGH);

    beep(500);
    delay(2000);

    sg90.write(180);
    saveDoorState(true);
    delay(5000);
    sg90.write(90);
    saveDoorState(false);

    Blynk.virtualWrite(VIRTUAL_DOOR, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Door closed");
    terminal.println("Door closed");
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Denied");
    terminal.println("Access Denied");
    beep(500);
  }

  mfrc522.PICC_HaltA();
}

void readSensors() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    return;
  }

  Blynk.virtualWrite(VIRTUAL_TEMP, temperature);
  Blynk.virtualWrite(VIRTUAL_HUMID, humidity);

  lcd.setCursor(0, 1);
  lcd.print("T:"); lcd.print(temperature); lcd.print("C ");
  lcd.print("H:"); lcd.print(humidity); lcd.print("%");
  terminal.print("T:"); terminal.print(temperature); terminal.print("C \n");
  terminal.print("H:"); terminal.print(humidity); terminal.print("%\n");
  terminal.flush();
  delay(500);
}

void updateGarageServo() {
  if (garageForward && !garageReverse) {
    sg360.write(180); // quay xuôi
  } else if (garageReverse && !garageForward) {
    sg360.write(0); // quay ngược
  } else {
    sg360.write(92); // dừng
  }
}

// ===== Blynk SWITCH điều khiển servo 360 độ =====
BLYNK_WRITE(VIRTUAL_GARAGE_OPEN) {
  garageForward = param.asInt();
  updateGarageServo();
}

BLYNK_WRITE(VIRTUAL_GARAGE_CLOSE) {
  garageReverse = param.asInt();
  updateGarageServo();
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RAIN_SENSOR_PIN, INPUT);
  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP); 
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Khoi dong...");
  terminal.println("Khoi dong...");

  SPI.begin();
  mfrc522.PCD_Init();

  sg90.attach(PIN_SG90, 500, 2400);
  sg90_2.attach(PIN_SG90_2, 500, 2400);
  sg360.attach(SERVO_PIN);
  sg90.write(90);
  sg90_2.write(0);
  sg360.write(92); // dừng ban đầu

  dht.begin(); 
  setupBLE();
}

void loop() {
  if (!wifiReady && newBLEDataReceived) {
    if (bleReceivedData.indexOf(',') != -1) {
      int splitIndex = bleReceivedData.indexOf(',');
      ssid_input = bleReceivedData.substring(0, splitIndex);
      password_input = bleReceivedData.substring(splitIndex + 1);
      ssid_input.trim();
      password_input.trim();

      setup_wifi();
      if (wifiReady) {
        Blynk.begin(BLYNK_AUTH_TOKEN, ssid_input.c_str(), password_input.c_str());
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("WiFi OK. Blynk on");
        terminal.print("WiFi OK. Blynk on");
        delay(1000);
        restoreDoorState();
      }
    }
  }

  if (wifiReady) {
    Blynk.run();
    readSensors();
    checkRFID();
  }

  int rainState = digitalRead(RAIN_SENSOR_PIN);
  Blynk.virtualWrite(VIRTUAL_RAIN, rainState == LOW ? 1 : 0);
  if (rainState == LOW) {
    sg90_2.write(90);
  } else {
    sg90_2.write(0);
  }

  int flameState = analogRead(FLAME_SENSOR_PIN);
  if (flameState >= 200) {
    Blynk.virtualWrite(VIRTUAL_FLAME, LOW);
  } else {
    Blynk.virtualWrite(VIRTUAL_FLAME, HIGH);
  }

  int gasState = analogRead(GAS_SENSOR_PIN);
  if (gasState <= 800){
    Blynk.virtualWrite(VIRTUAL_GAS, LOW);
  } else {
    Blynk.virtualWrite(VIRTUAL_GAS, HIGH);
  }

  bool flameDetected = (flameState <= 200);
  bool gasDetected = (gasState > 1000);
  if (flameDetected || gasDetected) {
    // Mở cửa
    sg90.write(180);
    saveDoorState(true);
    Blynk.virtualWrite(VIRTUAL_DOOR, HIGH);

    // Còi kêu liên tục cho đến khi an toàn
    while (flameDetected || gasDetected) {
      beep(1000);
      delay(300);

      flameDetected = (analogRead(FLAME_SENSOR_PIN) <= 200);
      gasDetected = (analogRead(GAS_SENSOR_PIN) > 1000);
    }

    // Khi hết nguy hiểm: đóng cửa
    sg90.write(90);
    saveDoorState(false);
    Blynk.virtualWrite(VIRTUAL_DOOR, LOW);
  }

  // Nút vật lý điều khiển servo 360
  bool currentButtonState = digitalRead(BUTTON1_PIN) == LOW;
  bool reverseButtonState = digitalRead(BUTTON2_PIN) == LOW;
  if (currentButtonState && !reverseButtonState) {
    sg360.write(180);
  } else if (reverseButtonState && !currentButtonState) {
    sg360.write(0);
  } else if (!garageForward && !garageReverse) {
    sg360.write(92); // chỉ dừng nếu cả Blynk và nút đều tắt
  }

  delay(1000);
}
