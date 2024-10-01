BUILD_DIR = build
PORT = /dev/ttyUSB0
CROSS_COMPILE = avr-

TARGET_OBJCOPY = $(CROSS_COMPILE)objcopy
TARGET_CC = $(CROSS_COMPILE)gcc
TARGET_LD = $(CROSS_COMPILE)gcc # -mmcu= doesnt work for ld?

TARGET_CFLAGS = -std=gnu99 -O3 -Wall -Wextra -Wpedantic -Wno-array-bounds -Wno-unused-function -Wno-unused-variable -mmcu=attiny1624
TARGET_LDFLAGS = -mmcu=attiny1624
TARGET_C_SOURCES = src/main.c
TARGET_C_INCLUDES =

TARGET_CFLAGS += $(addprefix -I, $(sort $(dir $(TARGET_C_INCLUDES))))

TARGET_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(TARGET_C_SOURCES:.c=.o)))
TARGET_DEP = $(TARGET_OBJECTS:%.o=%.d)

vpath %.c $(sort $(dir $(TARGET_C_SOURCES)))
vpath %.o $(BUILD_DIR)
vpath %.elf $(BUILD_DIR)

all: build.hex

run: all
	avrdude -F -V -c atmelice_updi -pt1626 -U flash:w:build.hex
#	pymcuprog ping -d attiny1626 -t uart -u $(PORT)

build:
	mkdir -p $(BUILD_DIR)

$(TARGET_OBJECTS): | build

-include $(TARGET_DEP)
$(BUILD_DIR)/%.o: %.c
	$(TARGET_CC) -MMD -c $(TARGET_CFLAGS) $< -o $@

$(BUILD_DIR)/build.elf: $(TARGET_OBJECTS)
	$(TARGET_LD) $(TARGET_LDFLAGS) $^ -o $@

build.hex: build.elf
	$(TARGET_OBJCOPY) -O ihex -R .eeprom $< $@

clean:
	rm -f build.hex
	rm -rf $(BUILD_DIR)
