PROJECT:= foo
PORT   := /dev/ttyUSB0

MODEL  := atmega328p
MAX_UPLOAD_SIZE := 30720

SRC    := $(wildcard *.cpp *.c lib/arduino/*.cpp lib/arduino/*.c lib/*.cpp lib/*.c)
OBJS   := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRC)))

CC     := avr-gcc
CPP    := avr-g++
OCPY   := avr-objcopy
SIZE   := avr-size

CFLAGS := \
    -mmcu=$(MODEL) \
		-ffunction-sections -fdata-sections -fno-exceptions \
		-funsigned-char -funsigned-bitfields \
		-fpack-struct -fshort-enums \
		-Os -Wall \
		-Ilib -Ilib/arduino -I/usr/avr/include \
		-B/usr/avr/lib/avr5/ \
		-DARDUINO=100 -DF_CPU=16000000L

all: $(PROJECT).hex

upload: $(PROJECT).hex check-size
	avrdude -V -F -c arduino -b 57600 -p $(MODEL) -P $(PORT) -U flash:w:$<

check-size:
	test `$(SIZE) --target=ihex -A $(PROJECT).hex | awk '/^Total/ {print $$2}'` -le $(MAX_UPLOAD_SIZE)

$(PROJECT).hex: $(PROJECT).elf
	$(OCPY) -O ihex -R .eeprom $< $@

$(PROJECT).elf: $(OBJS)
	$(CC) -mmcu=$(MODEL) -Os -Wl,--gc-sections -lm -B/usr/avr/lib/avr5/ -o $@ $^ -lc

%.o: %.c
	$(CC) -o $@ -c $(CFLAGS) $^
%.o: %.cpp
	$(CPP) -o $@ -c $(CFLAGS) $^

clean:
	-rm -f $(PROJECT).hex $(PROJECT).elf *.o lib/*.o lib/arduino/*.o

chat:
	chat -vsE $(PROJECT).chat

.PHONY: all clean upload
.SILENT:
