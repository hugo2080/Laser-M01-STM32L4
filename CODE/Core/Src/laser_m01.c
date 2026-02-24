/*
 * laser_m01.c
 */

#include "laser_m01.h"

// --- Zmienne prywatne modułu ---
static UART_HandleTypeDef *phuart; // Wskaźnik na UART czujnika
static GPIO_TypeDef *pEnaPort;
static uint16_t pEnaPin;

// Zmienna przechowująca ostatni błąd
static M01_Error_t _lastErrorCode = M01_ERR_NONE;

// Komendy (bajty)
static const uint8_t CMD_LASER_ON[] = {0xAA, 0x00, 0x01, 0xBE, 0x00, 0x01, 0x00, 0x01, 0xC1};
static const uint8_t CMD_LASER_OFF[] = {0xAA, 0x00, 0x01, 0xBE, 0x00, 0x01, 0x00, 0x00, 0xC0};
static const uint8_t CMD_QUICK[] = {0xAA, 0x00, 0x00, 0x22, 0x00, 0x01, 0x00, 0x00, 0x23};

// Bufor DMA
#define RX_BUFFER_SIZE 256 // Zwiększamy bufor dla bezpieczeństwa przy trybie ciągłym
static uint8_t rx_buffer[RX_BUFFER_SIZE]; // Uwaga: Usunięto 'volatile', DMA tego nie potrzebuje w ten sposób
static volatile uint16_t rx_tail = 0;     // Ogon (gdzie skończyliśmy czytać)
// Głowa (rx_head) jest teraz zarządzana przez sprzęt DMA!

static float _lastMeters = NAN;

// --- Funkcje pomocnicze (wewnętrzne) ---
static bool csumOK(const uint8_t *f, int n) {
	uint32_t s = 0;
	for(int i = 1; i < n - 1; i++)
		s += f[i];
	return ((uint8_t)s) == f[n - 1];
}

static uint32_t bcd32(const uint8_t *b) {
	uint32_t v = 0;
	for(int i = 0; i < 4; i++)
		v = v * 100 + ((b[i] >> 4) & 0x0F) * 10 + (b[i] & 0x0F);
	return v;
}

static void enaHigh() {
	HAL_GPIO_WritePin(pEnaPort, pEnaPin, GPIO_PIN_SET);
}
static void enaLow() {
	HAL_GPIO_WritePin(pEnaPort, pEnaPin, GPIO_PIN_RESET);
}

// --- Implementacja API ---

void M01_Init(UART_HandleTypeDef *huart_sensor, GPIO_TypeDef *ena_port, uint16_t ena_pin) {
	phuart = huart_sensor;
	pEnaPort = ena_port;
	pEnaPin = ena_pin;

	enaHigh(); // Włączamy zasilanie modułu

	// --- START DMA ---
	// Uruchamiamy odbiór w trybie kołowym (Circular).
	// Procesor nie robi nic, DMA samo wpisuje bajty do rx_buffer i zawija na początku.
	HAL_UART_Receive_DMA(phuart, rx_buffer, RX_BUFFER_SIZE);
}

// Funkcja obliczająca gdzie DMA aktualnie "pisze"
static uint16_t get_dma_head(void) {
	// __HAL_DMA_GET_COUNTER zwraca liczbę POZOSTAŁYCH do przesłania danych.
	// Więc: Head = Rozmiar - Pozostało
	return RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(phuart->hdmarx);
}

