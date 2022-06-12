#include <stdlib.h>
#include <unistd.h>
#include <cstdio>
#include <mpich/mpi.h>
#include <fstream>
#include <iostream>
#include <string>
#include <allegro5/allegro.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>



using namespace std;

// nRows/nProc deve essere un intero
#define nCols 80
#define nRows 80
#define V(R,C) ((R)*nCols+(C))

int chunk;
int nSteps = 200;
int myrank;
double start;

//Inizializza Matrice
void init(int* ReadM);

//Manda Bordi ai Vicini
void exchBord(int * ReadM, int size);

//stampa in output la matrice
void print(int*  const &ReadM);

//Scambia Le Matrici  (ottimizzazione)
void swap(int* &ReadM, int* &WriteM);

//Guarda il Vicinato e applica le Regole del gioco
void transFuncCell(int r, int c, int* ReadM, int* WriteM);

//Chiama transFuncCell Per Ogni Elemeneto della matrice
void transFunc(int chunk,int* ReadM, int* WriteM);

//Salva i vari step sul file output.txt 

void Salva(int const& chunk, int* const& ReadM, int const& size,fstream& output)
{
    for(int i = 0; i< size; i++){
        if(myrank == i)
            for(int r = 1; r<=chunk; r++){
                for(int c = 0; c<nCols; c++){
                    output << ReadM[V(r,c)];
                }
                output<<endl;
            }
        MPI_Barrier(MPI_COMM_WORLD);
    }
}


//Legge il file e usa disegna
void leggiDaFile();

//Allegro
void disegna(int mappa[nRows][nCols]);


int main(int argc, char** argv){
    fstream output ("output.txt" , fstream::app);
    int size;
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if(myrank == 0)
        start = MPI_Wtime();

    chunk = nRows/size;
    int * ReadM = new int[((chunk)+2)*nCols];
    int * WriteM = new int[((chunk)+2)*nCols];
    init(ReadM);
    for(int s = 0; s<nSteps; s++){
        if(myrank == 0)
            output<< "Anno:" << s << endl;
        exchBord(ReadM, size);
        transFunc(chunk, ReadM, WriteM);
        Salva(chunk, ReadM, size,output);
        swap(ReadM, WriteM);
    }

    //END OF PARALLEL FOR

    if(myrank == 0){
        printf("Tempo: %f Secondi \n ", (MPI_Wtime() - start));
    }
    output.close();


    delete [] ReadM;
    delete [] WriteM;


    /** ALLEGRO*/
    
    if(myrank==0) {
        ALLEGRO_DISPLAY *display = nullptr;
        ALLEGRO_EVENT ev;
        al_init();
        al_init_primitives_addon();
        display = al_create_display(320, 320);
        leggiDaFile();
        al_destroy_display(display);

    }
    
    MPI_Finalize();


}








/** FUNZIONI PER MPI*/

void init(int* ReadM){
    for(int r = 1; r<= chunk; r++){
        for(int c = 0; c< nCols; c++){
            ReadM[V(r,c)] = 0;
        }
    }

    if(myrank == 0){
        int i = 3;
        int j = nCols/2;
        ReadM[V(i-1,j)] =1;
        ReadM[V(i+1,j)] =1;
        ReadM[V(i, j+1)] =1;
        ReadM[V(i+1,j+1)] =1;
        ReadM[V(i+1,j-1)] =1;

    }

}

