Compile with: mpiCC main.cpp $(pkg-config allegro-5 allegro_font-5 allegro_primitives-5 --libs --cflags)
Run with:     mpirun -np N ./a.out