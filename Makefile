
MCU=attiny85
F_CPU=1000000
GIT_VERSION=$(shell git describe --abbrev=4 --always --tags)
CFLAGS=-g -Wall -mmcu=$(MCU) -DF_CPU=$(F_CPU)UL -DVERSION=\"$(GIT_VERSION)\" -Os
LDFLAGS=-Wl,-gc-sections -Wl,-relax
CC=avr-gcc
TARGET=odometer
OBJ=odometer.o

all: $(TARGET).hex

%.hex: %.obj
	avr-size --mcu=$(MCU) --format=avr $<
#	avr-objdump -h -d -S -z $< > $(TARGET).lss
	avr-objcopy -R .eeprom -O ihex $< $@

%.obj: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $@

clean:
	rm -f *.o *.lss *.hex *.obj *.hex

fuse:
	# eeprom preserve, BOD at 1.8 volt, 8 MHz oscillator prescaled by 8
	avrdude -q -P usb -c avrispmkII -p $(MCU) -U efuse:w:0b11111111:m -U hfuse:w:0b11010110:m -U lfuse:w:0b01100010:m

eeprom:
	# read EEPROM to stdout
	avrdude -q -P usb -c avrispmkII -p $(MCU) -U eeprom:r:-:d

flash: $(TARGET).hex
	avrdude -q -F -P usb -c avrispmkII -p $(MCU) -U flash:w:$<

