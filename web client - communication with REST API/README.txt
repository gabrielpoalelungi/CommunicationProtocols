321CC - POALELUNGI Gabriel - Tema 3 PCom 2021

Se pot face 3 tipuri de Request-uri: GET, POST, DELETE;

1) char *compute_get_request(char *host, char *url, char *query_params,
							char **cookies, int cookies_count, char *jwt_token)
	- Se creeaza o cerere de tip GET;
	- host : ip-ul server-ului pe care il accesam;
	- url : url-ul care aplicatia ce se afla pe host;
	- query_params: parametrii aditionali care se ataseaza url-ului (in cazul nostru, vom atasa id-ul unei carti);
	- cookies: lista de cookie-uri (poate fi NULL);
	- cookies_count: cate cookie-uri sunt;
	- jwt_token: token-ul jwt (poate fi NULL);

2) char *compute_delete_request(char *host, char *url, char *query_params,
							char **cookies, int cookies_count, char *jwt_token)

	- Asemenea lui 1), doar ca se face o cerere de tip DELETE;

3) char *compute_post_request(char *host, 
								char *url, 
								char* content_type, 					char **body_data,
								int body_data_fields_count, 
								char **cookies, 
								int cookies_count, 
								char *jwt_token)
	- body_data: lista de payload-uri ce va fi trimisa catre host;
	- body_data_fields_count: cate payload-uri sunt;
	- Se creeaza o cerere de tip POST;


Clientul poate executa urmatoarele comenzi:

"help":
	- La executarea comenzii, va aparea o lista cu comenzi posibile;

1) "register":

	- Apar 2 prompt-uri, unul pentru username si altul pentru parola;

	- Dupa ce au fost introduse, se elimina "\n" din fiecare;

	- Se creeaza o variabila JSON cu cele 2 obiecte "username" si 
	"password" astfel:
		- Utilizand biblioteca "parson", se creeaza o variabila JSON vida cu "json_value_init_object()";
		- Se preia lista de obiecte din variabila cu "json_value_get_object(variabila_JSON)";
		- Se seteaza cele 2 obiecte necesare cu functia "json_object_set_string(root_object, "nume_obiect", valoare_asociata);"

	- Se converteste variabila JSON intr-un string cu ajutorul functiei json_serialize_to_string_pretty(variabila_JSON);

	- Se creeaza request-ul de tip GET la fel ca in laboratorul 10;

	- Se afiseaza raspunsul HTTP si se afiseaza si un mesaj de succes (in cazul in care este succes);

	- Daca apare o eroare, se va vedea in raspunsul HTTP;


2) "login":

	- Se verifica daca e cineva logat deja (variabila isSomeoneLoggedIn arata acest lucru: =1 (e cineva logat), =0 (nu e nimeni logat)). In caz afirmativ, se va afisa un mesaj sugestiv care nu permite logare noua in timp ce un cont este logat in acelasi timp;

	- Se creeaza o variabila JSON asemenea celei de la register;

	- Se creeaza un POST Request;

	- Din raspuns se extrage cookie-ul cu ajutorul functiei strtok si se stocheaza in variabila main-ului "current_cookie_session";

	- Se afiseaza raspunsul HTTP;

	- Se afiseaza un mesaj de succes (in cazul in care este succes). In caz contrar, eroarea va fi inclusa in raspunsul HTTP;

3) "enter_library":

	- Daca nu este logat nimeni, nu va fi permis accesul in biblioteca (se verifica daca isSomeoneLoggedIn este 0 sau 1);

	- Se creeaza GET Request-ul;

	- Se extrage token-ul JWT din raspunsul HTTP si se stocheaza in variabila main-ului "current_JWT";

	- Se afiseaza un mesaj de succes. In caz contrar, aceste se va vedea in raspunsul HTTP;

4) "get_books":

	- Nu este permisa actiunea daca nu se demonstreaza accesul in biblioteca (prin variabila isSomeoneInLibrary);

	- Se creeaza un GET Request;

	- Se afiseaza raspunsul;

	- Se extrage lista de carti in format JSON;

	- Se face o conversie in variabila JSON si inapoi la string pentru a aerisi afisarea prin cele 2 functii: json_parse_string(books) si json_serialize_to_string_pretty(root_value);

5) "get_book":

	- Nu este permisa actiunea daca nu se demonstreaza accesul in biblioteca (prin variabila isSomeoneInLibrary);

	- Se creeaza un GET Request cu id-ul citit de la tastatura dat ca parametru;

	- Se afiseaza raspunsul (detaliile cartii interogate se afla in format JSON in interiorul raspunsului HTTP);

6) "add_book":

	- Nu este permisa actiunea daca nu se demonstreaza accesul in biblioteca (prin variabila isSomeoneInLibrary);

	- Se citesc de la tastatura datele cerute, se memoreaza si sunt convertite in obiecte JSON la fel ca la "register" sau "login";
		- Pentru page_count, se foloseste functia json_object_set_number(variabila_JSON, "page_count", page_count);

	- Se creeaza un string prin conversia JSON-ului la string;

	- Se creeaza POST Request-ul;

	- Se afiseaza raspunsul HTTP si se afiseaza si un mesaj de succes (in cazul in care este succes);

	- Daca apare o eroare, se va vedea in raspunsul HTTP;

7) "delete_book":

	- Nu este permisa actiunea daca nu se demonstreaza accesul in biblioteca (prin variabila isSomeoneInLibrary);

	- Se citeste de la tastatura id-ul cartii ce urmeaza sa fie sterse;

	- Se creeaza un DELETE Request cu id-ul citit;

	- Se afiseaza raspunsul HTTP si se afiseaza si un mesaj de succes (in cazul in care este succes);

	- Daca apare o eroare, se va vedea in raspunsul HTTP;

8) "logout":

	- Se creeaza GET Request-ul;

	- Se afiseaza raspunsul HTTP si se afiseaza si un mesaj de succes (in cazul in care este succes);

	- Daca apare o eroare, se va vedea in raspunsul HTTP;

	- Odata cu mesajul de succes, se reseteaza cookie-ul curent, token-ul JWT curent si variabilele isSomeoneLoggedIn si isSomeoneInLibrary;

9) "exit":
	
	- Se inchide programul;

