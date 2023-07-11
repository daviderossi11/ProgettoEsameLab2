#include <search.h>
#include <string.h>
#include <stdlib.h>
#include "xerrori.h"

extern char* strdup(const char*);

#define HERE __LINE__,__FILE__
typedef struct
{
    int init;                      // Variabile per la tabella hash
    int num_entry;               // Numero di stringhe distinte caricate nell'hash table
    int num_readers;             // Numero di lettori attivi
    int num_writers;             // Numero di scrittori attivi
    pthread_mutex_t hash_lock;   // Mutex per garantire l'accesso esclusivo alla tabella hash
    pthread_cond_t readers_cond; // Variabile di condizione per i lettori
    pthread_cond_t writers_cond;  // Variabile di condizione per gli scrittori
} Hash_t;

Hash_t hash;

typedef struct {
  int valore;    // numero di occorrenze della stringa 
  ENTRY *next;      // puntatore al prossimo elemento
} coppia;

// Puntatore alla testa della lista
ENTRY *testa_lista_entry = NULL;



// Crea una nuova tabella hash con la dimensione specificata
void creahash(size_t dim) {
    hash.init = hcreate(dim);

    // Inizializza le variabili di sincronizzazione
    if (hash.init == 0)
        xtermina("ERROR DURING THE CREATION OF THE HASHMAP", HERE);
    if (xpthread_mutex_init(&(hash.hash_lock), NULL, HERE) != 0)
        xtermina("ERROR DURING MUTEX INITIALIZATION", HERE);
    if (xpthread_cond_init(&(hash.readers_cond), NULL, HERE) != 0)
    {
        xpthread_mutex_destroy(&(hash.hash_lock), HERE);
        xtermina("ERROR DURING READERS CONDITION VARIABLE INITIALIZATION", HERE);
    }
    if (xpthread_cond_init(&(hash.writers_cond), NULL, HERE) != 0)
    {
        xpthread_mutex_destroy(&(hash.hash_lock), HERE);
        xpthread_cond_destroy(&(hash.readers_cond), HERE);
        xtermina("ERROR DURING WRITERS CONDITION VARIABLE INITIALIZATION", HERE);
    }
    hash.num_entry = 0;
    hash.num_readers = 0;
    hash.num_writers = 0;
}

ENTRY *crea_entry(char *s, int n) {
  ENTRY *e = calloc(1,sizeof(ENTRY));
  if(e==NULL) xtermina("errore malloc entry 1", HERE);
  e->key = strdup(s); // salva copia di s
  e->data = calloc(1,sizeof(coppia));
  if(e->key==NULL || e->data==NULL)
    xtermina("errore malloc entry 2", HERE);
  // inizializzo coppia
  coppia *c = (coppia *) e->data; // cast obbligatorio
  c->valore  = n;
  c->next = NULL;
  return e;
}

// Aggiunge un elemento alla tabella hash o incrementa il valore associato se l'elemento è già presente
void aggiungi(char *s) {
    ENTRY *e=crea_entry(s,1);

    xpthread_mutex_lock(&(hash.hash_lock), HERE);

    // Aspetta che tutti i scrittori abbiano terminato
    while (hash.num_writers > 0)
    {
        xpthread_cond_wait(&(hash.writers_cond), &(hash.hash_lock), HERE);
    }

    hash.num_writers++;

    ENTRY *ep= hsearch(*e, FIND);

    if(ep==NULL){

        // Inserisco un nuovo elemento nella tabella hash
        ep = hsearch(*e, ENTER);
        if(ep==NULL) xtermina("ERROR THE TABLE COULD BE FULL", HERE);

        coppia *c = (coppia *) e->data;
        // metto la vecchia testa della lista come next della nuova entry
        c->next = testa_lista_entry;
        // aggiorno la testa della lista
        testa_lista_entry = e;
        // aggiorno il numero di elementi nella tabella hash
        hash.num_entry++;
    } else {
        // l'elemento è gia' presente incremento il valore
        assert(strcmp(e->key,ep->key)==0);
        coppia *c = (coppia *) ep->data;
        c->valore +=1;
        free(e->key);
        free(e->data);
        free(e);
    }
    hash.num_writers--;

    // Sveglia eventuali thread scrittori in attesa
    xpthread_cond_signal(&(hash.writers_cond), HERE);

    // Sveglia eventuali thread lettori in attesa
    xpthread_cond_broadcast(&(hash.readers_cond), HERE);

    xpthread_mutex_unlock(&(hash.hash_lock), HERE);

}

// Restituisce il valore associato all'elemento nella tabella hash
int conta(char *s) {
    int value = 0;


    ENTRY *e=crea_entry(s,0);

    xpthread_mutex_lock(&(hash.hash_lock), HERE);


    // Aspetta che tutti i scrittori abbiano terminato
    while (hash.num_writers > 0)
    {
        xpthread_cond_wait(&(hash.writers_cond), &(hash.hash_lock), HERE);
    }

    hash.num_readers++;
    
    //controllo chhe la stringa sia presente nella tabella hash
    ENTRY *ep = hsearch(*e, FIND);
    if (ep != NULL){
        coppia *c = (coppia *) ep->data;
        value= c->valore;
    }

    free(e->key);
    free(e->data);
    free(e);

    hash.num_readers--;

    // Sveglia eventuali thread scrittori in attesa
    xpthread_cond_signal(&(hash.writers_cond), HERE);

    xpthread_mutex_unlock(&(hash.hash_lock), HERE);
    return value;
}

// Distrugge la tabella hash, liberando la memoria associata
void distruggihash() {

    // Libero la memoria allocata per le entry della tabella hash
    ENTRY *temp=NULL;
    while(testa_lista_entry!=NULL){
        
        coppia *c = (coppia *) testa_lista_entry->data;
        temp= c->next;
        free(c);
        free(testa_lista_entry->key);
        free(testa_lista_entry);
        testa_lista_entry=temp;
    }


    // Distrugge mutex e condition variable
    if (xpthread_mutex_destroy(&(hash.hash_lock), HERE) != 0) {
        xtermina("ERROR DURING MUTEX DESTRUCTION", HERE);
    }

    if (xpthread_cond_destroy(&(hash.readers_cond), HERE) != 0) {
        xtermina("ERROR DURING READERS CONDITION VARIABLE DESTRUCTION", HERE);
    }

    if (xpthread_cond_destroy(&(hash.writers_cond), HERE) != 0) {
        xtermina("ERROR DURING WRITERS CONDITION VARIABLE DESTRUCTION", HERE);
    }


    // Distrugge la tabella hash
    hdestroy();
}


// Restituisce il numero di elementi presenti nella tabella hash
int hash_size() {
    return hash.num_entry;
}

