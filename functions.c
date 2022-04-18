
#include "DnsHeader.h"
#include <time.h>

extern int recv_ret;//the length of received datagram

int question_size = 0;

void deal_with_query(Dns_Header* header, char* buffer, struct sockaddr_in *clientAddr)
{
	char domain_name[256] = "";
	int name_ret = get_Qname(domain_name, header, buffer);
	if (name_ret == 0)
	{
		debug("Failed to get the domain name.\n", 1);
		exit(1);
	}
	char string[256];
	sprintf(string, "get the domain successfully! it's %s\n", domain_name);
	debug(string, 1);
	char res_buffer[30];	//save the ip searched

	unsigned short query_type;
	memcpy(&query_type, buffer + recv_ret - 4, sizeof(unsigned short));
	int find_ret = 0;
	if(query_type == htons(0x0001))
	{
		find_ret = find_in_local(domain_name, res_buffer); //find_ret 1 found 2 blocked 3 didn't find
	}
	else
	{
		find_ret = 3;
	}
	
	switch (find_ret)
	{
	case FOUND:
	{
		memset(string, 0, sizeof(string));
		sprintf(string, "recv length = %d\n", recv_ret);
		debug(string, 1);
		//construct the success datagram
		char send_buffer[MAX_BUFFER_SIZE] = "";
		memset(send_buffer, 0, sizeof(send_buffer));
		// memcpy(send_buffer, 0, sizeof(send_buffer));
		//duplicate just change some message
		memcpy(send_buffer, buffer, recv_ret * sizeof(char));
		//chenge the flag -  16bit
		// printf("pre flag %u\n",header->FLAGS);
		header->FLAGS = htons(0x8180);
		// printf("now flag %u\n", header->FLAGS);
		memcpy(&send_buffer[2], &header->FLAGS, sizeof(unsigned short));
		// int a = 0;
		// a = header->FLAGS;
		// a = ntohs(a);
		// a = a >> 15;
		// printf("QR segment :%d\n", a);
		//change the answer count to 1
		unsigned short ancount = htons(0x0001);
		memcpy(&send_buffer[6], &ancount, sizeof(short));
		
		//for v6
		unsigned short temp_type;
		memcpy(&temp_type, buffer + recv_ret - 2, sizeof(unsigned short));
		


		//add ansewer after header
		Resource_Record rr;
		//pointer - point to the domain as a offset
		rr.NAME = htons(0xc00c);
		//type usually be 1 (IPv4) - host address
		rr.TYPE = htons(0x0001);
		//class ususally be IN - indicate to internet data
		rr.CLASS = htons(0x0001);
		//time to live 410 - 6 minutes, 50 seconds
		rr.TTL_1 = htons(0x1);
		rr.TTL_2 = htons(0x9a);
		//length of ip just ipv4 32bit and that's 4 bytes - ** the segment is counting in bytes.
		rr.DATALENGTH = htons(0x0004);
		//ip
		rr.RDATA = inet_addr(res_buffer);

		//insert into sendbuffer
		memcpy(send_buffer+recv_ret, &rr, sizeof(Resource_Record));
		
		char *real_send = (char*)malloc(recv_ret + sizeof(Resource_Record));
		memcpy(real_send, send_buffer, recv_ret + sizeof(Resource_Record));
		int send_ret = sendto(sock, real_send, recv_ret + sizeof(Resource_Record), 0, (struct sockaddr*) clientAddr, sizeof(*clientAddr));
		if(send_ret == -1)
		{
			debug("Send response failed.\n", 2);
			exit(1);
		}
		memset(string, 0, sizeof(string));
		sprintf(string, "Send response successfully. Response ip is %s\n", res_buffer);
		debug(string, 2);
	}
		break;
	case BLOCKED:
	{
		//blocked
		char send_buffer[MAX_BUFFER_SIZE] = "";
		// memcpy(send_buffer, 0, sizeof(send_buffer));
		//duplicate just change some message
		memcpy(send_buffer, buffer, recv_ret * sizeof(char));
		//chenge the flag -  16bit
		// printf("pre flag %u\n",header->FLAGS);
		header->FLAGS = htons(0x8185);
		// printf("now flag %u\n", header->FLAGS);
		memcpy(&send_buffer[2], &header->FLAGS, sizeof(unsigned short));
		int send_ret = sendto(sock, send_buffer, recv_ret, 0, (struct sockaddr*) clientAddr, sizeof(*clientAddr));
		if(send_ret == -1)
		{
			debug("Send response failed.\n", 2);
			exit(1);
		}
		debug("Send response successfully. The domain is blocked.\n", 2);
	}
		break;	
	case NOT_FOUND:
	{
		//to upper dns server
		srand(time(NULL));
		unsigned short now_id = header->ID;
		unsigned short new_id = rand() % MAX_ID_NUM;
		memset(string, 0, sizeof(string));
		sprintf(string, "We did not find the request locally and it will be transferred. \n");
		debug(string, 2);
		while(id_table[new_id] != NULL)
		{
			new_id = rand() % MAX_ID_NUM;
		}
		memset(string, 0, sizeof(string));
		sprintf(string, "The old ID is %u, the new ID is %u.\n", now_id, new_id);
		debug(string, 1);
		id_node *node = (id_node*)malloc(sizeof(id_node));
		id_table[new_id] = node;
		node->origin_id = now_id;
		node->client = *clientAddr;
		node->original_length = recv_ret;
		node->added_time = time(NULL);
		//start loading the contents.
		header->ID = htons(new_id);

		char send_buffer[MAX_BUFFER_SIZE];

		memcpy(send_buffer, buffer, recv_ret);

		memcpy(&send_buffer[0], &header->ID, sizeof(unsigned short));
		
		// upper_sock = socket(AF_INET, SOCK_DGRAM, 0);
		struct sockaddr_in upper_server;
		memset(&upper_server, 0, sizeof(upper_server));
		upper_server.sin_family = AF_INET;
		upper_server.sin_addr.s_addr = inet_addr(upper_dns);
		upper_server.sin_port = htons(DNS_SERVER_PORT);
		/*
		int bindVal = bind(upper_sock, (struct sockaddr*)&upper_server, sizeof(upper_server));
		if(bindVal == SOCKET_ERROR)
		{
			printf("bind upper dns socket failed!\n");
			exit(2);
		}
		printf("bind upper dns socket successfully\n");
		*/
		int send_ret = sendto(sock, send_buffer, recv_ret, 0, (struct sockaddr*)&upper_server, sizeof(struct sockaddr));
		if(send_ret == -1)
		{
			debug("send to upper dns failed!\n", 2);
			exit(2);
		}
		debug("send to upper dns successfully\n", 1);
		//closesocket(upper_sock);
		strcpy(node->URL, domain_name);
	}
		break;
	default:
		break;
	}
}

