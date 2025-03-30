#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_Sensor.h>

// Cấu hình cảm biến DHT11
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Cấu hình chân điều khiển rơ le
#define RELAY1 3  // Điều khiển cặp 1
#define RELAY2 4  // Điều khiển cặp 2
#define BUZZER 6  // Chuông cảnh báo

// Cảm biến khói/gas
#define SMOKE_SENSOR A0  

// Biến kiểm soát hệ thống
unsigned long startTime = 0;
bool isRelay1Running = true;
bool errorInRelay2 = false;
bool fireDetected = false;

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);  // Nhận lệnh từ ESP8266

    dht.begin();
    pinMode(RELAY1, OUTPUT);
    pinMode(RELAY2, OUTPUT);
    pinMode(BUZZER, OUTPUT);
    pinMode(SMOKE_SENSOR, INPUT);
    
    digitalWrite(RELAY1, LOW);
    digitalWrite(RELAY2, LOW);
    digitalWrite(BUZZER, LOW);
    
    startTime = millis();
}

void loop() {
    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();
    int smokeLevel = analogRead(SMOKE_SENSOR);

    if (isnan(temp) || isnan(humidity)) {
        Serial.println("Lỗi đọc cảm biến!");
        return;
    }

    Serial.print("Nhiệt độ: "); Serial.print(temp);
    Serial.print("°C | Độ ẩm: "); Serial.print(humidity);
    Serial.println("%");

    // Giám sát cháy
    if (smokeLevel > 400) {  // Ngưỡng phát hiện cháy
        Serial.println("🔥 CẢNH BÁO: Phát hiện cháy!");
        fireDetected = true;
        digitalWrite(BUZZER, HIGH);
    } else {
        fireDetected = false;
        digitalWrite(BUZZER, LOW);
    }

    unsigned long currentTime = millis();
    unsigned long elapsedTime = (currentTime - startTime) / 1000; // Tính theo giây

    if (elapsedTime >= 600 && elapsedTime < 900) {
        Serial.println("⏳ Cặp 2 đang khởi động...");
        digitalWrite(RELAY2, HIGH);
        if (random(0, 10) < 2) {  // 20% khả năng lỗi
            Serial.println("⚠️ LỖI: Cặp 2 không thể khởi động!");
            errorInRelay2 = true;
            digitalWrite(RELAY2, LOW);
        }
    }

    if (elapsedTime >= 900) {  
        if (errorInRelay2) {
            Serial.println("✅ Cặp 1 tiếp tục chạy do cặp 2 gặp sự cố.");
            errorInRelay2 = false;
        } else {
            isRelay1Running = !isRelay1Running;
            Serial.println("🔄 Chuyển cặp điều hòa...");
        }

        digitalWrite(RELAY1, isRelay1Running ? HIGH : LOW);
        digitalWrite(RELAY2, !isRelay1Running ? HIGH : LOW);
        startTime = millis();
    }

    // Nhận lệnh từ ESP8266 qua Serial1
    if (Serial1.available()) {
        String command = Serial1.readStringUntil('\n');
        command.trim();  

        if (command == "ON1") {
            digitalWrite(RELAY1, HIGH);
            Serial.println("⚡ Bật cặp 1!");
            Serial1.println("RELAY1_ON");
        } else if (command == "OFF1") {
            digitalWrite(RELAY1, LOW);
            Serial.println("❌ Tắt cặp 1!");
            Serial1.println("RELAY1_OFF");
        } else if (command == "ON2") {
            digitalWrite(RELAY2, HIGH);
            Serial.println("⚡ Bật cặp 2!");
            Serial1.println("RELAY2_ON");
        } else if (command == "OFF2") {
            digitalWrite(RELAY2, LOW);
            Serial.println("❌ Tắt cặp 2!");
            Serial1.println("RELAY2_OFF");
        } else if (command == "STATUS") {  
            String status = "🔥 Trạng thái: \n";
            status += " - Cặp 1: " + String(digitalRead(RELAY1) ? "Bật" : "Tắt") + "\n";
            status += " - Cặp 2: " + String(digitalRead(RELAY2) ? "Bật" : "Tắt") + "\n";
            status += " - Nhiệt độ: " + String(temp) + "°C\n";
            status += " - Độ ẩm: " + String(humidity) + "%\n";
            status += " - Khói/Gas: " + String(smokeLevel) + "\n";
            Serial1.println(status);
        }
    }

    delay(1000);
}
