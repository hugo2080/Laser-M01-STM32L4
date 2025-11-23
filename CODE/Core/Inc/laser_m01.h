/*
 * laser_m01.h
 *
 * Biblioteka do obsługi dalmierza laserowego M01 na STM32 (HAL)
 * Wykorzystuje przerwania UART i bufor kołowy.
 */

#ifndef INC_LASER_M01_H_
#define INC_LASER_M01_H_

#include "main.h" // Ważne: Dostęp do HAL i definicji pinów
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// --- Kody błędów (zgodnie z tabelą) ---
typedef enum {
    M01_ERR_NONE          = 0x0000, // Inerrancy
    M01_ERR_HARDWARE      = 0x0001, // Hardware error
    M01_ERR_DATA_OVERFLOW = 0x0002, // No output data / overflow
    M01_ERR_SIGNAL_WEAK   = 0x0003, // Signal too weak
    M01_ERR_SIGNAL_STRONG = 0x0004, // Signal too strong
    M01_ERR_TEMP_HIGH     = 0x0005, // Temp > 40 C
    M01_ERR_TEMP_LOW      = 0x0006, // Temp < -10 C
    M01_ERR_BAT_LOW       = 0x0007, // Voltage < 2.5V
    M01_ERR_OVER_RANGE    = 0x0008, // Out of range
    M01_ERR_READ          = 0x0009, // Bad reading
    M01_ERR_WRITE         = 0x000A, // Communication error
    M01_ERR_ADDR          = 0x000B, // Address error
    M01_ERR_TIMEOUT       = 0xFFFF  // (Nasz własny kod na brak odpowiedzi)
} M01_Error_t;

// --- Funkcje sterujące (API) ---

// Inicjalizacja: podaj uchwyt do UART i port/pin ENA
void M01_Init(UART_HandleTypeDef *huart_sensor, GPIO_TypeDef* ena_port, uint16_t ena_pin);

// Funkcja do wywołania w pętli głównej while(1) - mieli dane
float M01_Process(void);

// Pobiera kod ostatniego błędu (jeśli Process zwrócił -1.0f)
M01_Error_t M01_GetLastError(void);

// Zamienia kod błędu na czytelny tekst (angielski lub polski)
const char* M01_GetErrorText(M01_Error_t err_code);

// Komendy
void M01_LaserOn(void);
void M01_LaserOff(void);
void M01_QuickRead(void);
void M01_Reset(void);

// Pobranie ostatniego wyniku (jeśli potrzebujesz go użyć w logice programu)
float M01_GetLastDistance(void);

#endif /* INC_LASER_M01_H_ */
