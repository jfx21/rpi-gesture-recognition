# Raspberry Pi 3B – Rozpoznawanie gestów dłoni (C++ / OpenCV)

System rozpoznawania gestów dłoni w czasie rzeczywistym, zoptymalizowany pod kątem minimalnego zużycia czasu procesora (CPU) na platformie Raspberry Pi 3 Model B. Projekt wykorzystuje klasyczne metody przetwarzania obrazu (Computer Vision) w przestrzeni barw YCrCb z korekcją słabego oświetlenia.

Szczegółowy opis założeń, architektury i harmonogramu znajduje się w dokumencie [Proof of Concept (POC.md)](docs/POC.md).

## Główne cechy (obecna wersja)
* **Niskie zużycie CPU:** Implementacja w C++17 kompilowana z flagami optymalizacji pod architekturę ARM Cortex-A53 (`-O3 -mcpu=cortex-a53`).
* **Brak blokowania I/O:** Wielowątkowe przechwytywanie klatek (wzorzec Producent–Konsument) w rozdzielczości 320×240 przez V4L2.
* **Motion Gating:** Automatyczne pomijanie ciężkiego przetwarzania obrazu przy statycznym kadrze (`cv::absdiff`).
* **Obsługa słabego oświetlenia:** Przetwarzanie w przestrzeni barw YCrCb z wyrównywaniem histogramu CLAHE wyłącznie na kanale jasności (`Y`).
* **Kalibracja na żywo:** Wbudowane suwaki OpenCV do strojenia maski koloru skóry pod konkretną kamerę i oświetlenie.
* **Tryb Headless:** Możliwość kompilacji bez okien graficznych (`-DHEADLESS=ON`) na potrzeby pracy przez SSH i pomiarów wydajności.

## Struktura repozytorium

```text
rpi-gesture-recognition/
├── assets/                     # (Planowane) Przeskalowane ikony PNG dla gestów
├── docs/
│   └── POC.md                  # Dokumentacja Proof of Concept
├── src/
│   ├── camera.cpp              # Wielowątkowy odczyt klatek (V4L2)
│   ├── camera.hpp
│   ├── config.cpp              # Zapis i odczyt progów YCrCb
│   ├── config.hpp
│   ├── gesture_detector.cpp    # Motion Gating, CLAHE, segmentacja i analiza dłoni
│   ├── gesture_detector.hpp
│   └── main.cpp                # Pętla główna, obsługa GUI i trybu kalibracji
├── .gitignore
├── CMakeLists.txt
└── README.md
```

## Wymagania

### Sprzęt
* Raspberry Pi 3 Model B (lub nowsze / Pi Zero 2 W)
* Kamera USB obsługująca standard V4L2 (testowane na kamerze 720p w trybie 320×240)

### Oprogramowanie
* Raspberry Pi OS (32-bit lub 64-bit)
* Kompilator `g++` (C++17) oraz `CMake`
* `OpenCV 4.x` (`libopencv-dev`)
* `v4l-utils` (do konfiguracji parametrów kamery)

## Instalacja i kompilacja

1. Zainstaluj wymagane pakiety systemowe:

```bash
sudo apt update
sudo apt install -y git cmake g++ pkg-config libopencv-dev v4l-utils
```

2. Sklonuj repozytorium i przejdź do katalogu projektu:

```bash
git clone https://github.com/jfx21/rpi-gesture-recognition.git
cd rpi-gesture-recognition
```

3. Zbuduj projekt za pomocą CMake:

```bash
mkdir -p build && cd build
cmake ..
make -j4
```

*(Dla wersji bez interfejsu graficznego — np. przez SSH lub do benchmarków — użyj `cmake -DHEADLESS=ON ..` zamiast `cmake ..`.)*

## Konfiguracja kamery i uruchomienie

Przed uruchomieniem w słabym świetle zaleca się sprawdzenie obsługiwanych trybów kamery i zablokowanie automatycznego balansu bieli:

```bash
v4l2-ctl --device=/dev/video0 --list-formats-ext
v4l2-ctl --device=/dev/video0 -c white_balance_temperature_auto=0
```

### Tryb standardowy

```bash
./gesture_app
```

*(Jeśli uruchamiasz wersję GUI przez SSH, ale z podłączonym monitorem do Raspberry Pi, użyj `DISPLAY=:0 ./gesture_app`.)*

### Tryb kalibracji progów YCrCb (wymaga GUI)

```bash
./gesture_app --calibrate
```

* Dostosuj suwaki `Y`, `Cr`, `Cb`, obserwując okno `Maska`.
* Naciśnij **`s`**, aby zapisać progi do pliku `ycrcb_config.txt` (wczytywanego automatycznie przy kolejnych uruchomieniach).
* Naciśnij **`ESC`**, aby zamknąć program.

## Benchmarki (do uzupełnienia w Etapie 5)

| Tryb działania | Rozdzielczość | Średni czas klatki [ms] | Średnie FPS | Zużycie CPU [%] |
| --- | --- | --- | --- | --- |
| Headless (bez GUI) | 320×240 | — | — | — |
| GUI | 320×240 | — | — | — |
