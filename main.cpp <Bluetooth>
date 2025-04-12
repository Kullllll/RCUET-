// BLYNK
#define BLYNK_TEMPLATE_ID "TMPL6djdmxhu3"
#define BLYNK_TEMPLATE_NAME "Smart Plant Pot"
#define BLYNK_AUTH_TOKEN "R0XH0vmzmooV0GocS_g_L2R_7G2XP1IN"

#include <Arduino.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Preferences.h>
#include <BluetoothSerial.h>
#include <nvs_flash.h>
#include <BH1750.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// BLE
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// Virtual Pins
#define VIRTUAL_TEMP V0
#define VIRTUAL_HUMID V1
#define VIRTUAL_MOIST V2
#define VIRTUAL_LED V3
#define VIRTUAL_PUMP V4
#define VIRTUAL_LIGHT V5

// Hardware Pins
#define LED_PIN 4
#define DHTPIN 5
#define DHTTYPE DHT11
#define PUMP_PIN 18
const int moisturePin = 34;

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
BH1750 lightMeter(0x23); // địa chỉ mặc định BH1750

BluetoothSerial SerialBT;
Preferences preferences;

String ssid = "", password = "";
BLECharacteristic *pBLECharacteristic;
std::string wifiStatus = "⏳ Chờ dữ liệu WiFi...";
bool newCredentialsReceived = false;

class BLECallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      String data = String(value.c_str());
      int comma = data.indexOf(',');
      if (comma != -1) {
        ssid = data.substring(0, comma);
        password = data.substring(comma + 1);
        ssid.trim(); password.trim();
        newCredentialsReceived = true;
        Serial.println("✅ Nhận BLE:");
        Serial.println("SSID: " + ssid);
        Serial.println("PASS: " + password);
      }
    }
  }

  void onRead(BLECharacteristic *pCharacteristic) override {
    pCharacteristic->setValue(wifiStatus);
    Serial.println("📤 BLE Read yêu cầu → gửi lại trạng thái WiFi");
  }
};

void setupBLE() {
  BLEDevice::init("ESP32 của Link Link Link");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService("12345678-1234-1234-1234-1234567890ab");

  pBLECharacteristic = pService->createCharacteristic(
    "abcdefab-1234-5678-90ab-abcdefabcdef",
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
  );

  pBLECharacteristic->setCallbacks(new BLECallback());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();
  Serial.println("🔵 BLE đang chờ SSID,PASS từ LightBlue...");
}

void connectToWiFiAndBlynk() {
  WiFi.disconnect(); delay(1000);
  WiFi.begin(ssid.c_str(), password.c_str());

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500); Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiStatus = "✅ WiFi Connected!";
    Serial.print("\n📶 IP: "); Serial.println(WiFi.localIP());
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid.c_str(), password.c_str());
  } else {
    wifiStatus = "❌ Kết nối thất bại!";
    Serial.println("\n❌ Kết nối WiFi thất bại!");
  }

  pBLECharacteristic->setValue(wifiStatus);
  pBLECharacteristic->notify();
}

void setup_wifi() {
  WiFi.disconnect(true); delay(1000);
  setupBLE();

  Serial.println("⏳ Chờ dữ liệu BLE (SSID,PASS):");
  while (ssid == "" || password == "") delay(500);

  connectToWiFiAndBlynk();
}

BLYNK_WRITE(VIRTUAL_PUMP) {
  int pumpState = param.asInt();
  digitalWrite(PUMP_PIN, pumpState);
  Serial.println(pumpState ? "🟢 Bơm bật" : "🔴 Bơm tắt");
}

void setup() {
  Serial.begin(115200); delay(1000);
  setup_wifi();
  dht.begin();
  Wire.begin(); // I2C
  lightMeter.begin();
  lcd.begin(16, 2);
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print(" Smart Plant Pot ");
  Serial.print("dang khoi tao");
  lcd.setCursor(0, 1);
  lcd.print("   Dang khoi tao  ");
  delay(2000);
  lcd.clear();

  pinMode(LED_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
}

void loop() {
  Blynk.run();

  int temperature = dht.readTemperature();
  int humidity = dht.readHumidity();
  int soilMoistureValue = analogRead(moisturePin);
  int moisture = map(soilMoistureValue, 4095, 0, 0, 100);
  float lux = lightMeter.readLightLevel();

  // Gửi dữ liệu lên Blynk
  Blynk.virtualWrite(VIRTUAL_TEMP, temperature);
  Blynk.virtualWrite(VIRTUAL_HUMID, humidity);
  Blynk.virtualWrite(VIRTUAL_MOIST, moisture);
  Blynk.virtualWrite(VIRTUAL_LIGHT, lux);

  // Điều khiển máy bơm tự động
  if (moisture <= 30) {
    digitalWrite(PUMP_PIN, HIGH);
    Blynk.virtualWrite(VIRTUAL_PUMP, 1);
  } else if (moisture >= 60) {
    digitalWrite(PUMP_PIN, LOW);
    Blynk.virtualWrite(VIRTUAL_PUMP, 0);
  }

  // Điều khiển đèn LED
  if (lux < 50) {
    digitalWrite(LED_PIN, HIGH);
    Blynk.virtualWrite(VIRTUAL_LED, 1);
  } else {
    digitalWrite(LED_PIN, LOW);
    Blynk.virtualWrite(VIRTUAL_LED, 0);
  }

  // LCD: Hiển thị nhiệt độ & độ ẩm
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Nhiet: "); lcd.print(temperature); lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Do am: "); lcd.print(humidity); lcd.print("%");
  delay(2500);

  // LCD: Độ ẩm đất
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Do am dat: ");
  lcd.setCursor(0, 1);
  lcd.print(moisture); lcd.print(" %");
  delay(2500);

  // LCD: Cường độ ánh sáng
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Anh sang: ");
  lcd.setCursor(0, 1);
  lcd.print((int)lux); lcd.print(" lux");
  delay(2500);

  // LCD: Trạng thái WiFi
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi: ");
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("OK");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
  } else {
    lcd.print("FAIL");
    lcd.setCursor(0, 1);
    lcd.print("Dang ket noi...");
  }
  delay(2500);

  // Serial thủ công
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == '1') {
      digitalWrite(PUMP_PIN, HIGH);
      Serial.println("🟢 Bơm bật");
      Blynk.virtualWrite(VIRTUAL_PUMP, 1);
    } else if (cmd == '0') {
      digitalWrite(PUMP_PIN, LOW);
      Serial.println("🔴 Bơm tắt");
      Blynk.virtualWrite(VIRTUAL_PUMP, 0);
    }
  }

  if (newCredentialsReceived) {
    newCredentialsReceived = false;
    Serial.println("🔁 Kết nối lại WiFi với dữ liệu mới...");
    connectToWiFiAndBlynk();
  }
}