float M01_Process(void) {
	static uint8_t f[32];
	static int pos = 0;
	static uint32_t last_byte_time = 0;

	uint16_t rx_head = get_dma_head();

	while(rx_tail != rx_head) {
		uint8_t b = rx_buffer[rx_tail];
		rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;

		if(pos > 0 && (HAL_GetTick() - last_byte_time > 50))
			pos = 0;
		last_byte_time = HAL_GetTick();

		if(pos == 0 && b != 0xAA)
			continue; // Szukamy nagłówka 0xAA

		f[pos++] = b;

		// --- Walidacja Ramki ---
		// Ramka sukcesu ma zazwyczaj 13 bajtów.
		// Ramka błędu może być krótsza lub inna.
		// Zakładamy standardową strukturę 13 bajtów dla M01.

		if(pos >= 13) {
			if(csumOK(f, 13)) { // Sprawdź sumę kontrolną (dla 13 bajtów)
				uint8_t func = f[3]; // Bajt funkcji/komendy
				pos = 0; // Czyścimy TYLKO po pełnej ramce

				if(func == 0x20 || func == 0x21 || func == 0x22) { // 1. PRAWIDŁOWY POMIAR (0x22, 0x20, 0x21 - kody pomiaru)
					_lastErrorCode = M01_ERR_NONE;
					float dist = bcd32(&f[6]) / 1000.0f;
					_lastMeters = 1.0016f * dist + 0.0049f;
					return _lastMeters;
				} else if(func == 0xEE) { // 2. RAMKA BŁĘDU
					_lastErrorCode = (M01_Error_t)f[7]; // Uproszczenie dla kodów < 255
					return -1.0f;
				}
			} else {
				pos = 0; // Błędna suma - czyścimy i szukamy od nowa
			}
		}
	}
	return -2.0f; // Brak nowej pełnej ramki (nie jest to błąd sensu stricto, po prostu "jeszcze nic nie ma")
}

M01_Error_t M01_GetLastError(void) {
	return _lastErrorCode;
}

// Zamiana kodu na tekst (możesz spolszczyć teksty w cudzysłowach)
const char* M01_GetErrorText(M01_Error_t err_code) {
	switch(err_code) {
	case M01_ERR_NONE:
		return "OK";
	case M01_ERR_HARDWARE:
		return "Blad Sprzetu";
	case M01_ERR_DATA_OVERFLOW:
		return "Brak Danych";
	case M01_ERR_SIGNAL_WEAK:
		return "Slaby Sygnal";
	case M01_ERR_SIGNAL_STRONG:
		return "Silny Sygnal";
	case M01_ERR_TEMP_HIGH:
		return "Temp > 40 C";
	case M01_ERR_TEMP_LOW:
		return "Temp < -10 C";
	case M01_ERR_BAT_LOW:
		return "Voltage < 2.5V";
	case M01_ERR_OVER_RANGE:
		return "Poza Zasiegiem";
	case M01_ERR_READ:
		return "Blad Odczytu";
	case M01_ERR_WRITE:
		return "Blad Zapisu";
	case M01_ERR_ADDR:
		return "Blad Adresu";
	case M01_ERR_TIMEOUT:
		return "Brak Odp.";
	default:
		return "Blad Nieznany";
	}
}

void M01_LaserOn(void) {
	HAL_UART_Transmit(phuart, (uint8_t*)CMD_LASER_ON, 9, 100);
	printf("Laser ON sent\r\n");
}

void M01_LaserOff(void) {
	HAL_UART_Transmit(phuart, (uint8_t*)CMD_LASER_OFF, 9, 100);
	printf("Laser OFF sent\r\n");
}

void M01_QuickRead(void) {
// Opcjonalnie: wyczyść bufor programowy (tylko rx_head=rx_tail),
// jeśli chcesz mieć pewność, że odczytujesz świeży wynik, a nie stary zaległy.
// rx_tail = rx_head;

	HAL_UART_Transmit(phuart, (uint8_t*)CMD_QUICK, 9, 100);
	printf("Request Sent...\r\n");
}

void M01_Reset(void) {
	printf("Hardware Resetting M01...\r\n");
	enaLow();
	HAL_Delay(100);
	enaHigh();
	HAL_Delay(500); // Czas na rozruch
	_lastMeters = NAN;
}

float M01_GetLastDistance(void) {
	return _lastMeters;
}

