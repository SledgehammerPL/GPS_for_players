#Wyjaśnienie kodu

* Ustawienia WiFi: Podaj nazwę swojej sieci WiFi i hasło.
* Ustawienia serwera: Podaj adres serwera i port (zwykle 80 dla HTTP).
* Ustawienia GPS: Skonfiguruj SoftwareSerial do komunikacji z modułem GPS.
* Funkcja setup: Łączy się z WiFi.
* Funkcja loop: Co 10 sekund pobiera dane GPS i wysyła je do serwera.
* Funkcja getGPSData: Odczytuje dane NMEA z modułu GPS i wyciąga z nich odpowiednią linijkę $GPGGA.
* Funkcja parseGPGGA: Wyciąga współrzędne geograficzne z linijki $GPGGA.
* Funkcja sendToServer: Łączy się z serwerem i wysyła dane GPS w zapytaniu GET.

#Wymagania sprzętowe

* Moduł ESP-01S
* Moduł GPS NEO-7M
* Konwerter USB-UART (do programowania ESP-01S)

#Kabelki połączeniowe
Podłącz moduł GPS do ESP-01S według poniższych zaleceń:

* GPS VCC -> ESP-01S 3.3V
* GPS GND -> ESP-01S GND
* GPS TX -> ESP-01S GPIO5 (D1)
* GPS RX -> ESP-01S GPIO4 (D2)
