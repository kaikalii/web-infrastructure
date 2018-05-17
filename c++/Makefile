all: main.o
	g++ -g -std=c++11 main.o -I C:\sfml\include -L C:\sfml\lib -o a -lsfml-graphics -lsfml-window -lsfml-network -lsfml-system

main.o: main.cpp
	g++ -g -std=c++11 -c main.cpp -I C:\sfml\include -L C:\sfml\lib -lsfml-graphics -lsfml-window -lsfml-network -lsfml-system

static: main.cpp
	g++ -std=c++11 main.cpp delucad.res -D remove_console -o a -DSFML_STATIC -I C:\sfml\include -L C:\sfml\lib -lsfml-graphics-s -lsfml-window-s -lsfml-system-s -lopengl32 -lwinmm -lgdi32 -lfreetype -ljpeg -static-libgcc -static-libstdc++

clean:
	del main.o

scratch: clean all
