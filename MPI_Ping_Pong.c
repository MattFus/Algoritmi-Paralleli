#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv){
    int ping;
    int pong;
    int vai = 1;

    MPI_Init(&argc,&argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    while(vai){
        if(rank == 0){
            ping = 1;
            MPI_Send(&ping,1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(&pong,1, MPI_INT, 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            printf("Processor 0 got: \n" + "ping: %d\n" + "pong: %d", ping, pong);
        }
        else if(rank == 1){
            pong = 2;
            MPI_Recv(&ping,1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(&pong,1, MPI_INT, 0, 1, MPI_COMM_WORLD);
            
            printf("Processor 1 got: \n" + "ping: %d\n" + "pong: %d", ping, pong);
        }
        sleep(2);
    }
}