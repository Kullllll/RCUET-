#define BLYNK_TEMPLATE_ID "TMPL69RLnAsWY"
#define BLYNK_TEMPLATE_NAME "Smarth"
#define BLYNK_AUTH_TOKEN "BVNuiOIoCI5yycSayX7Wsqbs5PMk0N_K"

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

      Serial.println("📥 Đã nhận qua BLE:");
      Serial.println(bleReceivedData);

      pCharacteristic->setValue(bleStatus);
      pCharacteristic->notify();
    }
  }

  void onRead(BLECharacteristic *pCharacteristic) override {
    pCharacteristic->setValue(bleStatus);
    Serial.println("📤 BLE Read → gửi trạng thái: " + String(bleStatus.c_str()));
  }
};

void setupBLE() {
  BLEDevice::init("Home Home");
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

  Serial.println("🔵 BLE sẵn sàng. Gửi 'SSID,PASS' từ LightBlue/nRF Connect");
}

// ==== Blynk ảo & phần cứng ====
#define VIRTUAL_TEMP V0
#define VIRTUAL_HUMID V1
#define VIRTUAL_DOOR V5

#define SS_PIN 5
#define RST_PIN 16
#define BUZZER_PIN 12
#define PIN_SG90 27
#define PIN_SG90_2 13
#define RAIN_SENSOR_PIN 34
#define FLAME_SENSOR_PIN 33
MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo sg90, sg90_2;

byte validUID1[4] = {0x13, 0xA2, 0x1A, 0x2D};
byte validUID2[4] = {0x5A, 0xB2, 0xB5, 0x02};

bool doorOpen = false;
String ssid_input = "";
String password_input = "";

// === WiFi connection ===
void setup_wifi() {
  WiFi.begin(ssid_input.c_str(), password_input.c_str());
  Serial.print("📶 Đang kết nối tới WiFi: ");
  Serial.println(ssid_input);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Đã kết nối WiFi!");
    wifiReady = true;
    bleStatus = "✅ Kết nối WiFi thành công!";
  } else {
    Serial.println("\n❌ Kết nối WiFi thất bại!");
    bleStatus = "❌ Kết nối WiFi thất bại!";
  }

  pBLECharacteristic->setValue(bleStatus);
  pBLECharacteristic->notify();
}

// === RFID & Servo ===
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
    sg90.write(0);
    doorOpen = false;
    Serial.println("🛠 Cửa đóng lại theo trạng thái cũ");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Door closed");
  }
}

void checkRFID() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    lcd.setCursor(0, 0);
    lcd.print("Scan card...     ");
    return;
  }

  Serial.print("UID: ");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("UID: ");

  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    lcd.print(mfrc522.uid.uidByte[i], HEX);
    lcd.print(" ");
  }
  Serial.println();

  if (compareUID(mfrc522.uid.uidByte, validUID1) || compareUID(mfrc522.uid.uidByte, validUID2)) {
    Serial.println("✅ Truy cập hợp lệ");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Granted");

    beep(200);  // 📢 Kêu khi đúng
    delay(2000);

    sg90.write(180);
    saveDoorState(true);
    delay(5000);
    sg90.write(0);
    saveDoorState(false);

    Serial.println("🔒 Cửa đã đóng");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Door closed");
  } else {
    Serial.println("❌ Thẻ không hợp lệ");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Denied");

    beep(500);  // 📢 Kêu dài khi sai
  }

  mfrc522.PICC_HaltA();
}

void readSensors() {
  int temperature = random(20, 35);
  int humidity = random(40, 80);
  Serial.print("🌡 Temp: "); Serial.println(temperature);
  Serial.print("💧 Humi: "); Serial.println(humidity);
  Blynk.virtualWrite(VIRTUAL_TEMP, temperature);
  Blynk.virtualWrite(VIRTUAL_HUMID, humidity);
  delay(500);
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RAIN_SENSOR_PIN, INPUT);
  pinMode(FLAME_SENSOR_PIN, INPUT);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("🔋 Khoi dong...");

  SPI.begin();
  mfrc522.PCD_Init();

  sg90.attach(PIN_SG90, 500, 2400);
  sg90_2.attach(PIN_SG90_2, 500, 2400);
  sg90.write(0);
  sg90_2.write(0);

  setupBLE();

  Serial.println("✅ Đã khởi động xong!");
}

void loop() {
  if (!wifiReady && newBLEDataReceived) {
    if (bleReceivedData.indexOf(',') != -1) {
      int splitIndex = bleReceivedData.indexOf(',');
      ssid_input = bleReceivedData.substring(0, splitIndex);
      password_input = bleReceivedData.substring(splitIndex + 1);
      ssid_input.trim();
      password_input.trim();

      Serial.println("📡 Đang kết nối với:");
      Serial.println("SSID: " + ssid_input);
      Serial.println("PASS: " + password_input);

      setup_wifi();
      if (wifiReady) {
        Blynk.begin(BLYNK_AUTH_TOKEN, ssid_input.c_str(), password_input.c_str());
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("WiFi OK. Blynk on");
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
  Serial.print("📊 Trạng thái cảm biến mưa (digital): ");
  Serial.println(rainState);

  if (rainState == LOW) {
    Serial.println("🌧 Có mưa → servo 2 quay 90 độ");
    sg90_2.write(90);
  } else {
    Serial.println("☀️ Không mưa → servo 2 quay về 0 độ");
    sg90_2.write(0);
  }
  int flameState = analogRead(FLAME_SENSOR_PIN);
  Serial.println(flameState);
  if (flameState >= 200) {
    Serial.println("✅ Không có lửa.");
  } else {
    Serial.println("🔥 Có lửa!");
  }
  delay(200);
}
