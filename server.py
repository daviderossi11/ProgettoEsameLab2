#!/usr/bin/env python3

import argparse
import logging
import os
import signal
import socket
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from struct import pack, unpack

HOST = "127.0.0.1"
PORT = 54251
MAX_SEQUENCE_LENGTH = 2048

# Configurazione del logger
LOG_FILE = "server.log"
logging.basicConfig(
    filename=LOG_FILE,
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s",
)

# File descriptor delle pipe
capolet_fd = None
caposc_fd = None

# Variabili per la dimensione dei dati
capolet_size = 0
caposc_size = 0


# Funzione per scrivere sequenze di byte nelle FIFO
def write_to_pipe(data, pipe):
    try:
        if pipe == "capolet":
            os.write(capolet_fd, data)
        elif pipe == "caposc":
            os.write(caposc_fd, data)
    except OSError as e:
        logging.error(f"Error while writing to pipe: {e}")

# Funzione per gestire la connessione di un client
def handle_client(connection):
    global capolet_size, caposc_size
    # Leggi il tipo del client (A o B)
    client_type = connection.recv(1).decode()

    if client_type == "A":
        # Leggi la lunghezza della sequenza di byte
        length_bytes = recv_all(connection, 2)
        length = unpack("!H", length_bytes)[0]

        # Leggi la sequenza di byte dal client di tipo A
        data = recv_all(connection, length)
        if data.decode() == "EOF":

            # Invio della sequenza di byte di lunghezza 0 per indicare la fine dei dati
            write_to_pipe(b"", "capolet")
        else:

            # Invio della sequenza di byte al client di tipo A
            logging.info("Connected client of type A")
            capolet_size = capolet_size + length
            data = length_bytes + data
            write_to_pipe(data, "capolet")

    elif client_type == "B":
        logging.info("Connected client of type B")
        while True:
            # Leggi la lunghezza della sequenza di byte
            length_bytes = recv_all(connection, 2)
            length = unpack("!H", length_bytes)[0]
            if length == 0:
                break

            # Leggi la sequenza di byte dal client di tipo B
            data = recv_all(connection, length)
            if (data.decode()) == "EOF":
                # Invio della sequenza di byte di lunghezza 0 per indicare la fine dei dati
                write_to_pipe(b"", "caposc")
            else:
                # Invio della sequenza di byte al client di tipo B
                caposc_size = caposc_size + length
                data = length_bytes + data
                write_to_pipe(data, "caposc")

    connection.close()


# Funzione per avviare il server
def start_server(address, port, max_threads, process):
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((address, port))
    server_socket.listen(5)
    logging.info(f"Server started on {address}:{port}")

    global capolet_fd, caposc_fd, capolet_size, caposc_size
    # Apertura delle pipe
    capolet_fd = os.open("capolet", os.O_WRONLY)
    caposc_fd = os.open("caposc", os.O_WRONLY)

    # Crea il pool di thread per gestire i client
    executor = ThreadPoolExecutor(max_threads)

    try:
        while True:
            connection, client_address = server_socket.accept()
            executor.submit(handle_client, connection)
    except KeyboardInterrupt:
        logging.info("Server interrupted")
    finally:

        # Stampa le statistiche
        logging.info(f"Total bytes received from client A: {capolet_size}")
        logging.info(f"Total bytes received from client B: {caposc_size}")

        # Chiudi le pipe
        os.close(capolet_fd)
        os.close(caposc_fd)

        # Rimuovi le FIFO caposc e capolet
        os.unlink("caposc")
        os.unlink("capolet")

        # Termina il processo archivio
        if process:
            process.terminate()
            process.wait()

        # Chiudi il socket del server
        server_socket.shutdown(socket.SHUT_RDWR)
        server_socket.close()


# Funzione per leggere una quantità specifica di dati dal socket
def recv_all(conn, n):
    chunks = b""
    bytes_recd = 0
    while bytes_recd < n:
        chunk = conn.recv(min(n - bytes_recd, MAX_SEQUENCE_LENGTH))
        if len(chunk) == 0:
            raise RuntimeError("Socket connection broken")
        chunks += chunk
        bytes_recd += len(chunk)
    return chunks


# Funzione principale
def main():
    # Parsing degli argomenti della linea di comando
    parser = argparse.ArgumentParser()
    parser.add_argument("max_threads", type=int, help="Maximum number of threads to use")
    parser.add_argument("-r", type=int, default=3, help="Number of reader threads (excluding bosses)")
    parser.add_argument("-w", type=int, default=3, help="Number of writer threads (excluding bosses)")
    parser.add_argument("-v", action="store_true", help="Use valgrind for running the 'archivio' program")
    # Aggiungi gli altri argomenti necessari

    args = parser.parse_args()

    if args.max_threads <= 0:
        print("The number of threads must be greater than 0")
        sys.exit(1)

    if args.r<3: args.r=3
    if args.w<3: args.w=3

    # Avvia il programma archivio con i parametri specificati

    archivio_command = ["./archivio", str(args.r), str(args.w)]
    if args.v:
        archivio_command = ["valgrind", "--leak-check=full", "--show-leak-kinds=all", "--log-file=valgrind-%p.log"] + archivio_command

    process = subprocess.Popen(archivio_command)

    # Crea le FIFO capolet e caposc se non esistono già
    if not os.path.exists("capolet"):
        os.mkfifo("capolet")
    if not os.path.exists("caposc"):
        os.mkfifo("caposc")

    # Imposta il gestore del segnale SIGINT per terminare il server
    def signal_handler(sig, frame):
        logging.info("Received SIGINT signal. Server shutting down.")
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    # Avvia il server
    start_server(HOST, PORT, args.max_threads, process)


if __name__ == "__main__":
    main()
