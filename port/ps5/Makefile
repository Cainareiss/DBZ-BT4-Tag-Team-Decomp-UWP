PS5_HOST ?= ps5
PS5_PORT ?= 9021

ifneq ($(filter check host-check clean,$(MAKECMDGOALS)),)
else ifdef PS5_PAYLOAD_SDK
include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
$(error PS5_PAYLOAD_SDK is undefined; use 'make check' for host-only checks)
endif

BUILD := build
ELF := $(BUILD)/bt4_ps5_probe.elf
HOST_SMOKE := $(BUILD)/host_smoke
SOURCES := src/main.c src/log.c src/memory.c src/storage.c
CFLAGS := -std=c11 -O2 -g -Wall -Wextra -Werror -Iinclude

.PHONY: all check host-check clean test

all: check $(ELF)

check:
	python3 tools/check_layout.py
	python3 tools/check_upstream.py

host-check: check $(HOST_SMOKE)
	$${CC:-cc} -D_GNU_SOURCE -std=c11 -Wall -Wextra -Werror -Iinclude \
		-fsyntax-only $(SOURCES)
	$(HOST_SMOKE)

$(HOST_SMOKE): src/memory.c src/storage.c tools/host_smoke.c \
		include/bt4_ps5.h include/bt4_ps5_memory_layout.h
	mkdir -p $(BUILD)
	$${CC:-cc} -D_GNU_SOURCE -std=c11 -Wall -Wextra -Werror -Iinclude \
		-o $@ src/memory.c src/storage.c tools/host_smoke.c

$(ELF): $(SOURCES) include/bt4_ps5.h include/bt4_ps5_memory_layout.h
	mkdir -p $(BUILD)
	$(CC) $(CFLAGS) -o $@ $(SOURCES)

test: $(ELF)
	$(PS5_DEPLOY) -h $(PS5_HOST) -p $(PS5_PORT) $<

clean:
	rm -rf $(BUILD)
