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
    struct Road* next;
    int num;
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
        routes[i].next = &routes[i+1];
    }

    routes[NB_VAL - 1].num = NB_VAL - 1;
    routes[NB_VAL - 1].next = &routes[0];
}

int main()
{
    // Crée le segment de mémoire partagé et attache le tableau de routes dedans.
    int shmID = shmget((key_t)SHM, NB_VAL*sizeof(struct Road), IPC_CREAT |  S_IRUSR | S_IWUSR);
    struct Road* routes = (struct Road*)shmat(shmID, 0, NULL);

    // Crée le semaphore et l'initialise
    int mutex = creeSemaphore();
    initialiserSemaphore(mutex, 1);

    // Initalise les routes mis en mémoire partagé
    initialiserRoad(routes);

    // Juste un affichage pour être sûr que le semaphore est bien initialiser avec toutes les valeurs a 1
    int i;
    printf("Valeur du semaphore:\n");
    for(i = 0; i < NB_VAL; i++)
        printf("\tRang: %d; Valeur: %d;\n", i, semctl(mutex, i, GETVAL));
        
    return 0;
}