void exchBord(int * ReadM, int size){
    MPI_Request req;
    MPI_Status status;
    if(myrank == 0){
        MPI_Isend(&ReadM[V(1,0)], nCols, MPI_INT, size-1, 0, MPI_COMM_WORLD, &req);
        MPI_Isend(&ReadM[V((chunk),0)], nCols, MPI_INT, myrank+1, 1, MPI_COMM_WORLD, &req);
        MPI_Recv(&ReadM[V(0,0)], nCols, MPI_INT, size-1, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&ReadM[V((chunk)+1,0)], nCols, MPI_INT, myrank+1, 0, MPI_COMM_WORLD, &status);
    }

    else if(myrank == size-1){

        MPI_Isend(&ReadM[V(1,0)], nCols, MPI_INT, myrank-1, 0, MPI_COMM_WORLD, &req);
        MPI_Isend(&ReadM[V((chunk),0)], nCols, MPI_INT, 0, 1, MPI_COMM_WORLD, &req);
        MPI_Recv(&ReadM[V(0,0)], nCols, MPI_INT, myrank-1, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&ReadM[V((chunk)+1,0)], nCols, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    }
    else if(myrank != 0 && myrank != size-1){
        MPI_Isend(&ReadM[V(1,0)], nCols, MPI_INT, myrank-1, 0, MPI_COMM_WORLD, &req);
        MPI_Isend(&ReadM[V((chunk),0)], nCols, MPI_INT, myrank+1, 1, MPI_COMM_WORLD, &req);
        MPI_Recv(&ReadM[V(0,0)], nCols, MPI_INT, myrank-1, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&ReadM[V((chunk)+1,0)], nCols, MPI_INT, myrank+1, 0, MPI_COMM_WORLD, &status);
    }
}


void print(int* const &ReadM){
    for(int r = 1; r<chunk+1; r++){
        for(int c = 0; c<nCols; c++)
            printf("%d ",ReadM[V(r,c)]);
        printf("\n");
    }
}

void swap(int* &ReadM, int* &WriteM){
    int * tmp = ReadM;
    ReadM = WriteM;
    WriteM = tmp;
}

void transFuncCell(int r, int c, int* ReadM, int* WriteM){
    //formule di transizione per bordi:
    // (j+nCols)%nCols sia per pos che per neg
    // solo per pos -> j%nCols
    //solo per neg -> j+nCols
    int cont = 0;
    for(int i = -1; i<2; i++)
        for(int j = -1; j<2; j++){
            if((i!= 0 || j !=0) && ReadM[V((r+i+nRows)%nRows,(c+j+nCols)%nCols)]){
                cont++;
            }
            if(ReadM[V(r,c)] == 1){
                if(cont == 2 || cont == 3)
                    WriteM[V(r,c)] = 1;
                else WriteM[V(r,c)] = 0;
            }
            else{
                if(cont == 3){
                    WriteM[V(r,c)] = 1;
                }
                else WriteM[V(r,c)] = 0;
            }
        }
}

void transFunc(int chunk,int* ReadM, int* WriteM){
    for(int r = 1; r<= chunk; r++)
        for(int c = 0; c<nCols; c++)
            transFuncCell(r,c,ReadM,WriteM);
}


/**FUNZIONI PER ALLEGRO */

void leggiDaFile()
{
    fstream input("output.txt");
    string temp;

    int stampa[nRows][nCols];
    for (int s = 0; s < nSteps; ++s) {
        input>>temp;
       /* for (int k = 0; k < temp.size(); ++k) {
            cout<<temp[k];
        }
        cout<<endl;*/
        for (int i = 0; i < nRows; ++i) {
            input >> temp;
            for (int j = 0; j < temp.size(); j++)
                stampa[i][j]=temp[j]-'0';
        }

        /* stampa matrice per verificare
        for (int i = 0; i < nRows; ++i) {
            for (int j = 0; j < temp.size(); j++)
                cout<<stampa[i][j];
            cout<<endl;
        }
        cout<<endl;*/

        disegna(stampa);


    }


}


void disegna(int mappa[nRows][nCols])
{
    ALLEGRO_TIMER* timer = al_create_timer(0.1);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_start_timer(timer);
    bool close = false;
   // while (!close)

   // {
        ALLEGRO_EVENT event;

        al_wait_for_event(queue, &event);
        if (event.type == ALLEGRO_EVENT_TIMER)
        {

            for (unsigned i = 0; i < nRows; i++)
            {
                for (unsigned j = 0; j < nCols; j++)
                {
                    if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE){
                        cout<<"cazzo";

                    }
                    if (mappa[i][j] == 1)
                    {
                        al_draw_filled_rectangle(j * 4, i * 4, j * 4 + 4, i * 4 + 4, al_map_rgb(0, 0, 0));
                    }
                    if (mappa[i][j] == 0)
                    {
                        al_draw_filled_rectangle(j * 4, i * 4, j * 4 + 4, i * 4 + 4, al_map_rgb(255, 255, 255));
                    }
                }
            }
            al_flip_display();
        }

        close = true;
   // }

    al_destroy_timer(timer);
    al_destroy_event_queue(queue);
    
}
