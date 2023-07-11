#ifndef THREAD_H
#define THREAD_H

#include "buffer.h"

//Struttura dati per il master
typedef struct {
    char *pipename;
    bounded_buffer_t buffer;
} master;

//Struttura dati per i writer
typedef struct {
    bounded_buffer_t *buffer;
} writers;

//Struttura dati per i reader
typedef struct {
    bounded_buffer_t *buffer;
    pthread_mutex_t filemutex;
    FILE *file;
} readers;

//corpo dei thread master
void *master_body(void *arg);

//corpo dei thread writer
void *writer_body(void *arg);

//corpo dei thread reader
void *reader_body(void *arg);

#endif  // THREAD_H
