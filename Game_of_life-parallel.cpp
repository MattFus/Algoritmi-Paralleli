#include <stdlib.h>
#include <unistd.h>
#include <cstdio>
#include <mpich/mpi.h>
#include <fstream>
#include <iostream>
using namespace std;

#define nCols 10
#define nRows 10
#define V(R,C) ((R)*nCols+(C))

int * ReadM = new int[((nRows/2)+2)*nCols];
int * WriteM = new int[((nRows/2)+2)*nCols];
int nSteps = 5;
int myrank;
double start;


void init(){
    for(int r = 1; r< (nRows/2)+1; r++){
        for(int c = 1; c< nCols; c++){
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

void exchBord(){
    MPI_Request req;
    MPI_Status status;
    if(myrank == 0){
        MPI_Send(&ReadM[V(1,0)], nCols, MPI_INT, 1, 0, MPI_COMM_WORLD);
        MPI_Send(&ReadM[V((nRows/2),0)], nCols, MPI_INT, 1, 1, MPI_COMM_WORLD);
        MPI_Recv(&ReadM[V(0,0)], nCols, MPI_INT, 1, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&ReadM[V((nRows/2)+1,0)], nCols, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
    }

    if(myrank == 1){
        MPI_Isend(&ReadM[V(1,0)], nCols, MPI_INT, 0, 0, MPI_COMM_WORLD, &req);
        MPI_Isend(&ReadM[V((nRows/2),0)], nCols, MPI_INT, 0, 1, MPI_COMM_WORLD, &req);
        MPI_Recv(&ReadM[V(0,0)], nCols, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&ReadM[V((nRows/2)+1,0)], nCols, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    }
}

 void print(int step){
    for(int r = 1; r<nRows/2+1; r++){
        for(int c = 0; c<nCols; c++)
            printf("%d ",ReadM[V(r,c)]);
        printf("\n");
    }
}

void swap(){
    int * tmp = ReadM;
    ReadM = WriteM;
    WriteM = tmp;
}

int contaVicini(int r, int c){
    int i, j, cont = 0;
    for(i = r-1; i< r+1; i++){
        for(j = c-1; j< c+1; j++){
            if((i==r && j==c) || (i<0 || j<0) || (i>=nRows || j>=nCols)){
                continue;
            }
            else if(ReadM[V(i,j)] == 1){
                cont += 1;
            }
        }
    }
    return cont;
}

void transFuncCell(int r, int c){
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

void transFunc(){
    for(int r = 1; r<(nRows/2)+1; r++)
        for(int c = 0; c<nCols; c++)
            transFuncCell(r,c);
}

int main(int argc, char** argv){
    std::fstream output ("output.txt", fstream::app);
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    
    init();
    for(int s = 0; s<nSteps; s++){
        exchBord();
        if(myrank == 0)
            print(s);
        else {
            sleep(1);
            print(s);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        printf("\n");
        transFunc();
        for(int r = 1; r<nRows/2+1; r++){
            for(int c = 0; c<nCols; c++)
                output << ReadM[V(r,c)];
            output << "sono il thread: " << myrank<< "\n";
        }
        MPI_Barrier(MPI_COMM_WORLD);
        output << endl;
        swap();
    }
    MPI_Barrier(MPI_COMM_WORLD);
    if(myrank == 0){
        printf("tempo:%f", MPI_Wtime() - start);
    }
    MPI_Finalize();
    delete [] ReadM;
    delete [] WriteM;

}
