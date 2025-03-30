#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

const char* ssid = "Test_case01";
const char* password = "Hmm12345";
const char* botToken = "7744264972:AAHmQ0Ld4Qsm2e1lfVkt_MDYGYfJsGJnMeM";
const String chatID = "6196043840";
const char* telegramHost = "api.telegram.org";
const char* telegramFingerprint = "1F:77:5F:20:C5:D3:BD:67:DE:E8:07:9B:59:1D:22:E9:C0:E4:52:4B";

WiFiClientSecure client;
String lastUpdateID = "";

void sendMessage(String message) {
    if (client.connect(telegramHost, 443)) {
        String url = "/bot" + String(botToken) + "/sendMessage?chat_id=" + chatID + "&text=" + message;
        client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                     "Host: " + String(telegramHost) + "\r\n" +
                     "Connection: close\r\n\r\n");
        client.stop();
    }
}

void setup() {
    Serial.begin(115200);  
    Serial1.begin(9600);  // Gửi lệnh điều khiển đến Mega 2560
    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\n✅ WiFi Connected!");

    client.setFingerprint(telegramFingerprint);
    sendMessage("🤖 ESP8266 đã kết nối với Telegram!");
}

void loop() {
    String url = "/bot" + String(botToken) + "/getUpdates?offset=" + lastUpdateID;

    if (!client.connect(telegramHost, 443)) {
        Serial.println("❌ Kết nối Telegram API thất bại!");
        return;
    }

    // Gửi yêu cầu GET
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + String(telegramHost) + "\r\n" +
                 "Connection: close\r\n\r\n");

    // Đọc phản hồi từ server
    String response;
    while (client.available()) {
        response += client.readString();
    }
    client.stop();  

    if (response.length() == 0) {
        Serial.println("❌ Không nhận được phản hồi từ server!");
        return;
    }

    // Giải mã JSON
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
        Serial.println("❌ Lỗi phân tích JSON: " + String(error.c_str()));
        return;
    }

    JsonArray updates = doc["result"].as<JsonArray>();
    for (JsonObject update : updates) {
        String text = update["message"]["text"];
        lastUpdateID = String(update["update_id"].as<long>() + 1);

        if (text == "/on1") {
            Serial1.println("ON1");  
            sendMessage("🔹 Bật thiết bị 1");
        } else if (text == "/off1") {
            Serial1.println("OFF1");
            sendMessage("🔹 Tắt thiết bị 1");
        } else if (text == "/on2") {
            Serial1.println("ON2");
            sendMessage("🔹 Bật thiết bị 2");
        } else if (text == "/off2") {
            Serial1.println("OFF2");
            sendMessage("🔹 Tắt thiết bị 2");
        }
    }

    delay(2000);
}
