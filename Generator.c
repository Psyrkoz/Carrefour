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

/**
    Variable global car nécessaire pour la destruction de la memoire partagé.
*/
int shmID;

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
    @brief Structure d'une voiture contenant:
        - L'origine de la voiture (la premiere route sur laquel elle va rouler)
        - Sa route actuelle
        - Sa destination
        - Un numéro de voiture (plus utilisé mais toujours present pour debug)
*/
struct Voiture
{
    int origine;
    struct Road *routeActuelle;
    int destination;
    int numero;
};

/**
    @brief Cree une voiture
    @return Voiture v - voiture paramètré
*/
struct Voiture creeVoiture()
{
    struct Voiture v;
    v.origine = rand() % NB_VAL;
    v.routeActuelle = NULL;
    v.destination = rand() % NB_VAL;
    v.numero = rand();

    return v;
}

/**
    @brief Verifie sur le carrefour est bloqué (toute les valeurs du semaphore = 0).
    @return 1 si le carrefour est bloqué, sinon 0
*/
int carrefourBloque(int mutex)
{
    int i = 0;
    
    while(i < NB_VAL && semctl(mutex, i, GETVAL) == 0)
        i++;
    
    return i == NB_VAL;
}

/**
    @brief Fait avancer une voiture dans le carrefour
    @param [in, out] v, Voiture a faire avancer sur le carrefour
    @param [in, out] roads, les routes composant le carrefour
    @param [in] mutex, mutex du semaphore
    @return 0 si la voiture est arrivé a destination, 1 sinon 

*/
int Drive(struct Voiture *v, struct Road* roads, int mutex)
{
    int i = 0;
    fprintf(stderr, "Valeur du semaphore: %d %d %d %d\n", semctl(mutex, 0, GETVAL), semctl(mutex, 1, GETVAL), semctl(mutex, 2, GETVAL), semctl(mutex, 3, GETVAL));

    // On vérifie en premier si on peut faire avancé la voiture (dans le cas ou la voiture est déjà sur le carrefour)
    if((v->routeActuelle != NULL && carrefourBloque(mutex) == 1))
    {
        // Si la voiture est totalement bloqué dans le carrefour et qu'elle ne peut plus bougé,
        // la logique d'un conducteur serait de sortir du carrefour pour ne pas rester bloqué dans le carrefour a vie.
        fprintf(stderr, "Moi etre bloque dans carrefour mais moi partir de la route %d\n", v->routeActuelle->num);
        
        // Crée une structure buffer et remet a 1 le semaphore correspondant a la route où était la voiture
        // La route numéro 2 par exemple sera le semaphore numero 2
        struct sembuf buffer;
        buffer.sem_num = v->routeActuelle->num;
        buffer.sem_op = 1;

        semop(mutex, &buffer, 1);
        return 0;
    }
    // On vérifie ensuite si on est arrivé a destination
    else if(v->routeActuelle != NULL && (v->routeActuelle->num == v->destination))
    {
        // Si c'est le cas, on fait la même opération que précedemement.
        fprintf(stderr, "Moi etre arriver a ma destination (%d)! PogChamp\n", v->destination);
        
        struct sembuf buffer;
        buffer.sem_num = v->routeActuelle->num;
        buffer.sem_op = 1;

        semop(mutex, &buffer, 1);
        fprintf(stderr, "Et grace a moi, il y a peut etre un 1: %d %d %d %d\n", semctl(mutex, 0, GETVAL), semctl(mutex, 1, GETVAL), semctl(mutex, 2, GETVAL), semctl(mutex, 3, GETVAL));
        return 0;
    }
    // Ensuite, on verifie si la route actuelle est NULL (donc la voiture est posé sur aucune route)
    else if (v->routeActuelle == NULL)
    {
        // Si la voiture est pas sur la route, on essaye de la mettre sur la route
        // Pour ce faire on crée une structure buffer et on essaye d'appliqué -1 sur l'origine
        struct sembuf buffer;
        buffer.sem_num = v->origine;
        buffer.sem_op = -1;

        fprintf(stderr, "Moi vouloir entrer dans route %d\n", v->origine);
        semop(mutex, &buffer, 1);
        fprintf(stderr, "Moi etre entrer dans route %d\n", v->origine);

        // Une fois que l'on a réussi a passé, on met la route actuelle de la voiture = a l'origine de celle-ci
        v->routeActuelle = &roads[v->origine];
        return 1;
    }
    // Et dernièrement, c'est le cas ou la voiture est sur la route est n'est pas arrivé a destination.
    else
    {
        // Premièrement on essaye d'aller sur la route qui suit.
        struct sembuf buffer;
        buffer.sem_num = v->routeActuelle->next->num;
        buffer.sem_op = -1;

        fprintf(stderr, "Moi vouloir entrer dans route %d\n", v->routeActuelle->next->num);
        semop(mutex, &buffer, 1);
        fprintf(stderr, "Moi etre entrer dans route %d\n", v->routeActuelle->next->num);

        fprintf(stderr, "Moi vouloir partir de route %d\n", v->routeActuelle->num);

        // Une fois arrivé sur la prochaine route, on enlève la voiture de la route sur laquel il était.
        struct sembuf bufferDeux;
        bufferDeux.sem_num = v->routeActuelle->num;
        bufferDeux.sem_op = 1;

        semop(mutex, &bufferDeux, 1);
        fprintf(stderr, "Moi etre partir de route %d\n", v->routeActuelle->num);

        // Finalement on met la variable routeActuelle de la voiture = a là route qui suit.
        v->routeActuelle = v->routeActuelle->next;
        return 1;
    }
    return 1;
}

