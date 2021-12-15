#include <queue.h>
#include "skel.h"

struct route_table_entry *rtable;
int rtable_size;

struct arp_entry *arp_table;
int arp_table_len;


//Citirea tabelei de rutare si salvarea ei intr-un vector de route_table_entry
//Structura route_table_entry este declarata in include/skel.h
int read_route_table(struct route_table_entry *rtable, char *file_name) {
	char read_prefix[100], read_next_hop[100], read_mask[100];
	int read_interface;
	FILE *fp = fopen(file_name, "r");
	int index = 0;

	while (fscanf(fp, "%s %s %s %d", read_prefix, read_next_hop, read_mask, &read_interface) != EOF) {
		struct route_table_entry rt_entry;
		struct in_addr foo;

		//Prefix
		inet_aton(read_prefix, &foo);
		rt_entry.prefix = foo.s_addr;

		//Next-hop
		inet_aton(read_next_hop, &foo);
		rt_entry.next_hop = foo.s_addr;

		//Mask
		inet_aton(read_mask, &foo);
		rt_entry.mask = foo.s_addr;
		
		//Inteface
		rt_entry.interface = read_interface;

		*(rtable + index) = rt_entry;
		index++;
	}
	return index;
}

// Functie ce parseaza tabela ARP si o salveaza intr-un vector de arp-entry-uri
// Aceasta functie este luata din laboratorul 4 din skel.c
void parse_arp_table() 
{
	FILE *f;
	fprintf(stderr, "Parsing ARP table\n");
	f = fopen("arp_table.txt", "r");
	DIE(f == NULL, "Failed to open arp_table.txt");
	char line[100];
	int i = 0;
	for(i = 0; fgets(line, sizeof(line), f); i++) {
		char ip_str[50], mac_str[50];
		sscanf(line, "%s %s", ip_str, mac_str);
		fprintf(stderr, "IP: %s MAC: %s\n", ip_str, mac_str);
		arp_table[i].ip = inet_addr(ip_str);
		int rc = hwaddr_aton(mac_str, arp_table[i].mac);
		DIE(rc < 0, "invalid MAC");
	}
	arp_table_len = i;
	fclose(f);
	fprintf(stderr, "Done parsing ARP table.\n");
}

// Functie ce returneaza cel mai specific next-hop pentru adresa ip destinatie data ca parametru;
// Am facut aceasta functie la laboratorul 4;
struct route_table_entry *get_best_route(__u32 dest_ip) {

	struct route_table_entry *best_entry = NULL;
	for (int i = 0; i < rtable_size; i++) {
		if ((rtable[i].mask & dest_ip) == rtable[i].prefix) {
			if (best_entry == NULL)
				best_entry = &rtable[i];
			else
				if (rtable[i].mask > best_entry->mask)
					best_entry = &rtable[i];
		}
	}
	return best_entry;
}

// Functie ce returneaza adresa MAC corespunzatoare adresei IP destinatie;
struct arp_entry *get_arp_entry(__u32 ip) {

	for (int i = 0; i < arp_table_len; i++){
		if(arp_table[i].ip == ip)
			return &arp_table[i];
	}
    return NULL;
}

int main(int argc, char *argv[])
{
	packet m;
	int rc;

	init(argc - 2, argv + 2);

	// Alocare de memorie pentru vectorul de elemente route_table_entry;
	rtable = malloc(sizeof(struct route_table_entry) * 70000);
	rtable_size = read_route_table(rtable, argv[1]);

	// Alocare de memorie pentru vectorul de elemente arp_entry;
	arp_table = malloc(sizeof(struct arp_entry) * 100);
	parse_arp_table();

	while (1) {
		rc = get_packet(&m);

		struct ether_header *eth_hdr = (struct ether_header *)m.payload;
		struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
		struct icmphdr *icmp_hdr = parse_icmp(m.payload);


		// Vericarea sumei de control;
		// Daca nu corespunde, se da discard pachetului;
		if (ip_checksum(ip_hdr, sizeof(struct iphdr)) != 0){
			continue;
		}

		// Verifica daca este un ICMP Request;
		if (ip_hdr -> daddr == inet_addr (get_interface_ip (m.interface))) {
			// Daca este echo request;
			if (icmp_hdr -> type == 8) {
				// Trimite echo reply;
				send_icmp(ip_hdr-> daddr, ip_hdr -> saddr, eth_hdr -> ether_shost, eth_hdr -> ether_dhost, 0, 0, m.interface, ip_hdr -> id, ip_hdr -> id);
				continue;
			}
		}

		// Verificare TTL-ului >=1;
		// In caz contrar, se trimite un ICMP - Time exceeded si se da discard pachetului;
		if (ip_hdr->ttl <= 1){
			send_icmp_error(ip_hdr-> daddr, ip_hdr -> saddr, eth_hdr -> ether_shost, eth_hdr -> ether_dhost, 11, 0, m.interface);
			continue;
		}

		// Se cauta cea mai buna ruta pentru pachet;
		// In caz contrar, se trimite un ICMP - Destination unreachable si se da discard pachetului;
		struct route_table_entry *best_entry = get_best_route(ip_hdr->daddr);
		if (best_entry == NULL){
			send_icmp_error(ip_hdr-> daddr, ip_hdr -> saddr, eth_hdr -> ether_shost, eth_hdr -> ether_dhost, 3, 0, m.interface);
			continue;
		}

		// Se updateaza TTL-ul;
		// Se recalculeaza suma de control;
		ip_hdr->ttl -= 1;
		ip_hdr->check = 0;
		ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));

		// Se returneaza adresa MAC corespunzatoare adresei ip destinatie;
		struct arp_entry *next_arp_entry = get_arp_entry(ip_hdr->daddr);

		// Daca rezultatul este null, se da discard pachetului;
		if (next_arp_entry == NULL)
			continue;

		// Se updateaza adresele MAC destinatie si sursa;
		for (int i = 0; i < 6; i++){
			(eth_hdr->ether_dhost)[i] = (next_arp_entry->mac)[i];
		}
		get_interface_mac(best_entry->interface, eth_hdr->ether_shost);

		// Trimite pachetul pe interfata corespunzatoare celei mai bune rute;
		send_packet(best_entry->interface, &m);
	}
}
