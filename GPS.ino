#include <ESP8266WiFi.h>
#include <FS.h>
#include <SoftwareSerial.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

// Ustawienia GPS
SoftwareSerial gpsSerial(2, 0);  // RX, TX (GPIO2, GPIO0)

String ssid = "";
String password = "";
String serverUrl = "";
String playerId = "";

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);
  
  // Inicjalizacja SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("Błąd inicjalizacji SPIFFS");
    return;
  }
  
  // Odczytanie danych WiFi z pliku
  if (!readConfig()) {
    Serial.println("Błąd odczytu pliku konfiguracyjnego");
    return;
  }
  
  // Łączenie z WiFi
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.print("Łączenie z WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println("\nPołączono z WiFi: " + ssid);
  printWiFiInfo();
}

void loop() {
  String gpsData = getGPSData();
  sendToServer(gpsData);
  delay(10000); // Poczekaj 10 sekund przed kolejnym wysłaniem danych
}

bool readConfig() {
  File file = SPIFFS.open("/setup.txt", "r");  // Zmiana na setup.txt
  if (!file) {
    Serial.println("Błąd otwarcia pliku");
    return false;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    int separatorIndex = line.indexOf('=');
    if (separatorIndex == -1) continue;

    String key = line.substring(0, separatorIndex);
    String value = line.substring(separatorIndex + 1);

    if (key == "ssid") {
      ssid = value;
    } else if (key == "password") {
      password = value;
    } else if (key == "server_url") {
      serverUrl = value; // Ustawia serverUrl z pliku setup.txt
    } else if (key == "player_id") {
      playerId = value;
    }
  }
  
  file.close();
  return true;
}

String getGPSData() {
  String nmeaSentence = "";
  String latitude = "";
  String longitude = "";
  unsigned long startTime = millis();
  
  while (millis() - startTime < 2000) { // Czekaj 2 sekundy na dane GPS
    while (gpsSerial.available() > 0) {
      char c = gpsSerial.read();
      nmeaSentence += c;
      if (c == '\n') {
        if (nmeaSentence.startsWith("$GPGGA")) {
          int commaIndex = 0;
          int previousIndex = 0;
          int fieldIndex = 0;
          
          while ((commaIndex = nmeaSentence.indexOf(',', previousIndex)) != -1) {
            String field = nmeaSentence.substring(previousIndex, commaIndex);
            previousIndex = commaIndex + 1;
            fieldIndex++;
            
            if (fieldIndex == 3) {
              latitude = field;
            } else if (fieldIndex == 5) {
              longitude = field;
              break;
            }
          }
          return "lat=" + latitude + "&lon=" + longitude + "&gps_data=" + nmeaSentence;
        }
        nmeaSentence = "";
      }
    }
  }
  
  return generateFakeGPSData(); // Jeśli brak danych GPS, wygeneruj dane fałszywe
}

String generateFakeGPSData() {
  String latitude = "52.2296756"; // Przykładowa szerokość geograficzna (Warszawa)
  String longitude = "21.0122287"; // Przykładowa długość geograficzna (Warszawa)
  return "lat=" + latitude + "&lon=" + longitude + "&gps_data=FAKE_DATA";
}

void sendToServer(String gpsData) {
  Serial.println("Łączenie z serwerem: " + serverUrl);
  
  WiFiClientSecure client;
  client.setInsecure(); // Ignorowanie certyfikatów SSL/TLS
  HTTPClient http;
  
  if (http.begin(client, serverUrl)) {
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String postData = "player_id=" + String(playerId) + "&" + gpsData;
    Serial.println("Wysyłanie danych do serwera: " + postData);
    int httpCode = http.POST(postData);
    if (httpCode > 0) {
      Serial.println("HTTP status code: " + String(httpCode));
      String payload = http.getString();
      Serial.println("Odpowiedź serwera: " + payload);
    } else {
      Serial.println("Błąd połączenia HTTP: " + String(http.errorToString(httpCode).c_str()));
    }
    http.end();
  } else {
    Serial.println("Nieudane połączenie z serwerem!");
  }
}

void printWiFiInfo() {
  Serial.print("Adres MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Adres IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Brama (gateway): ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("DNS: ");
  Serial.println(WiFi.dnsIP());

  // Testowanie komunikacji ze światem zewnętrznym
  Serial.println("Próba nawiązania połączenia z serwerem HTTPS");

  WiFiClientSecure client; // Tworzymy obiekt WiFiClientSecure dla HTTPS
  client.setInsecure(); // Dodajemy obsługę wszystkich protokołów SSL/TLS
  HTTPClient http;
  
  // Konfigurujemy klienta HTTP jako bezpiecznego klienta
  http.begin(client, serverUrl); // Adres serwera testowego

  int httpCode = http.GET();
  if (httpCode > 0) {
    Serial.printf("[HTTP] Odpowiedź serwera: %d\n", httpCode);
    String payload = http.getString();
    Serial.println("Odpowiedź serwera: " + payload);
  } else {
    Serial.printf("[HTTP] Błąd: %s\n", http.errorToString(httpCode).c_str());
    Serial.println(http.getString()); // Dodajemy wyświetlenie treści odpowiedzi serwera w przypadku błędu
  }
  http.end();
}
