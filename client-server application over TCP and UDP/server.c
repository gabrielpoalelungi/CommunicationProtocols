#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include <math.h>
#include <netinet/tcp.h>

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[]) {

	if (argc < 2) {
		usage(argv[0]);
	}

	// Buffer-ul in care se primesc mesaje;
	char buffer[BUFLEN];

	// Vectorul de topic-uri care memoreaza, pentru fiecare, vectorul de abonati;
	struct topic all_topics[100];
	int n_all_topics = 0;

	// Vectorul de clienti TCP inregistrati pe server;
	struct tcp_client tcp_clients[100];
	int n_tcp_clients = 0;

	struct sockaddr_in server_addr, tcp_client_addr;
	socklen_t client_len;

	int port = atoi(argv[1]);
	DIE (port == 0, "atoi");

	// Portul UDP
	int udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(udp_sockfd < 0, "socket udp");

	// Portul pasiv TCP
	int tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sockfd < 0, "socket tcp");
	int flag = 1;
	int result = setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, (void *) &flag, sizeof(flag));
	DIE (result < 0, "Eroare la setare tcp no delay");

	// Adresa serverului, familia de adrese si portul rezervat pentru server pentru bind
	memset((char *) &server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	// Multimea de descriptori de citire
	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// Asocierea socket-ului UDP cu serverul;
	int ret_udp = bind(udp_sockfd, (struct sockaddr *) &server_addr, sizeof (struct sockaddr));
	DIE(ret_udp < 0, "bind UDP");

	// Asocierea socket-ului TCP cu serverul;
	int ret_tcp = bind(tcp_sockfd, (struct sockaddr *) &server_addr, sizeof (struct sockaddr));
	DIE(ret_tcp < 0, "bind TCP");

	// Ascult pentru MAX_CLIENTS clienti;
	ret_tcp = listen(tcp_sockfd, MAX_CLIENTS);
	DIE(ret_tcp < 0, "listen TCP");
	
	FD_SET (tcp_sockfd, &read_fds);
	FD_SET (0, &read_fds);
	FD_SET (udp_sockfd, &read_fds);

	if (tcp_sockfd > udp_sockfd) {
		fdmax = tcp_sockfd;
	} else {
		fdmax = udp_sockfd;
	}

	int OK = 1;
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	while(OK) {
		int new_tcp_sockfd;
		tmp_fds = read_fds;
		int ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select - server");

		memset(buffer, 0, BUFLEN);

		for(int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				// Daca un client TCP doreste sa se conecteze la server;
				if (i == tcp_sockfd) {
					client_len = sizeof(tcp_client_addr);
					new_tcp_sockfd = accept(tcp_sockfd, (struct sockaddr *) &tcp_client_addr, &client_len);
					DIE(new_tcp_sockfd < 0, "accept - server");

					FD_SET(new_tcp_sockfd, &read_fds);
					if (new_tcp_sockfd > fdmax) {
						fdmax = new_tcp_sockfd;
					}

					// Intreaba clientul ce ID are pentru a-l inregistra si pentru a afisa
					// un mesaj corespunzator;
					memset(buffer, 0, BUFLEN);
					strcpy(buffer, "ID_CLIENT?");
					int n = send(new_tcp_sockfd, buffer, strlen(buffer) + 1, 0);
					DIE (n < 0, "eroare send ID_CLIENT?");
				} else if (i == 0) {
					// Se citeste input de la tastatura;
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);

					// Se inchide serverul, se inchid toti socketii conectati si se trimite cate
					// un mesaj de deconectare fiecarui client conectat;
					if (strncmp(buffer, "exit", 4) == 0) {
						for(int k = 0; k < n_tcp_clients; k++) {
							memset(buffer, 0, BUFLEN);
							strcpy(buffer, "close client");
							int n = send(tcp_clients[k].current_socket, buffer, strlen(buffer) + 1, 0);
							close(tcp_clients[k].current_socket);
							FD_CLR(tcp_clients[k].current_socket, &read_fds);
						}
						OK = 0;
						break;
					}
				} else if (i == udp_sockfd) {
					char new_buffer[1575];
					char result_buffer[1575];

					memset(buffer, 0, BUFLEN);
					memset(new_buffer, 0, BUFLEN);
					memset(result_buffer, 0, BUFLEN);

					struct sockaddr_in cli_addr;
					socklen_t socklen = sizeof(cli_addr);

					// Se primeste mesaj de la un client UDP;
					int r = recvfrom(udp_sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*) &cli_addr, &socklen);
					DIE(r < 0, "eroare recvfrom udp_sockfd");

					// Se face cast mesajului la structura noastra de mesaj UDP;
					struct udp_message new_udp_message;
					new_udp_message = *((struct udp_message*) buffer);

					// Daca s-a primit un mesaj de la un topic care e deja inregistrat pe server,
					// nu-l mai adaugam;
					int can_add = 1;
					for (int k = 0; k < n_all_topics; k++) {
						if(strcmp(all_topics[k].topic, new_udp_message.topic) == 0) {
							can_add = 0;
							break;
						}
					}

					if (can_add) {
						all_topics[n_all_topics].n_subscribers = 0;
						strcpy(all_topics[n_all_topics++].topic, new_udp_message.topic);
					}

					// Configurarea mesajului ce va fi trimis abonatilor;
					sprintf(new_buffer, "%s:%u - ", inet_ntoa(cli_addr.sin_addr), cli_addr.sin_port);
					strcpy(result_buffer, new_buffer);
					strcat(result_buffer, new_udp_message.topic);

					if (new_udp_message.tip_date == 0) {

						sprintf(new_buffer, " - INT - ");
						strcat(result_buffer, new_buffer);
						if (new_udp_message.payload[0] == 0) {
							sprintf(new_buffer,"%d", ntohl(*((uint32_t *)(new_udp_message.payload + 1))));
							strcat(result_buffer, new_buffer);
						} else {
							sprintf(new_buffer,"-%d",ntohl(*((uint32_t *)(new_udp_message.payload + 1))));
							strcat(result_buffer, new_buffer);

						}

					} else if (new_udp_message.tip_date == 1) {

						sprintf(new_buffer, " - SHORT_REAL - ");
						strcat(result_buffer, new_buffer);
						sprintf(new_buffer,"%.2f", (float) ntohs(*((uint16_t *)(new_udp_message.payload))) / 100);
						strcat(result_buffer, new_buffer);

					} else if (new_udp_message.tip_date == 2) {

						sprintf(new_buffer, " - FLOAT - ");
						strcat(result_buffer, new_buffer);
						if (new_udp_message.payload[0] == 0) {
							int initial_number = (float)(ntohl(*((uint32_t *)(new_udp_message.payload + 1))));
							int exponent = *((uint8_t *)(new_udp_message.payload + 5));
							double result = initial_number / pow((double) 10, (double) exponent);
							sprintf(new_buffer,"%.*lf", exponent, result);
							strcat(result_buffer, new_buffer);
						} else {
							int initial_number = (double)(ntohl(*((uint32_t *)(new_udp_message.payload + 1))));
							int exponent = *((uint8_t *)(new_udp_message.payload + 5));
							double result = initial_number / pow(10, (double) exponent);
							sprintf(new_buffer,"-%.*lf", exponent, result);
							strcat(result_buffer, new_buffer);
						}
					} else if (new_udp_message.tip_date == 3) {

						sprintf(new_buffer, " - STRING - ");
						strcat(result_buffer, new_buffer);
						sprintf(new_buffer,"%s", new_udp_message.payload);
						strcat(result_buffer, new_buffer);
					}

					// Fiecare mesaj de la fiecare topic va fi trimis tuturor abonatilor sai;
					for(int j = 0; j < n_all_topics; j++) {
						if (strcmp(all_topics[j].topic, new_udp_message.topic) == 0) {
							for(int k = 0; k < all_topics[j].n_subscribers; k++) {
								if (all_topics[j].subscribers[k].subscriber->is_active == 1) {

									int n = send(all_topics[j].subscribers[k].subscriber->current_socket,
										result_buffer, strlen(result_buffer) + 1, 0);
									DIE(n < 0, "eroare la trimitere mesaj catre subscriberi");

									// Nu trimit urmatorul mesaj pana nu am primit ACK;
									int r = recv(all_topics[j].subscribers[k].subscriber->current_socket, buffer, sizeof(buffer), 0);
								} 
								else {
									// In cazul in care un abonat are optiunea SF = 1, iar acesta e deconectat, mesajul va fi pastrat in buffer-ul
									// propriu pana la reconectare;
									if (all_topics[j].subscribers[k].SF == 1) {
										int nr_messages = all_topics[j].subscribers[k].subscriber->nr_unreceived;
										strcpy(all_topics[j].subscribers[k].subscriber->unreceived[nr_messages], result_buffer);
										all_topics[j].subscribers[k].subscriber->nr_unreceived++;
									}
								}
							}
						}
					}
				// In cazul in care se primeste input de la unul din clientii TCP;
				} else {
					memset(buffer, 0, BUFLEN);
					int n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv - server");

					// Se primeste ID-ul clientului;
					if (strstr(buffer, "ID_CLIENT:")) {

						int flag = 1;

						// Se itereaza prin clientii inregistrati si daca mai e un client cu acelasi
						// ID conectat, cel ce doreste sa se conecteze nu va fi lasat si va fi afisat
						// un mesaj corespunzator;
						for (int j = 0; j < n_tcp_clients; j++) {
							if (strcmp(tcp_clients[j].id, buffer + 10) == 0 && tcp_clients[j].is_active == 1) {

								printf("Client %s already connected.\n", buffer + 10);

								memset(buffer, 0, BUFLEN);
								strcpy(buffer, "close client");

								int n = send(new_tcp_sockfd, buffer, strlen(buffer) + 1, 0);
								DIE(n < 0, "eroare trimitere inchidere client in already connected");

								flag = 0;
								FD_CLR(new_tcp_sockfd, &read_fds);
								close(new_tcp_sockfd);							
								break;

							} else if (strcmp(tcp_clients[j].id, buffer + 10) == 0) {
								// Daca clientul e inregistrat pe server, nu e conectat si doreste acum sa
								// se conecteze, va trece prin aceleasi proceduri ca la prima conectare;

								tcp_clients[j].is_active = 1;
								tcp_clients[j].current_socket = i;

								printf("New client %s connected from %s:%d.\n", buffer + 10, 
									inet_ntoa(tcp_client_addr.sin_addr), ntohs(tcp_client_addr.sin_port));

								// Se trimit mesajele primite cat era deconectat de la topic-urile unde are abonament de tip SF = 1;
								for (int p = 0; p < tcp_clients[j].nr_unreceived; p++) {

									int n = send(tcp_clients[j].current_socket, tcp_clients[j].unreceived[p], strlen(tcp_clients[j].unreceived[p]) + 1, 0);
									DIE(n < 0, "eroare la trimitere mesaje pastrate");
									int OK_Send = 1;
									while(OK_Send) {
										memset(buffer, 0, BUFLEN);
										int r = recv(tcp_clients[j].current_socket, buffer, sizeof(buffer), 0);
										if (strstr(buffer, "GOT_IT"))
											OK_Send = 0;
									}
									memset(tcp_clients[j].unreceived[p], 0, 1551);
								}
								tcp_clients[j].nr_unreceived = 0;

								flag = 0;
								break;
							}
						}
						// Prima conectare a clientului;
						if (flag == 1) {
							strcpy(tcp_clients[n_tcp_clients].id, buffer + 10);
							tcp_clients[n_tcp_clients].is_active = 1;
							tcp_clients[n_tcp_clients].current_socket = i;
							tcp_clients[n_tcp_clients].nr_unreceived = 0;
							n_tcp_clients++;

							printf("New client %s connected from %s:%d.\n", buffer + 10, 
								inet_ntoa(tcp_client_addr.sin_addr), ntohs(tcp_client_addr.sin_port));
						}
					}
					// Daca un client doreste sa se deconecteze, trimite un mesaj serverului, iar acesta
					// va afisa un mesaj corespunzator, iar socket-ul corespunzator va fi inchis si scos
					// din lista de socketi;
					if(strstr(buffer, "disconnected.")) {
						printf("%s\n", buffer);
						for (int j = 0; j < n_tcp_clients; j++) {
							if (tcp_clients[j].current_socket == i) {
								tcp_clients[j].is_active = 0;
								FD_CLR(tcp_clients[j].current_socket, &read_fds);
								close(tcp_clients[j].current_socket);
								break;
							}
						}
					}

					// Mesaj de unsubscribe de la client;
					if(strstr(buffer, "unsubscribe")) {
						struct tcp_client current_tcp_client;
						for (int k = 0; k < n_tcp_clients; k++)
							if (tcp_clients[k].current_socket == i) {
								current_tcp_client = tcp_clients[k];
								break;
							}

						// In ultimul token se afla topic-ul de la care vrea clientul sa se dezaboneze
						char *token = strtok(buffer, "\n ");
						token = strtok(NULL, "\n ");

						for (int k = 0; k < n_all_topics; k++) {
							if (strstr(all_topics[k].topic, token)) {
								for(int j = 0; j < all_topics[k].n_subscribers; j++) {
									if(strcmp(all_topics[k].subscribers[j].subscriber->id, current_tcp_client.id) == 0) {
										// Clientul este scos din lista de subscriberi ai topic-ului;
										for(int p = j; p < all_topics[k].n_subscribers - 1; p++) {
											all_topics[k].subscribers[p] = all_topics[k].subscribers[p+1];
										}
										all_topics[k].n_subscribers--;
									}
								}
								break;
							} 
						}
					// Mesaj de subscribe de la client;
					} else if(strstr(buffer, "subscribe")) {
						//Se retine referinta la clientul TCP care vrea sa se aboneze;
						struct tcp_client *current_tcp_client;
						for (int k = 0; k < n_tcp_clients; k++)
							if (tcp_clients[k].current_socket == i) {
								current_tcp_client = &tcp_clients[k];
								break;
							}

						// Se prelucreaza input-ul pentru a sti ce topic si ce optiune de topic
						// a ales clientul;
						char *token = strtok(buffer, "\n ");
						token = strtok(NULL, "\n ");
						char new_topic[50];
						strcpy(new_topic, token);

						token = strtok(NULL, "\n ");
						int new_SF = token[0] - 48;

						int OK_subscribe = 1;
						// Daca topic-ul exista deja, clientul este adaugat in lista de subscriberi a acestuia;
						for (int k = 0; k < n_all_topics; k++) {
							if (strstr(all_topics[k].topic, new_topic)) {
								all_topics[k].subscribers[all_topics[k].n_subscribers].subscriber = current_tcp_client;
								all_topics[k].subscribers[all_topics[k].n_subscribers++].SF = new_SF;
								OK_subscribe = 0;
								break;
							} 
						}
						// Daca nu exista, se va inregistra pe server, iar primul abonat va fi clientul curent;
						if (OK_subscribe) {
							strcpy(all_topics[n_all_topics].topic, new_topic);
							int nr_subscribers = all_topics[n_all_topics].n_subscribers;
							all_topics[n_all_topics].subscribers[nr_subscribers].subscriber = current_tcp_client;
							all_topics[n_all_topics].subscribers[nr_subscribers].SF = new_SF;
							all_topics[n_all_topics].n_subscribers++;
							n_all_topics++;
						}
					}
					memset(buffer, 0, BUFLEN);
				}
			}
		}
	}
	close (tcp_sockfd);
	close(udp_sockfd);
	return 0;
}