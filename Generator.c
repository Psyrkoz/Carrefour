#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h> 
#include "defs.h"

int shmID;

struct Road 
{
    struct Road* next;
    int num;
};

struct Voiture
{
    int origine;
    struct Road *routeActuelle;
    int destination;
    int numero;
};

struct Voiture creeVoiture()
{
    struct Voiture v;
    v.origine = rand() % NB_VAL;
    v.routeActuelle = NULL;
    v.destination = rand() % NB_VAL;
    v.numero = rand();

    return v;
}

int carrefourBloque(int mutex)
{
    int i = 0;
    
    while(i < NB_VAL && semctl(mutex, i, GETVAL) == 0)
        i++;
    
    return i == NB_VAL;
}

int Drive(struct Voiture *v, struct Road* roads, int mutex)
{
    int i = 0;
    fprintf(stderr, "Valeur du semaphore: %d %d %d %d\n", semctl(mutex, 0, GETVAL), semctl(mutex, 1, GETVAL), semctl(mutex, 2, GETVAL), semctl(mutex, 3, GETVAL));

    if((v->routeActuelle != NULL && carrefourBloque(mutex) == 1))
    {
        fprintf(stderr, "Moi etre bloque dans carrefour mais moi partir de la route %d\n", v->routeActuelle->num);
        
        struct sembuf buffer;
        buffer.sem_num = v->routeActuelle->num;
        buffer.sem_op = 1;

        semop(mutex, &buffer, 1);
        return 0;
    }
    else if(v->routeActuelle != NULL && (v->routeActuelle->num == v->destination))
    {
        fprintf(stderr, "Moi etre arriver a ma destination (%d)! PogChamp\n", v->destination);
        
        struct sembuf buffer;
        buffer.sem_num = v->routeActuelle->num;
        buffer.sem_op = 1;

        semop(mutex, &buffer, 1);
        fprintf(stderr, "Et grace a moid, y'a peut etre un 1: %d %d %d %d\n", semctl(mutex, 0, GETVAL), semctl(mutex, 1, GETVAL), semctl(mutex, 2, GETVAL), semctl(mutex, 3, GETVAL));
        return 0;
    }
    else if (v->routeActuelle == NULL) // Salut, j'suis pas encore sur le crossroad du coups j'me rend sur l'origine
    {
        struct sembuf buffer;
        buffer.sem_num = v->origine;
        buffer.sem_op = -1;

        fprintf(stderr, "Moi vouloir entrer dans route %d\n", v->origine);
        semop(mutex, &buffer, 1);
        fprintf(stderr, "Moi etre entrer dans route %d\n", v->origine);
        v->routeActuelle = &roads[v->origine];
        return 1;
    }
    else
    {
        struct sembuf buffer;
        buffer.sem_num = v->routeActuelle->next->num;
        buffer.sem_op = -1;

        fprintf(stderr, "Moi vouloir entrer dans route %d\n", v->routeActuelle->next->num);
        semop(mutex, &buffer, 1);
        fprintf(stderr, "Moi etre entrer dans route %d\n", v->routeActuelle->next->num);

        fprintf(stderr, "Moi vouloir partir de route %d\n", v->routeActuelle->num);


        struct sembuf bufferDeux;
        bufferDeux.sem_num = v->routeActuelle->num;
        bufferDeux.sem_op = 1;

        semop(mutex, &bufferDeux, 1);
        fprintf(stderr, "Moi etre partir de route %d\n", v->routeActuelle->num);
        v->routeActuelle = v->routeActuelle->next;
        return 1;
    }
    return 1;
}

void handle_ctrl_c()
{
    printf("Suppression de la memoire partage...\n");
    shmctl(shmID, IPC_RMID, NULL);
    exit(0);
}

int main()
{
    struct Road* routes;
    int mutex;

    signal(SIGINT, handle_ctrl_c);

    // Récupère le shm id
    shmID = shmget((key_t)SHM, 0, 0);
    if(shmID == -1)
    {
        perror("Impossible d'acceder a la memoire partage...\n");
        exit(1);
    }

    // Récupère le semid
    mutex = semget((key_t)SEMA, 0, 0);
    if(mutex == -1)
    {
        fprintf(stderr, "Impossible de recuperer le mutex\n");
    }

    // Récupère le buffer depuis la mémoire partagé
    routes = (struct Road*)shmat(shmID, NULL, 0);
    if(routes == (struct Road*)-1)
    {
        perror("Impossible d'attacher le tableau en memoire partage...\n");
        exit(2);
    } 

    while(1)
    {
        pid_t c = fork();
        if(c == 0)
        {
            srand(time(0));
            struct Voiture v = creeVoiture();
            while(Drive(&v, routes, mutex) == 1)
            {
                sleep(3);
            }
            exit(0);
        }
        else
        {
            sleep((rand()% 5) + 5);
        }
    }
    return 0;
}