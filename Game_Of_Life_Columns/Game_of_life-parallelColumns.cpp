#include <stdlib.h>
#include <unistd.h>
#include <cstdio>
#include <mpich/mpi.h>
#include <fstream>
#include <iostream>
using namespace std;

#define nCols 50
#define nRows 50
// old macro -> #define V(R,C) ((R)*nCols+(C))
//new macro for column divided matrix
#define V(R,C) ((R)*((chunk)+2)+(C))

int chunk;
int nProcs;
int nSteps = 10;
int myrank;
double start;
MPI_Datatype newT;


void init(int* ReadM); //Inizializza la matrice

void exchBord(int* ReadM); //Invia e riceve i dati sul bordo

void swap(int* &ReadM, int* &WriteM); //Scambia le matrici

void transFuncCell(int r, int c, int* ReadM, int* WriteM); //Funzione che calcola la transizione di una cella

void transFunc(int chunk, int* ReadM, int* WriteM); //Funzione che calcola la transizione di un chunk

int main(int argc, char** argv){
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcs);
    chunk = nCols/nProcs;
    int * ReadM = new int[((chunk)+2)*nRows];
    int * WriteM = new int[((chunk)+2)*nRows];
    //In an column divided matrix:
    //blockcount = nRows
    //blocklength = 1
    //stride = nCols/2 + 2
    //Il bordo da copiare ad EST -> nCols/2
    //Il bordo da copiare a WEST -> 1
    //Halocell EST -> nCols/2+1
    //Halocell WEST -> 0
    //MPI_Type_vector(int Blockcount, int blocksize, int stride, MPI_Datatype OLD, MPI_Datatype *NEW)
    //definiamo il nuovo datatype:

    MPI_Type_vector(nRows, 1, (chunk)+2, MPI_INT, &newT);
    MPI_Type_commit(&newT);

    if(myrank == 0) //start timer
        start = MPI_Wtime();

    init(ReadM);
    for(int s = 0; s<nSteps; s++){
        exchBord(ReadM);
        MPI_Barrier(MPI_COMM_WORLD);
        transFunc(chunk, ReadM, WriteM);
        swap(ReadM, WriteM);
    }

    MPI_Type_free(&newT);

    if(myrank == 0){
        printf("Tempo: %f Secondi \n ", (MPI_Wtime() - start));
    }

    delete [] ReadM;
    delete [] WriteM;
    MPI_Finalize();
}

void init(int* ReadM){
    for(int r = 0; r< nRows; r++){
        for(int c = 0; c< (chunk)+2; c++){
            ReadM[V(r,c)] = 0;
        }
    }

    if(myrank == 0){
    int i = nRows/2;
    int j = 3;
    ReadM[V(i-1,j)] =1;
    ReadM[V(i+1,j)] =1;
    ReadM[V(i, j+1)] =1;
    ReadM[V(i+1,j+1)] =1;
    ReadM[V(i+1,j-1)] =1;
    }

}

void exchBord(int* ReadM){
    MPI_Request req;
    MPI_Status status;
    //SPEDISCO VERSO EST -> TAG 10
    //RICEVO DA EST -> TAG 11
    //SPEDISCO VERSO WEST -> TAG 11
    //RICEVO DA WEST -> TAG 10
    //LA SIZE è 1 PERCHé USIAMO IL NUOVO DATATYPE
    if(myrank == 0){
        MPI_Isend(&ReadM[V(0,chunk)], 1, newT, myrank+1, 10, MPI_COMM_WORLD, &req);
        MPI_Isend(&ReadM[V(0,1)], 1, newT, nProcs-1, 11, MPI_COMM_WORLD, &req);
        MPI_Recv(&ReadM[V(0,chunk+1)], 1, newT, 1, 11, MPI_COMM_WORLD, &status);
        MPI_Recv(&ReadM[V(0,0)], 1, newT, nProcs-1, 10, MPI_COMM_WORLD, &status);
    }

    else if(myrank == nProcs-1){
        MPI_Isend(&ReadM[V(0,chunk)], 1, newT, 0, 10, MPI_COMM_WORLD, &req);
        MPI_Isend(&ReadM[V(0,1)], 1, newT, myrank-1, 11, MPI_COMM_WORLD, &req);
        MPI_Recv(&ReadM[V(0,chunk+1)], 1, newT, 0, 11, MPI_COMM_WORLD, &status);
        MPI_Recv(&ReadM[V(0,0)], 1, newT, myrank-1, 10, MPI_COMM_WORLD, &status);
    }
    else if(myrank != 0 && myrank != nProcs-1){
        MPI_Isend(&ReadM[V(0,chunk)], 1, newT, myrank+1, 10, MPI_COMM_WORLD, &req);
        MPI_Isend(&ReadM[V(0,1)], 1, newT, myrank-1, 11, MPI_COMM_WORLD, &req);
        MPI_Recv(&ReadM[V(0,chunk+1)], 1, newT, myrank+1, 11, MPI_COMM_WORLD, &status);
        MPI_Recv(&ReadM[V(0,0)], 1, newT, myrank-1, 10, MPI_COMM_WORLD, &status);
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
            if((i!= 0 || j !=0) && ReadM[V((r+i+nRows)%nRows,(c+j+nCols)%nCols)])
                cont++;
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

void transFunc(int chunk, int* ReadM, int* WriteM){
    for(int r = 0; r<nRows; r++)
        for(int c = 1; c<(chunk)+1; c++)
            transFuncCell(r,c, ReadM, WriteM);
}
