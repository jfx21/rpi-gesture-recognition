# Raspberry Pi 3B – Rozpoznawanie gestów dłoni (C++ / OpenCV)

System rozpoznawania gestów dłoni w czasie rzeczywistym, zoptymalizowany pod kątem minimalnego zużycia czasu procesora (CPU) na platformie Raspberry Pi 3 Model B. Projekt wykorzystuje klasyczne metody przetwarzania obrazu (Computer Vision), radzi sobie w warunkach słabego oświetlenia i wyświetla na ekranie ikony (emotikony) odpowiadające rozpoznanym gestom.

Szczegółowy opis założeń i architektury znajduje się w dokumencie [Proof of Concept (POC.md)](docs/POC.md).

## Główne cechy
* **Niskie zużycie CPU:** Implementacja w C++ kompilowana z flagami optymalizacji pod architekturę ARM Cortex-A53 (`-O3 -mcpu=cortex-a53`).
* **Brak blokowania I/O:** Wielowątkowe przechwytywanie klatek (wzorzec Producent–Konsument) w rozdzielczości 320×240 (MJPG przez V4L2).
* **Obsługa słabego oświetlenia:** Przetwarzanie w przestrzeni barw YCrCb z wyrównywaniem histogramu CLAHE wyłącznie na kanale jasności (`Y`).
* **Wizualizacja gestów:** Nakładanie pre-wczytanych ikon PNG na obraz wyjściowy (z możliwością całkowitego wyłączenia w trybie `HEADLESS` na potrzeby benchmarków).

## Wymagania

### Sprzęt
* Raspberry Pi 3 Model B (lub nowsze / Pi Zero 2 W)
* Kamera USB obsługująca standard V4L2 (testowane na kamerze 720p w trybie 320×240)

### Oprogramowanie
* Raspberry Pi OS (32-bit lub 64-bit)
* Kompilator `g++` (wspierający C++17)
* `OpenCV 4.x` (`libopencv-dev`)
* `v4l-utils` (do konfiguracji parametrów kamery)
* `CMake` (opcjonalnie)

## Struktura repozytorium

```text
rpi-gesture-recognition/
├── assets/                 # Przeskalowane ikony PNG (np. 64x64 px) dla gestów
├── docs/
│   └── POC.md              # Dokumentacja Proof of Concept
├── src/
│   └── main.cpp            # Kod źródłowy aplikacji
├── .gitignore
├── CMakeLists.txt
└── README.md
```

## Instalacja i kompilacja

1. Zainstaluj wymagane pakiety systemowe:

```bash
sudo apt update
sudo apt install libopencv-dev g++ pkg-config v4l-utils
```

2. Sklonuj repozytorium i przejdź do katalogu projektu:

```bash
git clone https://github.com/jfx21/rpi-gesture-recognition.git
cd rpi-gesture-recognition
```

3. Skompiluj kod źródłowy:

```bash
g++ -O3 -mcpu=cortex-a53 src/main.cpp -o gesture_app $(pkg-config --cflags --libs opencv4) -lpthread
```

*(Aby skompilować wersję bez interfejsu graficznego do pomiaru czystej wydajności algorytmu, dodaj flagę `-DHEADLESS`.)*

## Konfiguracja kamery i uruchomienie

Przed uruchomieniem w słabym świetle zaleca się zablokowanie automatycznego balansu bieli, aby zapobiec zmianom odcienia skóry (które zaburzają progowanie YCrCb):

```bash
v4l2-ctl --list-formats-ext
v4l2-ctl -c white_balance_temperature_auto=0
```

Uruchomienie aplikacji:

```bash
./gesture_app
```

Naciśnięcie klawisza `ESC` zamyka program w trybie GUI.

## Benchmarki (do uzupełnienia po testach)

| Tryb działania | Rozdzielczość | Średni czas klatki [ms] | Średnie FPS | Zużycie CPU [%] |
| --- | --- | --- | --- | --- |
| Headless (bez GUI) | 320×240 | — | — | — |
| GUI + Ikony PNG | 320×240 | — | — | — |
```