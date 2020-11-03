all: keyjam.c
	gcc $(shell sdl2-config --cflags) -o keyjam keyjam.c $(shell sdl2-config --libs) -lSDL2_gfx

clean:
	rm -f keyjam
