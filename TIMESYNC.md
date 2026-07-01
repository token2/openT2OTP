# Time synchronization in openT2OTP, an example of implementation

How the token keeps time, what "sync" actually means, and how the abstract
`hal_rtc_get()` / `hal_rtc_set()` calls map to real RTC hardware.

## The model in one sentence

At provisioning the host sends one **absolute** UTC timestamp; the firmware writes
it into the RTC once, and from then on the **RTC hardware** — not the firmware —
increments that value by one every second. TOTP codes are computed from whatever
the RTC has counted up to.

## Set once, then the hardware counts

Time is a single 32-bit Unix-seconds value, reached only through two HAL calls
([`firmware/include/token2_hal.h`](../firmware/include/token2_hal.h)):

```c
uint32_t hal_rtc_get(void);            /* seconds since the UNIX epoch, UTC */
void     hal_rtc_set(uint32_t unix_seconds);
```

**Setting time** happens in the config command
([`cmd_config` in `firmware/src/token2.c`](../firmware/src/token2.c)): the host's
UTC arrives as 4 big-endian bytes in the TLV, the firmware assembles it into a
local `utc` variable, and calls `hal_rtc_set(utc)`. That is the whole write path.
The `utc` local is transient — it exists only to pass the value into the RTC; the
firmware keeps **no** timestamp of its own (there is no time field in the persisted
state struct `token2_state_t`).

**Using time** happens whenever a code is generated: the firmware calls
`hal_rtc_get()` and feeds the result to TOTP, which computes
`counter = unix_time / step` and then `HMAC(seed, counter)`
([`totp_now_algo` in `firmware/src/totp.c`](../firmware/src/totp.c)).

So the value the firmware reads is `programmed_timestamp + seconds_elapsed`, where
the elapsed part is produced entirely by the RTC ticking on its 32.768 kHz crystal.
The firmware never adds a base and an offset — it reads one already-accumulated
number.

## What "time adjustment" means

There is no incremental "adjust by N seconds" operation. Correcting drift is just
doing the absolute set again with the current true time: send the config command
with the correct UTC, `hal_rtc_set` overwrites the counter, and error starts
re-accumulating from zero.

Note the security coupling: by default (`restricted_sync = 1`) a config/time write
**also clears the seed**, so on a standard token a field time-fix is effectively a
re-provision (seed + time together). Some models (e.g. C302i) relax this to allow a
standalone time re-sync without wiping the seed — see
[`SECURITY.md`](SECURITY.md) for that trade-off.

## Where the counter actually lives

Nowhere in this repository. `hal_rtc_get` / `hal_rtc_set` are **declarations**; the
core calls them but never implements them. The body is supplied per platform:

- **Test build:** a mock in `firmware/test/mock_hal.c` stores it in a plain global
  (`g_rtc`) so tests run on a PC. It does not tick.
- **Real hardware:** the port writes an actual RTC register — either an on-chip RTC
  (e.g. a SAM L22's 32-bit `COUNT`) or an external RTC over I²C.

This is deliberate: the portable firmware stays ignorant of which RTC and which
register sit underneath.

## Concrete example: Micro Crystal RV-3028-C7 over I²C

The RV-3028-C7 is a good reference because it exposes a native **32-bit UNIX time
counter**, so `hal_rtc_set`/`get` map to a direct 4-byte write/read with no
BCD-to-epoch conversion. Its counter is driven by a digitally offset-compensated
1 Hz clock, which is what keeps long-term drift small.

Key facts (Micro Crystal RV-3028-C7 Application Manual):

- I²C address `0x52` (8-bit write address `0xA4`, read `0xA5`).
- UNIX Time counter in registers `0x1B`–`0x1E` (UNIX_TIME_0..3), **little-endian**
  (`0x1B` = bits 0–7 … `0x1E` = bits 24–31).
- The counter's increment is inhibited during an I²C write to those four registers,
  so all four bytes can be written in one transaction coherently.
- The address pointer auto-increments after each byte, so one START/STOP loads all
  four.

### Raw command

To set the clock to `0x6864F980`, write 4 bytes from register `0x1B`, LSB first:

```
START
0xA4        ; device 0x52, write
0x1B        ; register pointer = UNIX_TIME_0 (auto-increments)
0x80        ; byte 0  (bits  0-7)
0xF9        ; byte 1  (bits  8-15)
0x64        ; byte 2  (bits 16-23)
0x68        ; byte 3  (bits 24-31)
STOP
```

With Linux `i2c-tools`:

```bash
i2ctransfer -y 1 w5@0x52 0x1B 0x80 0xF9 0x64 0x68
```

### The HAL body

```c
/* RV-3028-C7: 32-bit UNIX counter at 0x1B..0x1E, little-endian, addr 0x52. */
void hal_rtc_set(uint32_t unix_seconds){
    uint8_t buf[5] = { 0x1B,                 /* start at UNIX_TIME_0 */
        (uint8_t) unix_seconds,              /* LSB first */
        (uint8_t)(unix_seconds >> 8),
        (uint8_t)(unix_seconds >> 16),
        (uint8_t)(unix_seconds >> 24) };
    i2c_write(0x52, buf, sizeof buf);        /* your platform's I2C primitive */
}

uint32_t hal_rtc_get(void){
    uint8_t reg = 0x1B, ts[4];
    i2c_write(0x52, &reg, 1);                /* set address pointer */
    i2c_read (0x52, ts, 4);                  /* read 4 bytes */
    return ((uint32_t)ts[3] << 24) | ((uint32_t)ts[2] << 16)
         | ((uint32_t)ts[1] <<  8) |  (uint32_t)ts[0];
}
```

A robust `get` reads the counter twice and retries if the two differ, to guard
against a roll-over landing mid-read.

## Caveats

- The register addresses above are specific to the **RV-3028-C7**. A different RTC
  (on-chip SAM L22 RTC, DS3231, PCF85263, …) uses the same *model* — write absolute
  seconds, let the chip count — but a different register map. Always check the part's
  datasheet.
- This file is an **illustrative reference port**, not a description of what any
  particular production device runs below the HAL line. The whole point of the HAL
  boundary is that openT2OTP does not know or care which RTC is underneath.
