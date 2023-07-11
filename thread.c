#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>

#include "xerrori.h"
#include "hash.h"
#include "buffer.h"
#include "thread.h"

extern char* strdup(const char*);

#define MAX_SEQUENCE_LENGTH 2048

#define HERE __LINE__, __FILE__

int tokenize(const char *input_string, size_t length, char ***token_array)
{
    // Controlla gli argomenti
    if (input_string == NULL || token_array == NULL)
    {
        return -1;
    }

    // Copia l'input in una nuova stringa
    char *input_cp = malloc((length + 1) * sizeof(char));
    if (input_cp == NULL) return -1; 

    memcpy(input_cp, input_string, length + 1);
    input_cp[length] = '\0';

    // Tokenizza la stringa
    char *token;
    char *saveptr;
    int token_count = 0;
    char **tokens = NULL;

    token = strtok_r(input_cp, ".,:; \n\r\t", &saveptr);
    while (token != NULL)
    {
        // Copia il token in una nuova stringa
        char *token_copy = strdup(token);
        if (token_copy == NULL)
        {
            // Errore: memoria insufficiente per copiare il token
            for (int i = 0; i < token_count; i++)
                free(tokens[i]);

            free(tokens);
            free(input_cp);
            return -1; 
        }

        // Ridimensiona l'array di output
        char **temp = realloc(tokens, (token_count + 1) * sizeof(char *));
        if (temp == NULL)
        {
            // Errore: memoria insufficiente per ridimensionare l'array di output
            free(token_copy);

            // Dealloca tutte le stringhe di token già copiate
            for (int i = 0; i < token_count; i++)
                free(tokens[i]);

            free(tokens);
            free(input_cp);
            return -1; 
        }
        tokens = temp;

        // Aggiunge il token all'array di output
        tokens[token_count] = token_copy;

        token_count++;
        token = strtok_r(NULL, ".,:; \n\r\t", &saveptr);
    }

    // Imposta l'array di output
    *token_array = tokens;

    // Dealloca la stringa temporanea
    free(input_cp);

    return token_count;
}



void *master_body(void *arg){
    master *arguments = (master *)arg;

    //apertura pipe
    int pipe= open(arguments->pipename, O_RDONLY);
    if(pipe==-1) xtermina("errore open pipe", HERE);
    
    char readbuffer[MAX_SEQUENCE_LENGTH];
    int nread;

    //leggo dalla pipe
    while((nread = read(pipe, readbuffer, sizeof(uint16_t)))>0){
        if(nread < (int) sizeof(uint16_t)){
            break;
        }
        
        uint16_t length;

        memcpy(&length, readbuffer, sizeof(uint16_t));
        length = ntohs(length);

        if(length>MAX_SEQUENCE_LENGTH) break;

        nread=read(pipe, readbuffer, length);
        if(nread<=0) break;

        // Creo l'array di token
        char **token_array;
        int token_count = tokenize(readbuffer, nread, &token_array);
        if(token_count==-1) xtermina("errore tokenize", HERE);

        // Inserisco i token nel buffer
        for(int i=0; i<token_count; i++){
            char *token = token_array[i];
            buffer_push(&(arguments->buffer), token);
            free(token_array[i]);
        }

        free(token_array);
        
    }

    if(nread==-1) xtermina("errore lettura", HERE);

    //segnalo la fine della lettura
    xpthread_mutex_lock(&(arguments->buffer.lock),HERE);
    arguments->buffer.count=-1;
    xpthread_cond_broadcast(&(arguments->buffer.buffer_empty), HERE);
    xpthread_mutex_unlock(&(arguments->buffer.lock),HERE);


    xclose(pipe, HERE);
    pthread_exit(NULL);
}

void *writer_body(void *arg)
{
    writers *scrittore = (writers *)arg;

    char *str;

    //Leggo dal buffer e scrivo sulla tabella hash
    while ((str = buffer_pop(scrittore->buffer)) != NULL)
    {
        aggiungi(str);
        free(str);
    }
    pthread_exit(NULL);
}

void *reader_body(void *arg)
{
    readers *lettore = (readers *)arg;

    char *str;
    int value;
    //Leggo dal buffer, leggo dalla tabella hash e scrivo sul file
    while ((str = buffer_pop(lettore->buffer)) != NULL)
    {
        value = conta(str);

        xpthread_mutex_lock(&(lettore->filemutex), HERE);
        fprintf(lettore->file, "%s %d\n", str, value);
        xpthread_mutex_unlock(&(lettore->filemutex), HERE);
        
        free(str);
    }

    pthread_exit(NULL);
}
