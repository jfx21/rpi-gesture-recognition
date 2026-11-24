# Proof of Concept: Rozpoznawanie gestów dłoni na Raspberry Pi 3B

## 1. Cel projektu
Stworzenie szybkiego i niezawodnego systemu rozpoznawania gestów dłoni w czasie rzeczywistym, zoptymalizowanego pod kątem minimalnego zużycia czasu procesora (CPU). System dodatkowo zapewnia natychmiastową informację zwrotną w postaci wyświetlania na ekranie odpowiednich ikon (emotikonów) przypisanych do rozpoznanych gestów.

## 2. Środowisko sprzętowe i ograniczenia
* **Platforma:** Raspberry Pi 3 Model B (4x ARM Cortex-A53 1.2 GHz, 1 GB RAM).
* **Sensor:** Kamera USB 720p o niskiej czułości matrycy (wysoki poziom szumu w słabym oświetleniu).
* **Główne wyzwania:**
  1. Ograniczona moc obliczeniowa CPU (brak sprzętowego akceleratora NPU/TPU).
  2. Zniekształcenia kolorystyczne i szum generowany przez kamerę przy braku dedykowanego oświetlenia.
  3. Opóźnienia (latency) wynikające z buforowania klatek na magistrali USB.
  4. Narzut graficzny związany z renderowaniem interfejsu i ikon na ekranie.

## 3. Wybór technologii i uzasadnienie
Zamiast ciężkich sieci neuronowych (np. MediaPipe Hands w Pythonie, które obciążają CPU RPi 3B w 80–100% przy 5–12 FPS), przyjęto podejście oparte na **klasycznym przetwarzaniu obrazu (Computer Vision) w języku C++ z biblioteką OpenCV**.

| Kryterium | Rozwiązanie | Oczekiwany efekt |
| :--- | :--- | :--- |
| Język i kompilacja | C++ (`-O3 -mcpu=cortex-a53`) | Wielokrotnie niższy narzut CPU niż w Pythonie |
| Rozdzielczość | Downscaling na poziomie V4L2 do 320x240 (MJPG) | Redukcja liczby przetwarzanych pikseli i odciążenie USB |
| Wielowątkowość | Wzorzec Producent–Konsument (`std::thread`) | Brak blokowania analizy przez operacje I/O kamery |
| Słabe oświetlenie | Przestrzeń YCrCb + algorytm CLAHE na kanale `Y` | Poprawa kontrastu w cieniu bez wzmacniania szumu barwnego |
| Wizualizacja gestów | Preloadowane ikony PNG (z kanałem alfa) | Brak dyskowych operacji I/O w pętli głównej; opcjonalne wyłączenie w trybie headless |

## 4. Architektura potoku przetwarzania (Pipeline)

1. **Akwizycja (Wątek Producenta):** Ciągły odczyt klatek z kamery przez sterownik V4L2 i nadpisywanie współdzielonego bufora najnowszą klatką.
2. **Preprocessing (Wątek Konsumenta):**
   * Konwersja przestrzeni barw z BGR do YCrCb (separacja luminancji od chrominancji).
   * Wyrównanie histogramu (CLAHE) wyłącznie na kanale jasności `Y`.
   * Rozmycie Gaussa (jądro 5x5) w celu redukcji szumu matrycy.
3. **Segmentacja:** Progowanie koloru skóry (`cv::inRange`) oraz operacje morfologiczne (otwarcie i zamknięcie) usuwające artefakty tła.
4. **Ekstrakcja cech i klasyfikacja:**
   * Wyznaczenie największego konturu (dłoni).
   * Obliczenie otoczki wypukłej (`cv::convexHull`) oraz defektów wypukłości (`cv::convexityDefects`).
   * Zliczanie palców na podstawie głębokości defektów i kątów między wektorami (twierdzenie cosinusów, kąt < 90°).
   * Wygładzanie czasowe (np. głosowanie większościowe z ostatnich 5 klatek), aby zapobiec migotaniu wyświetlanych ikon.
5. **Warstwa prezentacji (Tryb GUI):**
   * Nakładanie pre-wczytanej do pamięci RAM grafiki (emotikony w formacie PNG) na róg obrazu z kamery lub w osobnym oknie statusu.
   * Mapowanie przykładowe: 0 palców (pięść), 1 palec (wskazujący), 2 palce (znak pokoju), 5 palców (otwarta dłoń / stop).

## 5. Planowane metryki i kryteria sukcesu
System zostanie zweryfikowany w dwóch trybach — *headless* (czysty pomiar algorytmu) oraz *GUI* (z renderowaniem emotikonów) — na podstawie:
* **Wydajności:** Średni czas przetwarzania pojedynczej klatki poniżej 33 ms (min. 30 FPS).
* **Zużycia zasobów:** Średnie obciążenie CPU mierzone narzędziem `top`/`htop` (cel: poniżej 30–40% jednego rdzenia w trybie headless).
* **Niezawodności:** Porównanie skuteczności rozpoznawania (liczba poprawnych klasyfikacji dla gestów 0–5 palców) w dobrym oraz słabym oświetleniu.

## 6. Harmonogram realizacji (Etapy)
* [ ] **Etap 1 (Setup):** Konfiguracja środowiska na RPi 3B, kompilacja bazowego potoku C++ / OpenCV, testy sterownika `v4l2-ctl`.
* [ ] **Etap 2 (Kalibracja):** Dodanie narzędzia do strojenia progów YCrCb w trudnych warunkach oświetleniowych.
* [ ] **Etap 3 (Optymalizacja):** Wdrożenie detekcji ruchu (Motion Gating) pomijającej analizę przy statycznym obrazie oraz rozróżnianie gestu 0 (pięść) od 1 palca.
* [ ] **Etap 4 (Wizualizacja):** Implementacja bufora stabilizującego gesty oraz nakładania ikon PNG (emotikonów) z obsługą przezroczystości.
* [ ] **Etap 5 (Benchmarki):** Zebranie pomiarów czasu przetwarzania i obciążenia CPU (tryb headless vs tryb z GUI), przygotowanie końcowego raportu.