
output: TerminalWm.o
	gcc TerminalWm.o -g -o LukeWm -lX11 -I /usr/include/freetype2 -lXft

TerminalWm.o:
	gcc -c TerminalWm.c -g -lX11 -I /usr/include/freetype2 -lXft

clean:
	rm /usr/local/bin/LukeWm

install:
	cp LukeWm /usr/local/bin
