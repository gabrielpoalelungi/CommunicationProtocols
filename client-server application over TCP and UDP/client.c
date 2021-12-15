#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include <netinet/tcp.h>
#include <netinet/tcp.h>

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	if (argc < 4) {
		usage(argv[0]);
	}
	// Buffer-ul in care se primesc mesaje;
	char buffer[BUFLEN];

	// Initializarea multimii de socketi pe care o sa se faca multiplexarea; 
	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// Socket-ul TCP
	int tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sockfd < 0, "tcp - client socket");

	// Dezactivarea algoritmului Neagle;
	int flag = 1;
	int result = setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, (void *) &flag, sizeof(flag));
	DIE (result < 0, "Eroare la setare tcp no delay");

	// Completarea in server_address cu adresa serverului, familia de adrese si portul pentru conectare;
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(atoi(argv[3]));
	int ret_sin_addr = inet_aton(argv[2], &server_address.sin_addr);
	DIE(ret_sin_addr == 0, "inet_aton at server_address - client");

	// Conectarea la server;
	int ret_connect = connect(tcp_sockfd, (struct sockaddr*) &server_address, sizeof(server_address));
	DIE (ret_connect < 0, "connect - client");

	// Adaugarea in multimea de socketi a socket-ului TCP si cel al STDIN-ului;
	FD_SET(tcp_sockfd, &read_fds);
	FD_SET(0, &read_fds);
	fdmax = tcp_sockfd;

	// Dezactivarea buffer-ului la afisare;
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	while(1) {

		tmp_fds = read_fds;
		int ret_select = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);

		memset(buffer, 0, BUFLEN);
		// Daca se primeste input de pe socket-ul de la STDIN;
		if (FD_ISSET(0, &tmp_fds)) {

			fgets(buffer, BUFLEN - 1, stdin);
			// Daca inputul este "exit", se trimite un mesaj de deconectare la server
			// si se inchide programul;
			if (strncmp(buffer, "exit", 4) == 0) {

				memset(buffer, 0, BUFLEN);
				strcpy(buffer, "Client ");
				strcat(buffer, argv[1]);
				strcat(buffer, " disconnected.");
				int n = send(tcp_sockfd, buffer, sizeof(buffer), 0);
				DIE(n < 0, "trimitere diconnect.\n");
				break;

			} else {

				// Se afiseaza in client un mesaj corespunzator comenzii;
				if(strstr(buffer, "unsubscribe")) {
					// Se trimite inputul de tip "subscribe";
					int n = send(tcp_sockfd, buffer, sizeof(buffer), 0);
					DIE(n < 0, "eroare trimitere subscribe");
					printf("Unsubscribed from topic.\n");
				} else if (strstr(buffer, "subscribe")) {
					// Se trimite inputul de tip "unsubscribe";
					int n = send(tcp_sockfd, buffer, sizeof(buffer), 0);
					DIE(n < 0, "eroare trimitere unsubscribe");
					printf("Subscribed to topic.\n");
				}
			}
			memset(buffer, 0, BUFLEN);

		} else {

			int bytes_received = recv(tcp_sockfd, buffer, BUFLEN, 0);
            DIE(bytes_received < 0, "Eroare la primire mesaj din client\n");

        	// Server-ul intreaba ce ID are clientul curent pentru a-l adauga
        	// in lista lui de clienti;
        	if (strstr(buffer, "ID_CLIENT?")) {

        		memset(buffer, 0, BUFLEN);
        		strcpy(buffer, "ID_CLIENT:");
        		strcat(buffer, argv[1]);
        		int n = send(tcp_sockfd, buffer, sizeof(buffer), 0);
        		DIE(n < 0, "eroare trimitere id client\n");

        	} else if(strstr(buffer, "close client")) {
        		// Daca server-ul se inchide, clientul primeste de la server
        		// comanda "close client", iar acesta se inchide;
        		break;

        	} else {

        		// Se primeste mesajul UDP prelucrat de server;
        		printf("%s\n", buffer);

       			// Se trimite un ACK server-ului pentru a confirma primirea mesajului;
        		memset(buffer, 0, BUFLEN);
        		strcpy(buffer, "GOT_IT");
        		int n = send(tcp_sockfd, buffer, sizeof(buffer), 0);
        		DIE(n < 0, "eroare trimitere ACK\n");

        	}
        	memset(buffer, 0, BUFLEN);
		}
	}
	close(tcp_sockfd);
	return 0;
}
