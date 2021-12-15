#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

int main(int argc, char *argv[])
{
	char *message;
	char *response;
	int sockfd;

	char current_cookie_session[500];
	char current_JWT[500];
	char input_buffer[100];
	int isSomeoneLoggedIn = 0;
	int isSomeoneInLibrary = 0;

	puts("Welcome to our website!\n");
	puts("Type \"help\" for commands.\n");
	
	while(1) {
		fgets(input_buffer, 100, stdin);

		// Comanda de help pentru a vizualiza comenzile;
		if (strstr(input_buffer, "help")) {
			puts("register - create a new account.\n");
			puts("login - log in to the website with a created account.\n");
			puts("logout - log out of the website.\n");
			puts("enter_library - obtain access to the library.\n");
			puts("get_books - obtain a list of your registered books.\n");
			puts("get_book - obtain information about a specific book of your own.\n");
			puts("add_book - add a book to your list of books.\n");
			puts("delete_book - delete a book from your list of books.\n");
			puts("exit - exit the website.\n");
		}

		// Crearea unui cont nou;
		if (strstr(input_buffer, "register")) {

			// Memorarea username-ului si parolei;
			char username[100], password[100];
			printf("username=");
			fgets(username, 100, stdin);
			printf("password=");
			fgets(password, 100, stdin);
			// Stergerea new-line-ului de la sfarsitul fiecaruia;
			username[strcspn(username, "\n")] = 0;
			password[strcspn(password, "\n")] = 0;

			// Crearea payload-ului de tip JSON cu cele 2 obiecte,
			// username si parola;
			JSON_Value *root_value = json_value_init_object();
			JSON_Object *root_object = json_value_get_object(root_value);
			json_object_set_string(root_object, "username", username);
			json_object_set_string(root_object, "password", password);

			// Convertirea payload-ul de tip JSON intr-un payload de tip
			// string;
			char *payload = NULL;
			payload = json_serialize_to_string_pretty(root_value);


			// Crearea unui GET Request;
			char **body_data = calloc(LINELEN, sizeof(char*));
			int k = 0;
			body_data[k] = calloc(LINELEN, sizeof(char));
			strcpy(body_data[k], payload);
			k++;

			sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
			message = compute_post_request("34.118.48.238", "/api/v1/tema/auth/register",
				 "application/json", body_data, k, NULL, 0, NULL);
			puts(message);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			// Se afiseaza raspunsul;
			puts(response);
			close_connection(sockfd);

			json_free_serialized_string(payload);
			json_value_free(root_value);

			// Mesajul de succes (cel de eroare e inclus in raspuns);
			if (strstr(response, "201 Created")) {
				puts("New account created successfully!\n");
			}
		} else

		// Login cu un cont deja creat;
		if(strstr(input_buffer, "login")) {

			// In caz de exista un cont deja logat, nu permitem o noua 
			// logare pana la urmatorul logout;
			if (isSomeoneLoggedIn != 0) {
				puts("A user is already logged in!\n");
				puts("Log out first and then log in with a new account!\n");
				continue;
			}

			// Acelasi principiu ca la register
			char username[100], password[100];
			printf("username=");
			fgets(username, 100, stdin);
			printf("password=");
			fgets(password, 100, stdin);
			username[strcspn(username, "\n")] = 0;
			password[strcspn(password, "\n")] = 0;

			JSON_Value *root_value = json_value_init_object();
			JSON_Object *root_object = json_value_get_object(root_value);
			json_object_set_string(root_object, "username", username);
			json_object_set_string(root_object, "password", password);

			char *payload = NULL;
			payload = json_serialize_to_string_pretty(root_value);

			// Crearea unui POST Request;
			char **body_data = calloc(LINELEN, sizeof(char*));
			int k = 0;
			body_data[k] = calloc(LINELEN, sizeof(char));
			strcpy(body_data[k], payload);
			k++;

			sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
			message = compute_post_request("34.118.48.238", "/api/v1/tema/auth/login",
				 "application/json", body_data, k, NULL, 0, NULL);
			puts(message);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);

			// Extragerea Cookie-ului din raspunsul HTTP;
			// Se va stoca in variabila globala acestei functii
			// "current_cookie_session";
			char *aux_response = malloc(BUFLEN * sizeof(char));
			strcpy(aux_response, response);
			char *token = strtok(aux_response, "\r\n");
			int OK = 0;
			while(token != NULL) {
				if (strstr(token, "connect")) {
					strcpy(current_cookie_session, strstr(token, "connect"));
					OK = 1;
					break;
				} else {
					token = strtok(NULL, "\r\n");
				}
			}
			// Se afiseaza raspunsul;
			puts(response);
			close_connection(sockfd);

			json_free_serialized_string(payload);
			json_value_free(root_value);
			free(aux_response);

			// Mesajul de succes (cel de eroare e inclus in raspuns);
			if(OK == 1) {
				isSomeoneLoggedIn = 1;
				puts("You logged in!\n");
			}
		} else

		// Accesarea bibliotecii;
		if (strstr(input_buffer, "enter_library")) {
			// Nu este permisa nicio alta actiune pana nu se logheaza cineva;
			if (isSomeoneLoggedIn == 0) {
				puts("You haven't logged in yet!");
				continue;
			}

			// Crearea GET Request-ului;
			char **body_cookie = calloc(5, sizeof(char *));
			body_cookie[0] = calloc(LINELEN, sizeof(char));
			strcpy(body_cookie[0], current_cookie_session);
	
			sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
			message = compute_get_request("34.118.48.238", "/api/v1/tema/library/access", NULL, body_cookie, 1, NULL);
			puts(message);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			// Se afiseaza raspunsul;
			puts(response);

			// Se extrage token-ul JWT si se stocheaza in variabila
			// globala functiei "current_JWT";
			char *aux_response = malloc(BUFLEN * sizeof(char));
			strcpy(aux_response, response);
			char *token = strtok(aux_response, "\r\n");
			int OK = 0;
			while(token != NULL) {
				if (strstr(token, "{")) {
					char *aux = strtok(token, "\"");
					aux = strtok(NULL, "\"");
					if (strstr(aux, "error")) {
						break;
					}
					aux = strtok(NULL, "\"");
					aux = strtok(NULL, "\"");
					strcpy(current_JWT, aux);
					OK = 1;
					break;
				} else {
					token = strtok(NULL, "\r\n");
				}
			}

			// Mesajul de succes (cel de eroare e inclus in raspuns);
			if (OK == 1) {
				puts("\nWelcome to the library!\n");
				isSomeoneInLibrary = 1;
			}
			close_connection(sockfd);
		} else

		// Lista de carti inregistrate pe contul curent;
		if(strstr(input_buffer, "get_books")) {

			// Nu este permisa nicio alta actiune pana nu se demonstreaza
			// accesul in biblioteca;
			if (isSomeoneInLibrary == 0) {
				puts("You haven't entered the library yet!");
				continue;
			}

			// Crearea GET Request-ului;
			char **body_cookie = calloc(5, sizeof(char *));
			body_cookie[0] = calloc(LINELEN, sizeof(char));
			strcpy(body_cookie[0], current_cookie_session);

			sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
			message = compute_get_request("34.118.48.238", "/api/v1/tema/library/books", NULL, body_cookie, 1, current_JWT);
			puts(message);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			// Se afiseaza raspunsul;
			puts(response);

			// Extragerea listei de carti in format JSON;
			char *aux_response = malloc(BUFLEN * sizeof(char));
			strcpy(aux_response, response);

			char books[50];
			char *token = strtok(aux_response, "\r\n");
			while(token != NULL) {
				if (strstr(token, "[")) {
					strcpy(books, token);
					break;
				} else {
					token = strtok(NULL, "\r\n");
				}
			}

			// Afisarea ei mai aerisita, dar tot in format JSON;
			JSON_Value *root_value = json_parse_string(books);
			puts(json_serialize_to_string_pretty(root_value));

			close_connection(sockfd);

		} else

		// Afisarea detaliilor unei carti din colectia de carti a contului curent;
		if(strstr(input_buffer, "get_book")) {

			// Nu este permisa nicio alta actiune pana nu se demonstreaza
			// accesul in biblioteca;
			if (isSomeoneInLibrary == 0) {
				puts("You haven't entered the library yet!");
				continue;
			}
			char id[5];
			printf("id=");
			scanf("%s", id);

			// Crearea GET Request-ului;
			sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
			message = compute_get_request("34.118.48.238", "/api/v1/tema/library/books", id, NULL, 0, current_JWT);
			puts(message);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);

			// Se afiseaza raspunsul;
			puts(response);
			close_connection(sockfd);

		} else

		if (strstr(input_buffer, "add_book")) {

			// Nu este permisa nicio alta actiune pana nu se demonstreaza
			// accesul in biblioteca;
			if (isSomeoneInLibrary == 0) {
				puts("You haven't entered the library yet!");
				continue;
			}

			// Citirea datelor cartii ce urmeaza sa fie adaugate;
			char title[20], author[20], genre[20], publisher[20];
			int page_count;
			printf("title=");
			fgets(title, 20, stdin);
			printf("author=");
			fgets(author, 20, stdin);
			printf("genre=");
			fgets(genre, 20, stdin);
			printf("publisher=");
			fgets(publisher, 20, stdin);
			printf("page_count=");
			scanf("%d", &page_count);
			title[strcspn(title, "\n")] = 0;
			author[strcspn(author, "\n")] = 0;
			genre[strcspn(genre, "\n")] = 0;
			publisher[strcspn(publisher, "\n")] = 0;

			// Crearea obiectului JSON;
			JSON_Value *root_value = json_value_init_object();
			JSON_Object *root_object = json_value_get_object(root_value);
			json_object_set_string(root_object, "title", title);
			json_object_set_string(root_object, "author", author);
			json_object_set_string(root_object, "genre", genre);
			json_object_set_number(root_object, "page_count", page_count);
			json_object_set_string(root_object, "publisher", publisher);

			// Conversie JSON - string;
			char *payload = NULL;
			payload = json_serialize_to_string_pretty(root_value);


			// Crearea POST request;
			char **body_data = calloc(LINELEN, sizeof(char*));
			int k = 0;
			body_data[k] = calloc(LINELEN, sizeof(char));
			strcpy(body_data[k], payload);
			k++;

			sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
			message = compute_post_request("34.118.48.238", "/api/v1/tema/library/books", "application/json", body_data, k, NULL, 0, current_JWT);
			puts(message);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			puts(response);

			// Mesaj de succes (cel de eroare va fi afisat in raspuns);
			if(strstr(response, "200 OK") && strlen(current_JWT) != 0) {
				printf("The book was successfully added\n %s\n", payload);
			}

			close_connection(sockfd);
			json_free_serialized_string(payload);
			json_value_free(root_value);

		} else

		if(strstr(input_buffer, "delete_book")) {

			// Nu este permisa nicio alta actiune pana nu se demonstreaza
			// accesul in biblioteca;
			if (isSomeoneInLibrary == 0) {
				puts("You haven't entered the library yet!");
				continue;
			}
			char id[5];
			printf("id=");
			scanf("%s", id);

			// Crearea DELETE request;
			sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
			message = compute_delete_request("34.118.48.238", "/api/v1/tema/library/books", id, NULL, 0, current_JWT);
			puts(message);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			puts(response);

			// Mesaj de succes (cel de eroare va fi afisat in raspuns);
			if (strstr(response, "200 OK") && strlen(current_JWT) != 0) {
				printf("The book with the id %s has been deleted!\n", id);
			}
			close_connection(sockfd);

		} else

		if(strstr(input_buffer, "logout")) {

			// Crearea GET request;
			char **body_cookie = calloc(5, sizeof(char *));
			body_cookie[0] = calloc(LINELEN, sizeof(char));
			strcpy(body_cookie[0], current_cookie_session);
	
			sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
			message = compute_get_request("34.118.48.238", "/api/v1/tema/auth/logout", NULL, body_cookie, 1, NULL);
			puts(message);
			send_to_server(sockfd, message);
			response = receive_from_server(sockfd);
			puts(response);

			// Daca este totul OK, se reseteaza cine e logat, cine e intrat in biblioteca, cookie-urile si JWT-urile;
			if(strstr(response, "200 OK")) {
				puts("You logged out!\n");
				current_JWT[0] = '\0';
				current_cookie_session[0] = '\0';
				isSomeoneLoggedIn = 0;
				isSomeoneInLibrary = 0;
			}

			close_connection(sockfd);
		} else

		if (strstr(input_buffer, "exit")) {
			return 0;
		}
	}
	return 0;
}
