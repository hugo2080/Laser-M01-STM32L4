# Laser-M01-STM32L4
Integration of a high-pressure laser distance measurement module into an STM32L4 platform. Based on: https://github.com/Andres-ros/laser-m01-esp32

Laser Range Module (M01, 6-pin) with STM32L4 Family

Integrating a High-Pressure Laser Range Module with the NUCLEO-L476RG Development Board

Materials: 50m Laser Range Module from Liancheng Electronics (AliExpress).

A minimal and functional example of reading distance via UART from the M01 laser module (6-pin board) using the STM32L476RG. Keys: Q (measurement), L (laser on), K (laser off), R (module reset).

HARDWARE
• M01 laser module (6-pin board). Liancheng Electronics (Shenzhen) Co., Ltd. AliExpress Store 
• NUCLEO-L476RG (also works with STM32L433CCT6).
• 3.3V power supply (recommended minimum ~150 mA).
• Dupont cables/solder.

Module Pinout (from top to bottom):
MIN (3.3 V) – 3.3 V power supply
ENA – enable (active high state)
GND – ground
RXD – module UART input
TXD – module UART output
NC
Wire Harness Colors (according to manufacturer):
• Red = MIN (3.3 V)
• Black = GND
• Green = RXD (module)
• Yellow = TXD (module)
Note: Module TX → STM32 RX, module RX ← STM32 TX (crossed). Recommended Connection (NUCLEO-L476RG):
• Red (MIN) → 3.3 V
• Black (GND) → GND
• Yellow (TXD module) → PC5 (STM32 RX)
• Green (RXD module) ← PC4 (STM32 TX)
• ENA (if available) → PA10 HIGH (or directly to 3.3 V) If the harness has only 4 wires (red/black/green/yellow), the ENA is usually integrated and set to HIGH; connect it.

USAGE
Load the program and open PuTTY on the STM VCP serial port at 115200 baud.
Connect the laser to a matte wall (2-4 m) with a dark background.

Press:
• L → turns on the laser.
• Q → Quick read (displays X.XXX).
• K → Turns off the laser. 
• R → Resets the module (cuts off and resets the ENA). 
Note: The emitter is usually infrared; the "dot" is not visible to the naked eye. It can be seen with a cell phone camera.

Quick diagnostics (if no data is present on RX):
• Crossed wires: Yellow (module TXD) → STM32 RX; Green (module RXD) ← STM32 TX.
• Common GND between the module and the STM32.
• High ENA level (high PA10 or 3.3V voltage).
• Echo over cable (express test): Bridge green ↔ yellow on the module connector; the signal sent by the STM32 should be returned identically by RX. If there is no echo, check solder joints/continuity on the PC5/PC4 connector.
• If the device is set to 115200, change Connectivity -> USART3 -> Parameter Settings -> Baud Rate to 115200.

SAFETY
This device operates in the infrared (IR) spectrum (≈905–940 nm). Do not point it at eyes or nearby reflective surfaces.
