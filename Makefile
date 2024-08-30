rotating_mesh: src/main.c bin
	gcc src/main.c -w -lm -lSDL2 -o bin/rotating_mesh

bin:
	mkdir bin
