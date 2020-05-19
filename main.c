#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

struct Road 
{
    struct Road* next;
    int num;
};

int creeSemaphore()
{
    return semget((key_t)SEMA, NB_VAL, IPC_CREAT | S_IRUSR | S_IWUSR);
}

void initialiserSemaphore(int mutex, int init_value)
{
    int i;
    for(i = 0; i < NB_VAL; i++)
    {
        semctl(mutex, i, SETVAL, init_value);
    }
}

void initialiserRoad(struct Road* routes)
{
    int i;
    for(i = 0; i < NB_VAL - 1; i++)
    {
        routes[i].num  = i;
        routes[i].next = &routes[i+1];
    }

    routes[NB_VAL - 1].num = NB_VAL - 1;
    routes[NB_VAL - 1].next = &routes[0];
}

int main()
{
    int shmID = shmget((key_t)SHM, NB_VAL*sizeof(struct Road), IPC_CREAT |  S_IRUSR | S_IWUSR);
    struct Road* routes = (struct Road*)shmat(shmID, 0, NULL);

    int mutex = creeSemaphore();
    initialiserSemaphore(mutex, 1);
    initialiserRoad(routes);

    int i;
    printf("Valeur du semaphore:\n");
    for(i = 0; i < NB_VAL; i++)
        printf("\tRang: %d; Valeur: %d;\n", i, semctl(mutex, i, GETVAL));
        
    return 0;
}