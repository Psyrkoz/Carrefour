#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h> 
#include "defs.h"

struct Road 
{
    struct Road* next;
    int num;
};

struct Voiture
{
    int origine;
    int routeActuelle;
    int destination;
};

struct Voiture creeVoiture()
{
    struct Voiture v;
    v.origine = rand() % 4;
    v.routeActuelle = -1;
    v.destination = rand() %4;

    return v;
}

struct Road* creeRoute()
{
    struct Road* r = malloc(4*sizeof(struct Road));
    int i;
    for(i = 0; i < 3; i++)
    {
        r[i].num = i;
        r[i].next = &r[i+1];
    }

    r[3].num = 3;
    r[3].next = &r[0];


    return r;
}

int main()
{
    struct sembuf* routesbuf;
    int shmID;

    shmID = shmget((key_t)SHM, 0, 0);
    if(shmID == -1)
    {
        perror("Impossible d'acceder a la memoire partage...\n");
        exit(1);
    }

    routesbuf = (struct sembuf*)shmat(shmID, NULL, 0);
    if(routesbuf == (struct sembuf*)-1)
    {
        perror("Impossible d'attacher le tableau en memoire partage...\n");
        exit(2);
    }

    int i = 0;
    for(i; i < NB_VAL; i++)
    {
        printf("Nombre: %d, Operation: %d,  Flag: %d\n", routesbuf[i].sem_num, routesbuf[i].sem_op, routesbuf[i].sem_flg);
    }

    struct Road* r = creeRoute();    
    while(1)
    {
        pid_t c = fork();
        if(c == 0)
        {
            srand(time(0));
            struct Voiture v = creeVoiture();
            fprintf(stderr, "%d %d %d\n", v.origine, v.destination, v.routeActuelle);
            break;
        }
        else
        {
            sleep(1);
        }
    }
}