#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

// Structura ce defineste un client TCP conectat la server;
struct tcp_client {
	// ID-ul cu care e cunoscut in server;
	char id[10];

	// Este conectat sau deconectat?
	unsigned short is_active;

	// Socket-ul curent pe care acesta trimite date;
	int current_socket;

	// Mesajele primite se salveaza aici cat timp el nu este conectat;
	char unreceived[17][1551];
	int nr_unreceived;

};

// Structura ce defineste un abonat la un topic;
struct tcp_subscriber {
	// Referinta la clientul TCP conectat la server care este si abonat;
	struct tcp_client* subscriber;
	// Tipul de abonament pe care il are pentru un topic anume;
	int SF;
};

// Structura ce defineste un topic si abonatii acestuia;
struct topic {
	char topic[50];
	struct tcp_subscriber subscribers[20];
	int n_subscribers;
};

// Structura ce defineste un mesaj UDP;
struct udp_message {
	char topic[50];
	uint8_t tip_date;
	char payload[1500];
};

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN		1551	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	100	// numarul maxim de clienti in asteptare

#endif
