GCC_ARGS = -Wall -Werror

all: build/xmutil.o build/xmodem.o
	gcc -g -o xmutil build/xmodem.o build/xmutil.o $(GCC_ARGS)

build/xmutil.o: src/xmutil.c
	gcc -g -c src/xmutil.c -o build/xmutil.o $(GCC_ARGS)

build/xmodem.o: src/xmodem.c
	gcc -g -c src/xmodem.c -o build/xmodem.o $(GCC_ARGS)

.PHONY: clean

clean:
	rm build/*.o
	rm xmutil
