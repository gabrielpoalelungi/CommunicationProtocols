321CC - Poalelungi Gabriel - Tema 2 PCom 2021

---------------------------------------------------------------------------------------------------------

I. Protocolul de nivel aplicatie implementat foloseste urmatoarele structuri:

struct tcp_clients {

	char id[10];
	unsigned short is_active;
	int current_socket;
	char unreceived[17][1551];
	int nr_unreceived;

};

	- O variabila de tip tcp_client retine:
		- ID-ul clientului;
		- Daca este activ in momentul curent sau nu;
		- Mesajele primite de la abonamentele cu SF = 1 cat timp acesta este deconectat;
---------------------------------------------------------------------------------------
struct tcp_subscriber {
	struct tcp_client* subscriber;
	int SF;
};

	- O variabila de tip tcp_subscriber retine:
		- Detaliile clientului TCP;
		- Tipul de abonament pe care l-a ales (SF or NO_SF);
----------------------------------------------------------------------------------------
struct topic {
	char topic[50];
	struct tcp_subscriber subscribers[20];
	int n_subscribers;
};

	- O variabila de tip topic retine:
		- Numele topic-ului;
		- Vectorul de abonati de tip tcp_subscribers;
----------------------------------------------------------------------------------------
struct udp_message {
	char topic[50];
	uint8_t tip_date;
	char payload[1500];
};

	- O variabila de tip udp_message retine:
		- Numele topic-ului;
		- Tipul de mesaj al topic-ului;
		- Mesajul propriu-zis;
-----------------------------------------------------------------------------------------

II. Ce date retine server-ul?

	- Un vector de clienti TCP inregistrati in server (vector de (struct tcp_client));

	- Un vector de topic-uri retinute de server (vector de (struct topic));

-----------------------------------------------------------------------------------------

III. Modul de functionare al protocolului de nivel aplicatie este urmatorul:

	- Unui client care doreste sa se conecteze ii va fi acceptata conexiunea.

	- Server-ul intreaba clientul ce ID are (trimitand mesajul "ID_CLIENT?"), iar clientul ii raspunde trimitand mesajul "ID_CLIENT: <id>".

	- Daca clientul are un ID cu al unui altui client deja conectat, conectarea (care deja a fost acceptata) ii va fi intrerupta, iar in server se va afisa
	cine este deja conectat.

	- Clientul este inregistrat in server, iar server-ul va afisa un mesaj corespunzator "New client <client> connected from <IP>:<PORT>.".

	- Daca clientul este deja inregistrat, la conectare ii vor fi afisate mesajele primite de la abonamentele cu optiunea SF cat timp acesta era deconectat.

	- Daca un client doreste sa se deconecteze, acesta va trimite server-ului un mesaj de tipul "Client <id> disconnected.", iar server-ul va sti sa il deconecteze.

	- Daca un client doreste sa se aboneze la un anumit topic, clientul va trimite un input serverului de tipul "subscribe <topic> <SF>", iar serverul va prelucra comanda si il va inregistra ca abonat la topicul <topic>. Dupa ce comanda a fost trimisa, in fereastra clientului va fi afisat mesajul "Subscribed to topic.".

	- Analog, pentru dezabonare, server-ul il va scoate pe client din lista de abonati ai topicului, iar in client va fi afisat mesajul "Unsubscribed from topic.".

	- Daca server-ul primeste mesaj de la un client UDP, server-ul il va prelucra conform cerintei si il va trimite tuturor abonatilor si/sau il va salva in memoria proprie pentru clientii care sunt deconectati si care au ales abonament de tip SF.
		- Fiecare trimitere se continua de abia dupa ce clientul care a primit acel mesaj trimite un ACK. Astfel, nu se pierd mesajele daca debitul este prea mare;

	- Daca server-ul primeste comanda "exit", acesta va inchide intai toate conexiunile cu clientii si va trimite cate un mesaj de tipul "close client" fiecarui client pentru a initia in fiecare inchiderea.