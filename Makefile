CC ?= cc
CFLAGS ?= -Wall -Wextra -O2 -std=c11
SRC_DIR = src
BUILD_DIR = build
PREFIX ?= /usr/local

COMMON = $(SRC_DIR)/json.c $(SRC_DIR)/tar.c $(SRC_DIR)/manifest.c
HEADERS = $(wildcard $(SRC_DIR)/*.h)

.PHONY: all clean install uninstall

all: $(BUILD_DIR)/kaya $(BUILD_DIR)/mkkay

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/kaya: $(SRC_DIR)/kaya.c $(COMMON) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/kaya.c $(COMMON)

$(BUILD_DIR)/mkkay: $(SRC_DIR)/mkkay.c $(COMMON) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/mkkay.c $(COMMON)

install: all
	mkdir -p $(PREFIX)/bin
	cp $(BUILD_DIR)/kaya $(PREFIX)/bin/kaya
	cp $(BUILD_DIR)/mkkay $(PREFIX)/bin/mkkay
	@echo "Kuruldu: $(PREFIX)/bin/kaya, $(PREFIX)/bin/mkkay"

uninstall:
	rm -f $(PREFIX)/bin/kaya $(PREFIX)/bin/mkkay

clean:
	rm -rf $(BUILD_DIR)
