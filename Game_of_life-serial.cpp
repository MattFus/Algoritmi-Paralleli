#include <stdlib.h>
#include <unistd.h>
#include <cstdio>

#define nCols 10
#define nRows 10
#define V(R,C) ((R)*nCols+(C))

int * ReadM = new int[nRows*nCols];
int * WriteM = new int[nRows*nCols];
int nSteps = 50;

void init(){
    for(int r = 0; r< nRows; r++){
        for(int c = 0; c< nCols; c++){
            ReadM[V(r,c)] = 0;
        }
    }
    int i = nRows/2;
    int j = nCols/2;
    ReadM[V(i-1,j)] =1;
    ReadM[V(i+1,j)] =1;
    ReadM[V(i, j+1)] =1;
    ReadM[V(i+1,j+1)] =1;
    ReadM[V(i+1,j-1)] =1;
}

void print(int step){
    printf("---%d\n", step);
    for(int r = 0; r<nRows; r++){
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
    for(int r = 0; r<nRows; r++)
        for(int c = 0; c<nCols; c++)
            transFuncCell(r,c);
}

int main(int argc, char** argv){
    init();
    for(int s = 0; s<nSteps; s++){
        print(s);
        transFunc();
        
        swap();
    }
    delete [] ReadM;
    delete [] WriteM;
}