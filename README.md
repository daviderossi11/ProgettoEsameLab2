# Progetto per Esame di Laboratorio 2

Il progetto consiste nello sviluppo di un server per la gestione di due tipi di connessioni diverse, che interagisce con un archivio. Il server e i client sono scritti in Python, mentre l'archivio è un programma C eseguito come sottoprocesso dal server. I client interagiscono, tramite il server, con una tabella hash definita nell'archivio, consentendo l'aggiunta di elementi o la lettura del numero di occorrenze di quelli già presenti. Gli output di lettura saranno scritti nel file lettori.log, mentre tutte le connessioni saranno tracciate nel file server.log. L'archivio gestisce i segnali **SIGINT** e **SIGTERM**: alla ricezione del primo, il programma scrive su **STDERR** il numero di stringhe distinte salvate nell'hash table e non termina; per **SIGTERM**, il programma scrive il numero di stringhe distinte salvate nell'hash table su **STDOUT** e termina in modo pulito. Il server gestisce la ricezione di **SIGINT**, gestendo l'eccezione e inviando un SIGTERM all'archivio. Se abilitato, al termine del programma verrà anche prodotto un file di log con l'output del tool Valgrind.


## Come scaricare il progetto

Il progetto deve essere eseguito esclusivamente su ambiente Linux.
Per installare il progetto, segui questi comandi:
```shell
git clone  git@github.com:daviderossi11/ProgettoEsameLab2.git ProgettoEsameLab2
cd ProgettoEsameLab2
```

## Come usare il progetto

Il server espone diversi parametri:

- `<max_threads>`: il numero massimo di thread che il server può creare per gestire le connessioni in ingresso (obbligatorio)
- `-r <num_readers>`: il numero di thread readers che l'archivio crea (default: 3)
- `-w <num_writers>`: il numero di thread writers che l'archivio crea (default: 3)
- `-v`: flag per abilitare la creazione di un file di log con l'output di Valgrind (default: non presente)

Il client di tipo 1:
- `<filename>`: nome del file in input (obbligatorio)

Il client di tipo 2:
- `<filenames>`: nome dei file in input (obbligatorio almeno un file)

L'archivio:
- `<num_writers>`: il numero di thread writers (obbligatorio)
- `<num_readers>`: il numero di thread readers (obbligatorio)

Per eseguire correttamente il programma, è necessario eseguire prima il `Makefile` per creare l'eseguibile dell'archivio.

Esempio:

```shell
make
./server.py 5 -r 2 -w 4 -v &  # parte il server con 5 thread che a sua volta fa partire l'archivio
./client2 file1 file2         # scrive dati sull'archivio
./client1 file3               # interroga l'archivio
pkill -INT -f server.py       # manda un segnale SIGINT al server che manderà un segnale SIGTERM a archivio che terminerà
```

## Struttura file del progetto
Il progetto è strutturato in una singola cartella radice che contiene tutti i file del progetto.
I seguenti file fanno parte del progetto:

- `server.py`: eseguibile Python per la creazione e la gestione del server.
- `client1`: eseguibile Python per eseguire una connessione di tipo A con il server.
- `client2`: eseguibile Python per eseguire una connessione di tipo B con il server.
- `archivio.c`: codice sorgente dell'eseguibile `archivio` ottenuto dal `Makefile` e avviato come sottoprocesso da `server.py`.
- `thread.c` e `thread.h` : libreria che espone funzioni per il corpo dei thread e la funzione `tokenize`usati in `archivio.c`, `thread.h` è incluso in `ar\chivio.c`
- `buffer.c` e `buffer.h`: libreria che espone funzioni per la gestione di un buffer circolare in uno schema produttore-consumatore. `buffer.h` è incluso in `archivio.c`.
- `hash.c` e `hash.h`: libreria che espone funzioni per la gestione dell'accesso multithread a una tabella hash. `hash.h` è incluso in `archivio.c`.
- `xerrori.c` e `xerrori.h`: libreria che espone funzioni per la gestione degli errori. `xerrori.h` è incluso in `archivio.c`.
- `file1`, `file2` e `file3`: file di testo da passare ai client per verificare il corretto funzionamento del programma.


## Scelte implementative

Le principali scelte implementative del progetto sono le seguenti:

- Il file `server.py` e `archivio.c` sono rispettivamente realizzati in Python e in C (dove `server.py` è un eseguibile), mentre entrambi i client (`client1` e `client2`) sono eseguibili Python. Questa scelta è stata fatta poiché, per il progetto ridotto, era libera la scelta tra Python e C.
- È stata creata la libreria hash.c per la gestione dell'accesso concorrente all'hash table, al fine di rendere il codice più modulare e leggibile. Analogamente, la libreria buffer.c è stata creata per la gestione del buffer, mentre thread.c per la gestione dei corpi dei thread.
- La funzione `int tokenize(const char *input_string, size_t length, char ***output)` utilizza la funzione `strtok_r()` al posto di `strtok()` poiché è thread-safe, essendo utilizzata in un programma multithread.
- Per quanto riguarda l'accesso concorrente all'hash table definita in `hash.c`, è stato utilizzato uno schema scrittori/lettori sbilanciato a favore degli scrittori, poiché le operazioni di scrittura saranno più frequenti rispetto alle operazioni di lettura. Inoltre, le operazioni di scrittura modificano l'integrità dei dati, a differenza delle operazioni di lettura che possono essere eseguite in parallelo.
- Quando `server.py` riceve il segnale **SIGINT** manda un segnale **SIGTERM** ad `archivio` che distruggerà tutti i mutex e le conditional variables, e deallocherà tutta la memoria usata (compresa la tabella hash)



## Protocollo di rete

Il protocollo di rete seguente viene utilizzato per la comunicazione tra il server e i client.
Il server apre un socket TCP e rimane in attesa di connessioni dai client.
I client possono effettuare due tipi di connessioni: A e B.

##### Connessioni di tipo A
Le connessioni di tipo A servono per interrogare l'hash table.

- Il client1 utilizza le connessioni di tipo A.
- Il client apre una connessione di tipo A inviando un pacchetto di connessione contenente l'identificatore 'A', seguito dalla lunghezza del messaggio e dal messaggio effettivo.
- Dopo l'invio del pacchetto, la connessione viene chiusa.
- Se il client1 deve inviare più stringhe, deve aprire una nuova connessione di tipo A per ciascuna stringa.
- Quando il client ha finito di scrivere manda un dato al server in modo che anche lui capisca che non riceverà altri dati dal client.

##### Connessioni di tipo B
Le connessioni di tipo B servono per aggiungere elementi all'hash table.

- Il client2 utilizza le connessioni di tipo B.
- il client apre una connessione per ogni file che gli è stato passato e una finale per indicare al server che ha finito di leggere tutti i file
- Il client apre una connessione di tipo B inviando un pacchetto di connessione contenente l'identificatore 'B', seguito dalla lunghezza del messaggio e dal messaggio effettivo.
- L'identificatore viene indicato solo ad apertura della connessione.
- La connessione rimane aperta finché il client2 ha messaggi da inviare.
- Quando il client2 ha terminato l'invio dei messaggi, la connessione viene chiusa.

