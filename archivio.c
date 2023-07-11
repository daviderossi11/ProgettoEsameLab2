#define _GNU_SOURCE
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <sys/syscall.h>

#include "xerrori.h"
#include "hash.h"
#include "buffer.h"
#include "thread.h"

#define MAX_SEQUENCE_LENGTH 2048
#define FILE_READER "lettori.log"
#define NUM_ELEM 1000000
#define HERE __LINE__, __FILE__



// Corpo per la gestione dei segnali
void *signal_handler(void *arg) {
    sigset_t *set = (sigset_t *)arg;
    int sig;
    char message[256];
    int len;
    int ret;

    while (1) {
        sigwait(set, &sig);

        if (sig == SIGINT) {

            // Stampa il numero di elementi presenti nella hash map
            len = sprintf(message, "THE PROCESS %ld GOT SENDEND A SIGINT SIGNAL. INSIDE THE HASHMAP WE HAVE %d ELEMENTS\n", (long)syscall(SYS_gettid), hash_size());
            ret = write(STDOUT_FILENO, message, len);
            if(ret<0) xtermina("ERROR IN WRITE", HERE);
        } else if (sig == SIGTERM) {

            // Stampa il numero di elementi presenti nella hash map e termina
            len = sprintf(message, "THE PROCESS %ld GOT SENDEND A SIGTERM SIGNAL. INSIDE THE HASHMAP WE HAVE %d ELEMENTS\n", (long)syscall(SYS_gettid), hash_size());
            ret = write(STDOUT_FILENO, message, len);
            if(ret<0) xtermina("ERROR IN WRITE", HERE);
            pthread_exit(NULL);
        }
    }
}



int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <num_writers> <num_readers>\n", argv[0]);
        return 1;
    }

    int num_writers = atoi(argv[2]);
    int num_readers = atoi(argv[1]);

    if(num_writers < 3 || num_readers < 3) xtermina("ERROR IN THE NUMBER OF WRITERS OR READERS", HERE);

    // creazione delle strutture dati
    master caposc, capolet;
    readers lettori;
    writers scrittori;

    // Creazione dei thread
    pthread_t master_w, master_r, master_sig;
    pthread_t *writer_threads = calloc(num_writers, sizeof(pthread_t));
    pthread_t *reader_threads = calloc(num_readers, sizeof(pthread_t));

    // Creazione hashmap
    creahash(NUM_ELEM);
    
    // Associo le pipe ai master
    caposc.pipename = calloc(7, sizeof(char));
    capolet.pipename = calloc(8, sizeof(char));
    strcpy(caposc.pipename, "caposc");
    strcpy(capolet.pipename, "capolet");

    // Creazione buffer
    buffer_init(&(caposc.buffer));
    buffer_init(&(capolet.buffer));
    lettori.buffer = &(capolet.buffer);
    scrittori.buffer = &(caposc.buffer);

    // Inizzializzazione segnali
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    // Apertura file "lettori.log"
    lettori.file = xfopen(FILE_READER, "wr", HERE);
    xpthread_mutex_init(&(lettori.filemutex), NULL, HERE);

    // Creazione thread master
    xpthread_create(&master_w, NULL, master_body, &caposc, HERE);
    xpthread_create(&master_r, NULL, master_body, &capolet, HERE);
    xpthread_create(&master_sig, NULL, signal_handler, &set, HERE);

   // Creazione thread writer
   for (int i = 0; i < num_writers; i++) {
        xpthread_create(&(writer_threads[i]), NULL, writer_body, &scrittori, HERE);
    }

    // Creazione thread reader
    for (int i = 0; i < num_readers; i++) {
        xpthread_create(&(reader_threads[i]), NULL, reader_body, &lettori, HERE);
    }

    // Attendo che il thread di gestione dei segnali riceva SIGTERM
    xpthread_join(master_sig, NULL, HERE);


    // Join dei lettori
    for (int i = 0; i < num_readers; i++) {
        xpthread_join(reader_threads[i], NULL, HERE);
    }


    // Join dei writer
    for (int i = 0; i < num_writers; i++) {
        xpthread_join(writer_threads[i], NULL, HERE);
    }

    // Join dei master

    xpthread_join(master_w, NULL, HERE);
    xpthread_join(master_r, NULL, HERE);

    // Deallocazione memoria
    free(caposc.pipename);
    free(capolet.pipename);

    // Chiusura file
    fclose(lettori.file);
    xpthread_mutex_destroy(&(lettori.filemutex), HERE);

    // Deallocazione memoria
    free(reader_threads);
    free(writer_threads);

    
    // Distruzione dei buffer
    buffer_destroy(&(caposc.buffer));
    buffer_destroy(&(capolet.buffer));

    // Distruzione hashmap
    distruggihash();
    

    return 0;
}