void deal_with_response(Dns_Header* header, char* buffer)
{
	check_timeout();

	unsigned short new_id = header->ID;
	char string[256];
	sprintf(string, "received response, the ID is %d.\n", header->ID);
	debug(string, 2);
	id_node *ptr = id_table[new_id];
	if(ptr != NULL)
	{
		unsigned short origin = ptr->origin_id;
		struct sockaddr_in clientAddr = ptr->client;
		int original_len = ptr->original_length;
		char url[256];
		strcpy(url, ptr->URL);
		id_table[new_id] = NULL;
		free(ptr);

		header->ID = htons(origin);

		char send_buffer[MAX_BUFFER_SIZE];
		memcpy(send_buffer, buffer, recv_ret);
		memcpy(&send_buffer[0], &header->ID, sizeof(unsigned short));

		int send_ret = sendto(sock, send_buffer, recv_ret, 0, (struct sockaddr*)&clientAddr, sizeof(struct sockaddr));
		if(send_ret == -1)
		{
			debug("sending back to client dns failed!\n", 2);
			exit(2);
		}
		debug("Sending to client dns successfully.\n", 2);

		ret_packet_to_log(buffer, original_len, url);
	}
	else
	{
		debug("The ID is expired and has been deprecated.\n", 2);
	}
	
}

