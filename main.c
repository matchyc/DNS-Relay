#include "DnsHeader.h"
#include <string.h>
#include <ctype.h>

#pragma comment (lib,"ws2_32.lib")

#define UPPER_DNS_ADDR "10.3.9.5"//origin dns server

#define DEFAULT_FILE_DIR ".\\dnsrelay.txt"

int main(int argc, char* argv[])
{
	printf(" __    _  _______  ___   __   __  _______  ______   __    _  _______ \n");
	printf("|  |  | ||   _   ||   | |  | |  ||       ||      | |  |  | ||       |\n");
	printf("|   |_| ||  |_|  ||   | |  |_|  ||    ___||  _    ||   |_| ||  _____|\n");
	printf("|       ||       ||   | |       ||   |___ | | |   ||       || |_____ \n");
	printf("|  _    ||       ||   | |       ||    ___|| |_|   ||  _    ||_____  |\n");
	printf("| | |   ||   _   ||   |  |     | |   |___ |       || | |   | _____| |\n");
	printf("|_|  |__||__| |__||___|   |___|  |_______||______| |_|  |__||_______|\n");
	printf("Welcome to NaiveDNS. Loading.....\n");
	if(check_input(argc, argv) != 0)
	{
		printf("Usage: dnsrelay [-d | -dd] [dns-server-ipaddr] [filename].\n");
		printf("dns-server-ipaddr should be ipv4 and filename should be absolute directory.\n");
		return 1;
	}
	
	init_hosts_file();
	
	WSADATA wasData;
	
	if (WSAStartup(MAKEWORD(2, 2), &wasData) != 0)
	{
		printf("WSAStartup failed!\n");
		exit(1);
	}

	sock = socket(AF_INET, SOCK_DGRAM, 0);

	if (sock == INVALID_SOCKET)
	{
		printf("Can't create a socket, check your setting\n");
		exit(0);
	}
	//create socket succefully!

	//server addr
	struct sockaddr_in servAddr;

	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = INADDR_ANY;
	servAddr.sin_port = htons(DNS_SERVER_PORT);
	int bindVal = bind(sock, (struct sockaddr*)&servAddr, sizeof(servAddr));

	if (bindVal == SOCKET_ERROR)
	{
		printf("Operation bind() failed!\n");
			exit(1);
	}

	//client address
	struct sockaddr clientAddr;
	int addrSize = sizeof(struct sockaddr);
	printf("%d\n",sizeof(Resource_Record));
	//receive buffer
	char buffer[MAX_BUFFER_SIZE] = "";
	//while - for listen
	printf("ready to enter 'while' to listen\n");
	while(1)
	{
		recv_ret = recvfrom(sock, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&clientAddr, &addrSize);
		if (recv_ret == SOCKET_ERROR)
		{
			//get message
			debug("recvfrom() failed!\n", 2);
			char temp[100] = "failed to recive data from you!";

			sendto(sock, temp, 32, 0, (struct sockaddr*)&clientAddr, sizeof(struct sockaddr));
		}
		else
		{
			debug("receive data successfully.\n",1);
			// puts(buffer);
			buffer[recv_ret] = '\0';

			//stuff the header
			Dns_Header *myHeader = (Dns_Header*)malloc(sizeof(Dns_Header));
			Dns_Header *temp = (Dns_Header*)buffer;//buffer
			//transfer
			myHeader->ID = ntohs(temp->ID);
			myHeader->FLAGS = ntohs(temp->FLAGS);
			char string[256];
			sprintf(string, "My header: %u, %u, %u, %u, %u\n", myHeader->ID, myHeader->FLAGS, myHeader->QDCOUNT, myHeader->ANCOUNT, myHeader->NSCOUNT, myHeader->ARCOUNT);
			debug(string, 0);
			//check for answer or query?
			if (((myHeader->FLAGS >> 15) & 1) == 0)
			{
				//QR == 0 query
				debug("A query datagram has come.\n", 2);
				deal_with_query(myHeader, buffer, (struct sockaddr_in*)&clientAddr);
			}
			else if (((myHeader->FLAGS >> 15) & 1) == 1)
			{
				//QR == 1 respond
				debug("A response datagram has come.\n", 2);
				deal_with_response(myHeader, buffer);
			}
			free(myHeader);
		}
	}

	closesocket(sock);
	WSACleanup();

	return 0;
}

int check_input(int argc, char *argv[])
{
	/*
	Here it works like this.
	argc can be 1 or 2 or 3 or 4.
	argv[0] must be dnsrelay.
	argv[1], argv[2], argv[3] are optional and can omit each of them.
	argv[1] - argv[3] can be in arbitrary order.
	We count matched argvs to make sure each input has its proper usage.
	*/
	int matched = 1;	//the number of matched argv with pattern
	int matched_loc[4] = {1, 0, 0, 0};
	strcpy(upper_dns, UPPER_DNS_ADDR);
    strcpy(file_dir, DEFAULT_FILE_DIR);
	if (argc > 4)
		return 1;
	else if (argc == 1)
		return 0;
	for(int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-dd") == 0)
		{
			debug_mode = 2;
			debug("More verbose debug.\n", 2);
			matched++;
			matched_loc[i] = 1;
			break;
		}
		else if (strcmp(argv[i], "-d") == 0)
		{
			debug_mode = 1;
			debug("Verbose debug.\n", 2);
			matched++;
			matched_loc[i] = 1;
			break;
		}
	}
	if(matched == argc)		//means 2 inputs, one dnsrelay and another -d/-dd
		return 0;
	/* Below we try to find the ip address. */
	int found_ip_flag = 0;
	for(int i = 1; i < argc; i++)
	{
		int len = strlen(argv[i]);
		int dot_count = 0; //check if ipv4
		for(int j = 0; j < len; j++)
		{
			if(isdigit(argv[i][j]))
			{
				if(j==len-1 && argv[i][j] != '.' && dot_count == 3)	//successfully reached the end where it must be a digit
					found_ip_flag = 1;
			}
			else if (argv[i][j] == '.')
			{
				dot_count++;
			}
			else
				break;		//neither . nor digit
		}
		if(found_ip_flag == 1)
		{
            memset(upper_dns, 0, sizeof(upper_dns));
			strcpy(upper_dns, argv[i]);
			char string[256];
			sprintf(string, "Set DNS server address:%s\n", upper_dns);
			debug(string, 2);
			matched++;
			matched_loc[i] = 1;
			break;
		}
	}
	if(matched == argc)		//means 3 inputs, one dnsrelay, another -d/-dd, the other ip address.
		return 0;	
    /* Below we try to find the location of the dnsrelay file. */
	else if (matched == argc-1)
	{
		for(int i = 1; i < argc; i++)
		{
			if(matched_loc[i] == 0)
			{
				FILE *fp = fopen(argv[i], "r");
				if(fp == NULL)
					return 1;
				else
				{
					fclose(fp);
                    memset(file_dir, 0, sizeof(file_dir));
					strcpy(file_dir, argv[i]);
					char string[256];
					sprintf(string, "set hosts file directory :%s\n", file_dir);
					debug(string, 2);
					return 0;
				}
			}
		}
	}
	return 1;
}

void debug(char *string, int level)
{
	if(level >= 2-debug_mode)
		printf("%s", string);
}