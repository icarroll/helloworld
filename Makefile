dots: dots.cpp
	g++ dots.cpp -I/mingw64/include/SDL2 -I /mingw64/include/cairo -L/mingw64/lib -Wl,-subsystem,windows -lmingw32 -lSDL2main -lSDL2 -lcairo -mwindows -o dots.exe
