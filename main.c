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

struct sembuf* initialiserSemaphore(int mutex, int init_value, struct sembuf* routes)
{
    //struct sembuf* routes = malloc(NB_VAL*sizeof(struct sembuf));
    int i;
    for(i = 0; i < NB_VAL; i++)
    {
        semctl(mutex, i, SETVAL, init_value);
        routes[i].sem_num = i;
        routes[i].sem_op  = semctl(mutex, i, GETVAL);
        routes[i].sem_flg = 0;
    }
    
    return routes;
}

void displaySemaphore(int mutex)
{
    printf("--> Acquisition de %d mutex pour PID=%d\n", NB_VAL, getpid());

    int i = 0;
    for(i; i < NB_VAL; i++)
    {
        printf("--> Valeur %deme semaphore=%d pour PID=%d\n", i+1, semctl(mutex, i, GETVAL), getpid());
    }
}

int carrefourBloque(int mutex)
{
    int i = 0;
    while(i < NB_VAL && semctl(mutex, i, GETVAL) == 0)
        i++;

    return i != NB_VAL;
}

int main()
{
    int shmID = shmget((key_t)SHM, NB_VAL*sizeof(struct sembuf), IPC_CREAT |  S_IRUSR | S_IWUSR);
    struct sembuf* routes = (struct sembuf*)shmat(shmID, 0, NULL);

    int mutex = creeSemaphore();
    initialiserSemaphore(mutex, 0, routes);

    int i;
    for(i = 0; i < NB_VAL; i++)
        printf("Nombre: %d, Operation: %d,  Flag: %d\n", routes[i].sem_num, routes[i].sem_op, routes[i].sem_flg);
    return 0;
}