#include <string.h>
#include "buffer.h"
#include "xerrori.h"

#define HERE __LINE__, __FILE__


extern char* strdup(const char*);

// Inizializza il buffer 
int buffer_init(bounded_buffer_t *buffer)
{
    // Alloca memoria per il buffer
    buffer->buffer = (char **)calloc(PC_buffer_len, sizeof(char *));
    if (buffer->buffer == NULL)
        xtermina("Errore nell'allocazione della memoria",HERE);

    buffer->read_index = 0;
    buffer->write_index = 0;
    buffer->count = 0;

    // Inizializza il mutex
    if (xpthread_mutex_init(&(buffer->lock), NULL, HERE) != 0)
    {
        free(buffer->buffer);
        xtermina("Errore nell'inizializzazione del mutex", HERE);
    }

    // Inizializza la condition variable buffer_full
    if (xpthread_cond_init(&(buffer->buffer_full), NULL, HERE) != 0)
    {
        xpthread_mutex_destroy(&(buffer->lock), HERE);
        free(buffer->buffer);
        xtermina("Errore nell'inizializzazione della condition variable buffer_full", HERE);
    }

    // Inizializza la condition variable buffer_empty
    if (xpthread_cond_init(&(buffer->buffer_empty), NULL, HERE) != 0)
    {
        xpthread_cond_destroy(&(buffer->buffer_full), HERE);
        xpthread_mutex_destroy(&(buffer->lock), HERE);
        free(buffer->buffer);
        xtermina("Errore nell'inizializzazione della condition variable buffer_empty", HERE);
    }

    return 0;  
}

// Distrugge il buffer circolare
int buffer_destroy(bounded_buffer_t *buffer)
{
    // Dealloca le stringhe nel buffer
    for (int i = 0; i < PC_buffer_len; i++)
        free(buffer->buffer[i]);

    free(buffer->buffer);

    // Distrugge le variabili di condizione e il mutex
    xpthread_mutex_destroy(&(buffer->lock), HERE);

    xpthread_cond_destroy(&(buffer->buffer_full), HERE);

    xpthread_cond_destroy(&(buffer->buffer_empty), HERE);

    return 0; 
}

// Aggiunge una stringa al buffer circolare
int buffer_push(bounded_buffer_t *buffer, char *string)
{
    xpthread_mutex_lock(&(buffer->lock), HERE);

    // Attende finché il buffer è pieno
    while (buffer->count == PC_buffer_len)
        xpthread_cond_wait(&(buffer->buffer_full), &(buffer->lock), HERE);

    // Copia la stringa nel buffer
    buffer->buffer[buffer->write_index] = strdup(string);
    buffer->write_index = (buffer->write_index + 1) % PC_buffer_len;
    buffer->count++;

    // Segnala che il buffer non è vuoto
    xpthread_cond_signal(&(buffer->buffer_empty), HERE);


    xpthread_mutex_unlock(&(buffer->lock), HERE);


    return 0; 
}

// Preleva una stringa dal buffer 
char *buffer_pop(bounded_buffer_t *buffer)
{
    char *string = NULL;

    xpthread_mutex_lock(&(buffer->lock), HERE);

    // Attende finché il buffer è vuoto
    while (buffer->count == 0)
        xpthread_cond_wait(&(buffer->buffer_empty), &(buffer->lock), HERE);

    // Verifica se il buffer è stato segnato come vuoto
    if (buffer->count == -1)
    {
        xpthread_mutex_unlock(&(buffer->lock), HERE);
        return NULL;  // Buffer segnato come vuoto
    }


    // Preleva la stringa dal buffer
    string = buffer->buffer[buffer->read_index];
    buffer->buffer[buffer->read_index] = NULL;
    buffer->read_index = (buffer->read_index + 1) % PC_buffer_len;
    buffer->count--;

    // Segnala che il buffer non è pieno
    xpthread_cond_signal(&(buffer->buffer_full), HERE);

    xpthread_mutex_unlock(&(buffer->lock), HERE);
    

    return string;  // Restituisce la stringa prelevata dal buffer
}
