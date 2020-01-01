#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pcap.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <ctime>
#include <string>
#include <map>
#include <utility>

using namespace std;

typedef struct TO{
	struct TO* next;
	char addr;
	int count;
}to;

typedef struct FROM{
	struct FROM *next;
	to *first;
	char addr;
}from;

typedef pair<string, string> pss;
map<pss, int> mp;


pcap_t *inp;
struct pcap_pkthdr *header;
const u_char *content;
char errbuff[PCAP_ERRBUF_SIZE];

void printUsage();
void make_pair(char *, char*);


int main(int argc, char **argv)
{
	if(argc != 3) printUsage();

	if(strncmp(argv[1], "-r", 2)) printUsage();
	
	inp = pcap_open_offline(argv[2], errbuff);
	if(inp == NULL)
	{
		fprintf(stderr, "Open file \"%s\"failed!\n", argv[2]);
		return 0;
	}

	struct tm *rawtime;
	char out_time[1000];

	struct ether_header *eth;
	char addr[1000];

	string src, dst;
	
	while(pcap_next_ex(inp, &header, &content) == 1)
	{
		//timestamp
		rawtime = localtime(&(header->ts.tv_sec));
		strftime(out_time, 1000, "%Y-%m-%d %H:%M:%S UTC", rawtime);
		printf("Timestamp: %s\n\n", out_time);

		//source/dest
		eth = (struct ether_header *)content;
		
		printf("Source Address: ");
		for(int i = 0; i < ETHER_ADDR_LEN; i++)
		{
			if(i > 0) printf(":");
			printf("%x", eth -> ether_shost[i]);
		}
		printf("\n");

		printf("Destination Address: ");
		for(int i = 0; i < ETHER_ADDR_LEN; i++)
		{
			if(i > 0) printf(":");
			printf("%x", eth -> ether_dhost[i]);
		}
		printf("\n\n");

		//IP packet
		struct ip *ip = (struct ip *)(content + ETHER_HDR_LEN);
		const struct tcphdr *t;
		const struct udphdr *u;
		if(ntohs(eth->ether_type) == ETHERTYPE_IP)
		{
			printf("<IP Packet>\n");
			inet_ntop(AF_INET, &(ip->ip_src), addr, INET_ADDRSTRLEN);
			printf("Source: %s\n", addr);
			src = addr;
			inet_ntop(AF_INET, &(ip->ip_dst), addr, INET_ADDRSTRLEN);
			dst = addr;
			printf("Destination: : %s\n", addr);

			mp[{src, dst}]++;

			//TCP & UDP
			if(ip->ip_p == IPPROTO_TCP)
			{
				t = (struct tcphdr*)(content + sizeof(struct ip) + sizeof(struct ether_header));

				printf("<TCP>\n");
				printf("Source port: %u\n", ntohs(t->source));
				printf("Destination port: %u\n", ntohs(t->dest));
			}
			else if(ip->ip_p == IPPROTO_UDP)
			{
				u = (struct udphdr *)(content + sizeof(struct ip) + sizeof(struct ether_header));

				printf("<UDP>\n");
				printf("Source port: %u\n", ntohs(u->source));
				printf("Destination port: %u\n", ntohs(u->dest));
			}
		}

		struct ip6_hdr *ip6 = (struct ip6_hdr*)(content + ETHER_HDR_LEN);
		if(ntohs(eth->ether_type) == ETHERTYPE_IPV6)
		{
			printf("<IPV6 Packet>\n");

			inet_ntop(AF_INET6, &(ip6->ip6_src), addr, INET6_ADDRSTRLEN);
			printf("Source: %s\n", addr);
			src = addr;
			inet_ntop(AF_INET6, &(ip6->ip6_dst), addr, INET6_ADDRSTRLEN);
			dst = addr;
			printf("Destination: %s\n", addr);
			mp[{src, dst}]++;
		}


		printf("\n------------------------------\n\n");
	}


	printf("\n=====Analysis=================\n\n");
	for(pair<pss, int> p : mp)
	{
		printf("%s -> %s:\t\t%d\n", p.first.first.c_str(), p.first.second.c_str(), p.second);
	}
}

void make_pair(char *src, char *dest)
{
	
}

void printUsage()
{
	fprintf(stderr, "Useage: ./hw3 -r pcap_file\n");
	exit(0);
}
