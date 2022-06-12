#include <stdlib.h>
#include <unistd.h>
#include <cstdio>
#include <mpich/mpi.h>
#include <mpich/mpi_proto.h>

using namespace std;

// nRows/nProc deve essere un intero
#define nCols 1200
#define nRows 1200
#define V(R,C) ((R)*nCols+(C))

int chunk;
int nSteps = 500;
int myrank;
double start;

//Inizializza Matrice
void init(int* ReadM);

//Manda Bordi ai Vicini
void exchBord(int * ReadM, int size);

//Scambia Le Matrici  (ottimizzazione)
void swap(int* &ReadM, int* &WriteM);

//Guarda il Vicinato e applica le Regole del gioco
void transFuncCell(int r, int c, int* ReadM, int* WriteM);

//Chiama transFuncCell Per Ogni Elemeneto della matrice
void transFunc(int chunk,int* ReadM, int* WriteM);

int main(int argc, char** argv){
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
        exchBord(ReadM, size);
        transFunc(chunk, ReadM, WriteM); 
        swap(ReadM, WriteM);
    }

    //END OF PARALLEL FOR

    if(myrank == 0){
        printf("Tempo: %f Secondi \n ", (MPI_Wtime() - start));
    }

    delete [] ReadM;
    delete [] WriteM;
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


// void print(int* const &ReadM){
//     for(int r = 1; r<chunk+1; r++){
//         for(int c = 0; c<nCols; c++)
//             printf("%d ",ReadM[V(r,c)]);
//         printf("\n");
//     }
// }

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