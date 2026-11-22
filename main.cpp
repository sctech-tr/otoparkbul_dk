#include <WiFi.h>
#include <HTTPClient.h>

// Wi-Fi
const char* ssid = "x"; // değiştirin
const char* password = "x"; // değiştirin

// Sunucu
const char* sunucuURL = "http://x:3000"; 

// "x" adına kayıtlı API anahtarı // değiştirin
const char* apiKey = "x"; // değiştirin

// Sensör sayısı
const int TOPLAM_PARK = 5;

// Sensör pinleri
// not: deneyap kart'ta
// D0, D1, D4, D9, D12, D13, D14, D15, A4, A5
// pinleri sorunsuz kullanılabilir.
// yani toplam 5 park yeri için pin var.
// D0 ve D1'den emin değilim
const int trigPinleri[TOPLAM_PARK] = {D13, D0, D4, D12, A4};
const int echoPinleri[TOPLAM_PARK] = {D14, D1, D9, D15, A5};

bool parkDurumlari[TOPLAM_PARK] = {false, false, false, false, false};

// Araç algılama mesafesi
const int esikMesafe = 20;


int mesafeOlcu(int trigPin, int echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);

    digitalWrite(trigPin, LOW);

    long sure = pulseIn(echoPin, HIGH, 30000);

    if (sure == 0) return 999;

    return sure * 0.034 / 2;
}


void webSitesineGonder(int parkID, int durum) {

    if (WiFi.status() == WL_CONNECTED) {

        HTTPClient http;

        String tamURL = String(sunucuURL) + "/ee";

        // 1 = entry, 0 = exit
        String type = durum == 1 ? "entry" : "exit";

        // JSON verisi
        String httpRequestData =
            "{\"type\":\"" + type +
            "\",\"spaceNumber\":" + String(parkID) + "}";

        Serial.println("[Wi-Fi] Sunucuya veri gönderiliyor...");

        http.begin(tamURL);

        // Gerekli HTTP başlıkları
        http.addHeader("Content-Type", "application/json");
        http.addHeader("x-api-key", apiKey);

        // POST isteği
        int httpKodu = http.POST(httpRequestData);

        if (httpKodu > 0) {

            Serial.print("[Wi-Fi] HTTP kodu: ");
            Serial.println(httpKodu);

            Serial.print("[Wi-Fi] Sunucu yanıtı: ");
            Serial.println(http.getString());

        } else {

            Serial.print("[Wi-Fi] Hata: ");
            Serial.println(http.errorToString(httpKodu));
        }

        http.end();

    } else {

        Serial.println("[Wi-Fi] Bağlantı yok!");
    }
}


void setup() {

    Serial.begin(115200);
    delay(1500);

    Serial.println("Başladı! :)");

    for (int i = 0; i < TOPLAM_PARK; i++) {
        pinMode(trigPinleri[i], OUTPUT);
        pinMode(echoPinleri[i], INPUT);
    }

    Serial.print("Wi-Fi ağına bağlanılıyor: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWi-Fi bağlantısı başarılı!");

    Serial.print("IP adresi: ");
    Serial.println(WiFi.localIP());
}


void loop() {

    for (int i = 0; i < TOPLAM_PARK; i++) {

        int anlikMesafe = mesafeOlcu(
            trigPinleri[i],
            echoPinleri[i]
        );

        if (anlikMesafe > 0 && anlikMesafe < 400) {

            // Araba geldi
            if (anlikMesafe <= esikMesafe && !parkDurumlari[i]) {

                parkDurumlari[i] = true;

                int parkID = i + 1;

                Serial.print("[PARK ");
                Serial.print(parkID);
                Serial.println("] --> ARABA GELDİ!");

                webSitesineGonder(parkID, 1);
            }

            // Araba gitti
            else if (anlikMesafe > esikMesafe && parkDurumlari[i]) {

                parkDurumlari[i] = false;

                int parkID = i + 1;

                Serial.print("[PARK ");
                Serial.print(parkID);
                Serial.println("] <-- ARABA GİTTİ!");

                webSitesineGonder(parkID, 0);
            }
        }

        delay(30);
    }

    delay(200);
}
