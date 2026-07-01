# Host build for unit tests. Cross-compile the src/ files for your target
# with your SE/MCU toolchain; the core has no host dependencies beyond the HAL.
CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -Os -Iinclude
CORE     = src/sm4.c src/token2.c
TESTOBJ  = test/mock_hal.c test/test_main.c

test: $(CORE) $(TESTOBJ)
	$(CC) $(CFLAGS) $^ -o test_fw
	./test_fw

kat:
	$(CC) $(CFLAGS) src/sm4.c test/kat.c -o kat_fw && ./kat_fw

totp:
	$(CC) $(CFLAGS) src/totp.c test/totp_kat.c -o totp_fw && ./totp_fw

seed:
	$(CC) $(CFLAGS) src/sm4.c test/seed_test.c -o seed_fw && ./seed_fw

slcd:
	$(CC) $(CFLAGS) src/slcd.c test/slcd_test.c -o slcd_fw && ./slcd_fw

provision:
	$(CC) $(CFLAGS) src/sm4.c src/token2.c src/totp.c test/mock_hal.c test/provision_test.c -o provision_fw && ./provision_fw

all: test kat totp seed slcd provision

clean:
	rm -f test_fw kat_fw totp_fw seed_fw slcd_fw provision_fw

.PHONY: test kat totp seed slcd provision all clean