void check_timeout()
{
	time_t nowtime;
	for(int i = 0; i<MAX_ID_NUM; i++)
	{
		nowtime = time(NULL);
		if(id_table[i] != NULL)
			if(nowtime - id_table[i]->added_time > MAX_QUERY_TIME)
			{
				free(id_table[i]);
				id_table[i] = NULL;
			}
	}

	for(int i = 0; i < 2048; i++)
	{
		nowtime = time(NULL);
		struct HashNode *ptr = &(hashmap->table[i]), *prev = &(hashmap->table[i]);
		while (ptr != NULL)
        {
            if(ptr->expire_time > 0 && ptr->expire_time < nowtime)
			{
                prev->next = ptr->next;
				free(ptr);
				ptr = prev->next;
			}
			else
            	ptr=ptr->next;
			if(ptr != &(hashmap->table[i]))
				prev = prev->next;
        }
	}
}

void ret_packet_to_log(char send_buffer[], int original_len, char url[])
{
	Resource_Record rr;
	short Qtype;
	char* location = send_buffer + original_len;
	Dns_Header *temp_header = (Dns_Header*)malloc(sizeof(Dns_Header));
	memcpy(temp_header, send_buffer, sizeof(Dns_Header));
	int answer_count = temp_header->ANCOUNT;

	memcpy(&Qtype, location-4*sizeof(char), sizeof(Qtype));
	char string[256];
	printf(string, "Qtype %d, Answer_count %d.\n", ntohs(Qtype), answer_count);
	debug(string, 0);
	if(ntohs(Qtype) == 1 && answer_count > 0)
	{
		memcpy(&rr, location, sizeof(Resource_Record));
		memset(string, 0, sizeof(string));
		sprintf(string, "Resource record type: %u, datalength: %u\n", ntohs(rr.TYPE), ntohs(rr.DATALENGTH));
		debug(string, 0);
		memset(string, 0, sizeof(string));
		sprintf(string, "%d\n", location);
		debug(string, 0);
		while (ntohs(rr.TYPE) != 1)							//不是A的统统跳过 
		{ 
			location = location + 12 + ntohs(rr.DATALENGTH);
			memset(string, 0, sizeof(string));
			sprintf(string, "%d\n", location);
			debug(string, 0);
			memset(&rr, 0, sizeof(Resource_Record));
			memcpy(&rr, location, sizeof(Resource_Record));
			memset(string, 0, sizeof(string));
			sprintf(string, "Resource record type: %u, datalength: %u\n", ntohs(rr.TYPE), ntohs(rr.DATALENGTH));
			debug(string, 0);
		}
		change_to_char_arr(rr.RDATA, IP);
		exp_time = time(NULL) + rr.TTL_2 * 1<<8 + rr.TTL_1;
		add_item(url, IP, exp_time);
		//memset(URL, 0, sizeof(URL));
		memset(IP, 0, sizeof(IP));
		debug("Added to txt\n", 1);
	}
	free(temp_header);
}

int get_Qname(char* domain, Dns_Header* header, char* buffer)
{
	int offset = sizeof(Dns_Header);
	int later_bytes = 0;
	int index_d = 0;
	int i = offset;
	for (; buffer[i] != 0;)
	{
		later_bytes = buffer[i];
		
		for (int j = 1; j <= later_bytes; ++j)
		{
			domain[index_d++] = buffer[i + j];
			if (j == later_bytes)
			{
				i += j + 1;
				domain[index_d++] = '.';
			}
		}
	}
	index_d -= 1;
	domain[index_d] = '\0';
	char string[256]; 
	sprintf(string, "domain in the incoming datagram is %s\n", domain);
	debug(string, 1);
	return strlen(domain);
}

void change_to_char_arr(unsigned long input, char *IP)
{
	input = ntohl(input);
	sprintf(IP, "%d.%d.%d.%d", input/(1<<24), (input%(1<<24))/(1<<16), (input%(1<<16))/(1<<8), input%(1<<8)); 
}