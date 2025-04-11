#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

String ssid = "", password = "";
bool newCredentialsReceived = false;
std::string wifiStatus = "⏳ Chờ dữ liệu WiFi...";
BLECharacteristic* pBLECharacteristic;

// BLE Callback xử lý dữ liệu từ LightBlue
class BLECallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      String data = String(value.c_str());
      int comma = data.indexOf(',');
      if (comma != -1) {
        ssid = data.substring(0, comma);
        password = data.substring(comma + 1);
        ssid.trim();
        password.trim();
        newCredentialsReceived = true;

        Serial.println("✅ Nhận từ BLE:");
        Serial.println("SSID: " + ssid);
        Serial.println("PASS: " + password);
      }
    }
  }

  void onRead(BLECharacteristic *pCharacteristic) override {
    pCharacteristic->setValue(wifiStatus);
    Serial.println("📤 LightBlue yêu cầu READ → gửi trạng thái");
  }
};

void setupBLE() {
  BLEDevice::init("ESP32 BLE WiFi Setup");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService("12345678-1234-1234-1234-1234567890ab");

  pBLECharacteristic = pService->createCharacteristic(
    "abcdefab-1234-5678-90ab-abcdefabcdef",
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );

  pBLECharacteristic->setCallbacks(new BLECallback());

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->start();

  Serial.println("🔵 BLE đang chờ dữ liệu từ LightBlue (ssid,password)...");
}

void setup() {
  Serial.begin(115200);
  setupBLE();
}

void loop() {
  if (newCredentialsReceived) {
    newCredentialsReceived = false;

    Serial.println("\n📡 Dữ liệu mới nhận được:");
    Serial.println("SSID: " + ssid);
    Serial.println("PASS: " + password);

    // 🟡 Thêm logic ở đây để dùng ssid và password, ví dụ WiFi.begin(ssid, password);

    // Phản hồi BLE
    wifiStatus = "✅ Dữ liệu nhận OK!";
    pBLECharacteristic->setValue(wifiStatus);
    pBLECharacteristic->notify();
  }

  delay(1000);
}
