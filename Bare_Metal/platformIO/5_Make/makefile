CC = arm-none-eabi-gcc
CFLAGS = -c -mcpu=cortex-m4 -mthumb -std=gnu11
LDFLAGS = -nostdlib -T stm32_ls.ld -Wl,-Map=firmware.map -Wl,--build-id=none


final : firmware.elf

main.o : main.c
	$(CC) $(CFLAGS) $^ -o $@

startup.o : startup.c
	$(CC) $(CFLAGS) $^ -o $@

firmware.elf : main.o startup.o
	$(CC) $(LDFLAGS) $^ -o $@

flash :
	openocd -f board/st_nucleo_f4.cfg

clean :
	rm -f *.o *.elf *.map


