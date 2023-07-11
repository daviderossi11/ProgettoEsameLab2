#ifndef BUFFER_H
#define BUFFER_H
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include "xerrori.h"


#define PC_buffer_len 10

typedef struct {
    char **buffer;                      // Array di puntatori a stringhe
    int read_index;                     // Indice di lettura
    int write_index;                    // Indice di scrittura
    int count;                          // Numero di elementi nel buffer
    pthread_mutex_t lock;               // Mutex per la sincronizzazione
    pthread_cond_t buffer_full;     // Condition variable per il buffer pieno
    pthread_cond_t buffer_empty;    // Condition variable per il buffer vuoto
} bounded_buffer_t;

// Funzione per l'inizializzazione del buffer
int buffer_init(bounded_buffer_t *buffer);

// Funzione per la distruzione del buffer
int buffer_destroy(bounded_buffer_t *buffer);

// Funzione per l'inserimento di una stringa nel buffer
int buffer_push(bounded_buffer_t *buffer, char *string);

// Funzione per la rimozione di una stringa dal buffer
char *buffer_pop(bounded_buffer_t *buffer);

#endif /* BUFFER_H */
