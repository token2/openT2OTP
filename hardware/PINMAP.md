# Pin map — SAM L22 (U1) reference assignment

This maps the MCU pins to nets and peripherals for the reference board. SLCD pin
numbers are illustrative — the exact COM/SEG assignment is finalised against the
chosen LCD glass datasheet and transcribed into `hal_slcd_set_digit()` in
firmware (see `../docs/PORTING.md`).

## Power

| Net | Pins | Notes |
| --- | --- | --- |
| VDD (+3V) | VDDIN, VDDIO, VDDANA | from CR2032; decouple each with 100 nF |
| GND | GND pads | star/ground pour outside antenna keep-out |

## Timekeeping

| Net | MCU pin | To |
| --- | --- | --- |
| XIN32 | PA00 | Y1 (32.768 kHz) |
| XOUT32 | PA01 | Y1 |

## Segment LCD (SLCD peripheral)

| Function | MCU pins (illustrative) | To |
| --- | --- | --- |
| COM0..COM3 | LCD_COM0..3 | DS1 backplanes (via zebra) |
| SEG0..SEG10 | LCD_SEG0..10 | DS1 segments (via zebra) |
| LCD bias | CAPL, CAPH, BIAS reservoirs | C3..C6 per datasheet |

11 SEG + 4 COM = 15 SLCD pins for a 6-digit 7-seg glass (42 segments, 1/4 duty).

## NFC front-end (U2)

| Net | MCU pin | To |
| --- | --- | --- |
| NFC_SCK | SERCOM SPI SCK | U2 SCLK |
| NFC_MOSI | SERCOM SPI MOSI | U2 MOSI |
| NFC_MISO | SERCOM SPI MISO | U2 MISO |
| NFC_CS | GPIO | U2 CS |
| NFC_IRQ | GPIO (EIC) | U2 IRQ |
| ANT1/ANT2 | — | U2 to L1 via C1/C2 tuning |

## Secure element (U3, optional)

| Net | MCU pin | To |
| --- | --- | --- |
| SE_SDA | SERCOM I2C SDA | U3 SDA (10k pull-up R1) |
| SE_SCL | SERCOM I2C SCL | U3 SCL (10k pull-up R2) |

## User + debug

| Net | MCU pin | To |
| --- | --- | --- |
| BTN | GPIO (EIC, wake) | SW1 to GND, internal pull-up |
| SWDIO | PA31 | J1 |
| SWCLK | PA30 | J1 |
| RESET | RESETN | J1 |
