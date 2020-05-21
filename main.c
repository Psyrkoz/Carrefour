#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

/**
    @brief Structure d'une route contenant:
        - son numero (0, ..., NB_VAL-1)
        - la route qui suit
*/
struct Road 
{
    int next;
    int num;
    int numVoitureDessus;
};

/**
    @brief Crée le semaphore de NB_VAL (4) valeurs
    @return int - valeurs donné par semget
*/  
int creeSemaphore()
{
    return semget((key_t)SEMA, NB_VAL, IPC_CREAT | S_IRUSR | S_IWUSR);
}

/**
    @brief Initialise toute les valeurs du semaphore.
*/
void initialiserSemaphore(int mutex, int init_value)
{
    int i;
    for(i = 0; i < NB_VAL; i++)
    {
        semctl(mutex, i, SETVAL, init_value);
    }
}

/**
    @brief Crée le chaînage des routes tel que:

    -----------------
    |   1   |       |
    |   |   |  <- 0 |
    |   v   |       |
    -----------------
    |       |   ^   |
    | 2 ->  |   |   |
    |       |   3   |
    -----------------
*/
void initialiserRoad(struct Road* routes)
{
    int i;
    for(i = 0; i < NB_VAL - 1; i++)
    {
        routes[i].num  = i;
        routes[i].next = i+1;
        routes[i].numVoitureDessus = -1;
    }

    routes[NB_VAL - 1].num = NB_VAL - 1;
    routes[NB_VAL - 1].next = 0;
    routes[NB_VAL - 1].numVoitureDessus = -1;
}

int main()
{
    int i;
    // Crée le segment de mémoire partagé et attache le tableau de routes dedans.
    int shmID = shmget((key_t)SHM, NB_VAL*sizeof(struct Road), IPC_CREAT |  S_IRUSR | S_IWUSR);
    struct Road* routes = (struct Road*)shmat(shmID, 0, NULL);

    // Crée le semaphore et l'initialise
    int mutex = creeSemaphore();
    initialiserSemaphore(mutex, 1);

    // Initalise les routes mis en mémoire partagé
    initialiserRoad(routes);

    // Juste un affichage pour être sûr que le semaphore est bien initialiser avec toutes les valeurs a 1
    printf("Valeur du semaphore:\n");
    
    for(i = 0; i < NB_VAL; i++)
        printf("\tRang: %d; Valeur: %d;\n", i, semctl(mutex, i, GETVAL));

    while(1)
    {
        system("clear");
        printf("-------------------------------------------------\n");
        printf("                       |                         \n");
        printf("%23d|%23d                         \n", routes[1].numVoitureDessus, routes[0].numVoitureDessus);
        printf("                       |                         \n");
        printf("-------------------------------------------------\n");
        printf("                       |                         \n");
        printf("%23d|%23d                         \n", routes[2].numVoitureDessus, routes[3].numVoitureDessus);
        printf("                       |                         \n");
        printf("-------------------------------------------------\n");
        sleep(1);
    }
        
    return 0;
}