// Fonction de rappel lorsque SIGINT est attrapé (CTRL+C par exemple)
void handle_ctrl_c()
{
    printf("Suppression de la memoire partage...\n");
    // Détruit la mémoire partagé
    int ret = shmctl(shmID, IPC_RMID, NULL);
    if(ret == -1)
    {
        perror("Impossible de supprimer la memoire partagé...\n");
        exit(4);
    }
    exit(0);
}

int main()
{
    /** Crée 2 variables:
            - Le tableau de route qu'on va recupéré de la mémoire partagé
            - le mutex
    */
    struct Road* routes;
    int mutex;

    // On assigne le signal SIGINT a la fonction de rappel juste au dessus (handle_ctrl_c)
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
        exit(2);
    }

    // Récupère le tableau de route depuis la mémoire partagé
    routes = (struct Road*)shmat(shmID, NULL, 0);
    if(routes == (struct Road*)-1)
    {
        perror("Impossible d'attacher le tableau en memoire partage...\n");
        exit(3);
    } 

    // Tant qu'on a pas CTRL+C
    while(1)
    {
        // On crée un processus enfant
        pid_t c = fork();

        // Si le processus actuelle est l'enfant
        if(c == 0)
        {
            // L'enfant lance lui même srand(...) car sinon, toute les valeurs généré seront les mêmes ET dans le même ordre pour les enfants 
            srand(time(0));

            // On crée la voiture de l'enfant
            struct Voiture v = creeVoiture();

            // Tant qu'on est entrain de rouler (l'opération est faite dans Drive)...
            while(Drive(&v, routes, mutex) == 1)
            {
                // ... on fait une pause de 3 secondes entre chaque changement de routes pour simuler le traffic, 
                // sinon les voitures irons tellement vite qu'il n'y aurait qu'une seul voiture sur le routes tout le temps
                sleep(3);
            }
            exit(0);
        }
        // Si le processus actuelle est le père
        else
        {
            // On attends entre 5 et 10 secondes avant de crée une nouvelle voiture
            sleep((rand()% 5) + 5);
        }
    }
    return 0;
}