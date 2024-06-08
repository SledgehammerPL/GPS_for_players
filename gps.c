#include <ESP8266WiFi.h>
#include <FS.h>
#include <SoftwareSerial.h>

// Ustawienia GPS
SoftwareSerial gpsSerial(D1, D2);  // RX, TX

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
  
  Serial.println("\nPołączono z WiFi");
}

void loop() {
  bool isFakeData = false;
  String gpsData = getGPSData();
  if (gpsData.length() == 0) {
    gpsData = generateFakeGPSData();
    isFakeData = true;
  }
  sendToServer(gpsData, isFakeData);
  delay(10000); // Poczekaj 10 sekund przed kolejnym wysłaniem danych
}

bool readConfig() {
  File file = SPIFFS.open("/wifi.txt", "r");
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
      serverUrl = value;
    } else if (key == "player_id") {
      playerId = value;
    }
  }
  
  file.close();
  return true;
}

String getGPSData() {
  String nmeaSentence = "";
  unsigned long startTime = millis();
  
  while (millis() - startTime < 2000) { // Czekaj 2 sekundy na dane GPS
    while (gpsSerial.available() > 0) {
      char c = gpsSerial.read();
      nmeaSentence += c;
      if (c == '\n') {
        if (nmeaSentence.startsWith("$GPGGA")) {
          return parseGPGGA(nmeaSentence);
        }
        nmeaSentence = "";
      }
    }
  }
  
  return "";
}

String parseGPGGA(String nmeaSentence) {
  int commaIndex = 0;
  int previousIndex = 0;
  int fieldIndex = 0;
  String latitude = "";
  String longitude = "";
  
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
  
  return "lat=" + latitude + "&lon=" + longitude;
}

String generateFakeGPSData() {
  String latitude = "52.2296756"; // Przykładowa szerokość geograficzna (Warszawa)
  String longitude = "21.0122287"; // Przykładowa długość geograficzna (Warszawa)
  return "lat=" + latitude + "&lon=" + longitude;
}

void sendToServer(String gpsData, bool isFakeData) {
  WiFiClient client;
  if (client.connect(serverUrl.c_str(), 80)) {
    Serial.println("Połączono z serwerem");
    String url = serverUrl + "?" + gpsData + "&player_id=" + playerId;
    if (isFakeData) {
      url += "&fakedata";
    }
    
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + serverUrl + "\r\n" +
                 "Connection: close\r\n\r\n");
                 
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }
    client.stop();
    Serial.println("Dane wysłane");
  } else {
    Serial.println("Połączenie nieudane");
  }
}

