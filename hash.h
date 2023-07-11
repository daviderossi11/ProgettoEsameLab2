#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>

// Crea una nuova tabella hash con la dimensione specificata
void creahash(size_t dim);

// Aggiunge un elemento alla tabella hash o incrementa il valore associato se l'elemento è già presente
void aggiungi(char *s);

// Restituisce il valore associato all'elemento nella tabella hash
int conta(char *s);

// Distrugge la tabella hash, liberando la memoria associata
void distruggihash();

// Restituisce il numero di elementi presenti nella tabella hash
int hash_size();



#endif  // HASHMAP_H
