# Atmega2560-baremetal-flood-leakage-detection
Bare-metal AVR-C firmware for ATmega2560 featuring real-time flood level and electrical leakage detection using sensor fusion (Ultrasonic ToF &amp; ACS712) and hardware interrupts.

# Bare-Metal Flood & Electrical Leakage Detection System

[![Microchip](https://img.shields.io/badge/MCU-ATmega2560-orange.svg)](https://www.microchip.com/)
[![Language](https://img.shields.io/badge/Language-Bare--Metal%20AVR--C-blue.svg)]()
[![Simulation](https://img.shields.io/badge/Simulation-Proteus%208.x-green.svg)](https://www.labcenter.com/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

An embedded safety and environmental monitoring system built entirely on bare-metal **AVR-C** without high-level Arduino libraries. Designed for low-latency detection of critical flood levels and electrical leakage currents, providing automated safety trips, sensor fusion, and asynchronous serial telemetries.

---

## System Highlights & Architecture

* **Hardware Target:** Microchip ATmega2560 (16 MHz)
* **Core Philosophy:** Direct Register Manipulation & Threadless ISR Architecture
* **Sensor Fusion Engine:** 
  * **Ultrasonic Time-of-Flight (ToF):** High-precision distance measurement for flood stage monitoring using Hardware Timers & Pin Change Interrupts (`PCINT0`).
  * **ACS712 Current Sensing:** Real-time RMS current computation using internal ADC interrupts (`ADC_vect`) for instant ground fault / leakage detection.
* **Safety & Actuation:** Hardware-level tripping mechanism via GPIO interrupts to prevent electrical hazards during flooding events.
* **Data Telemetry:** Non-blocking asynchronous serial data streaming via `USART3` at 9600 bps.

---

## Low-Level Technical Implementation

### Direct Register & Interrupt Control
Rather than relying on polling loops or high-overhead library calls, the firmware executes deterministic control routines using native AVR registers:

* **Timers & Input Capture:** Configured `Timer1` and `Timer2` registers for precise microsecond pulse width capture (Ultrasonic Echo processing).
* **ADC Interrupts (`ADC_vect`):** Continuous free-running/triggered analog sampling for AC/DC current waveform analysis with minimal CPU cycles.
* **Hardware Interruption:** Leveraged Pin Change Interrupts (`PCINT`) for instantaneous fault response and zero-latency safety trips.

```c
// Sample Logic Snippet: Direct Register Initialization Pattern
void system_init(void) {
    // Configure Timer/Counter for ToF Measurement
    TCCR1A = 0x00;
    TCCR1B = (1 << CS11); // Prescaler = 8 (0.5 µs resolution at 16MHz)
    
    // Enable ADC Interrupt and Select Channel
    ADCSRA |= (1 << ADIE) | (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);
    
    // Enable Global Interrupts
    sei();
}

System Block Diagram
+------------------+         +-----------------------+
  | Ultrasonic Sensor|-------> | PCINT / Timer1 Capture| ──┐
  | (Flood Depth)    |         +-----------------------+   │
  +------------------+                                     │   +------------------+
                                                           ├──>| Bare-Metal Logic |
  +------------------+         +-----------------------+   │   |   (ATmega2560)   |
  | ACS712 Current   |-------> | ADC Interrupt Engine  | ──┘   +--------+---------+
  | (Ground Leakage) |         | (ADC_vect)            |                │
  +------------------+         +-----------------------+                │
                                                                        ├─> [Safety Relay Trip]
                                                                        └─> [USART3 Telemetry]


Repository Structure
.
├── transmitter_node/       # [Board no.1: Flood, leakge detect & Serial transmitter]
│   ├── main.c              # main code (ADC, PCINT, USART Tx)
│   ├── adc.c
│   └── usart.c
│
├── receiver_node/          # [Board no.2: Serial Receiver & notify]
│   └── main.c              # main code (USART Rx ISR, Display Driver)
│  
│
├── proteus/
│   └── flood_system.pdsprj # Proteus
│
└── README.md

How to Run & Simulate
1. Prerequisites
Proteus Design Suite (v8.9 or higher)

AVR Toolchain (avr-gcc, avr-libc) OR Microchip / Atmel Studio 7.0

2. Compilation
Compile the source code to generate the target binary (.hex file):

avr-gcc -mmcu=atmega2560 -DF_CPU=16000000UL -O2 -c src/main.c -o main.o
avr-gcc -mmcu=atmega2560 main.o -o firmware.elf
avr-objcopy -O ihex -R .eeprom firmware.elf firmware.hex

3. Simulation
Open proteus/schematic.pdsprj in Proteus Design Suite.

Double-click on the ATmega2560 component.

In the Program File field, browse and select the compiled firmware.hex.

Click Play to start the interactive simulation and open the Virtual Terminal to observe USART3 telemetry.

Author
Suppawich Woraphonpaiboon

Electrical Engineering Graduate (Second Class Honours) | Kasetsart University

LinkedIn: linkedin.com/in/suppawich-woraphonpaiboon

Email: g.suppawich@gmail.com
