#include <WS2tcpip.h>
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#pragma comment (lib,"ws2_32.lib")

#define MAX_ID_NUM 200

#define DNS_SERVER_PORT 53

#define MAX_BUFFER_SIZE 512

#define MAX_QUERY_TIME 5

SOCKET sock; //udp socket
SOCKET upper_sock;

//0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
/*-------------------------------------*/
/*                  ID                                */
/*-------------------------------------*/
/*                 FLAGS                         */
/*-------------------------------------*/
/*                QDCOUNT                  */
/*-------------------------------------*/
/*                ANCOUNT                 */
/*-------------------------------------*/
/*                NSCOUNT                */
/*-------------------------------------*/
/*                ARCOUNT                  */
/*-------------------------------------*/


typedef struct {
	unsigned short ID;		
	unsigned short FLAGS;		//16bit
	unsigned short QDCOUNT;		 
	unsigned short ANCOUNT;
	unsigned short NSCOUNT;
	unsigned short ARCOUNT;
}Dns_Header;

//0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
/*-------------------------------------*/
/*                NAME						  */
/*-------------------------------------*/
/*                TYPE							 */
/*-------------------------------------*/
/*                CLASS						    */
/*-------------------------------------*/
/*                TTL							      */
/*													   */
/*-------------------------------------*/
/*               RDLENGTH			     */
/*-------------------------------------*/
/*                RDATA						    */
/*-------------------------------------*/


typedef struct
{
	unsigned short NAME;//name
	unsigned short TYPE;//RRtype
	unsigned short CLASS;//usually set as 1 - internet data
	unsigned short TTL_1;//for byte alignment rules in the memory, I need to set two TTL segment;
	unsigned short TTL_2;//time to live
	unsigned short DATALENGTH;//length
	unsigned long RDATA;//actual data
}Resource_Record;

typedef struct{
	char URL[256];
	unsigned short origin_id;
	struct sockaddr_in client;	
	int original_length;
	time_t added_time;
}id_node;

id_node* id_table[MAX_ID_NUM];

struct HashNode{	
	char url[256];				//Old DNS can only support URL of length 255 at most.
	char ipv4[16];				//***.***.***.***+EOF at most.
	time_t expire_time;			//-1 in the id node from dnsrelay.txt, while others will write its expire time
	struct HashNode *next;
};

struct map{
	int size;
	struct HashNode *table;
};

struct map *hashmap;

int debug_mode;				//锟斤拷锟斤拷模式
char upper_dns[32];
char file_dir[100];
int recv_ret;
char IP[16];
time_t exp_time;

enum{FOUND = 1, BLOCKED, NOT_FOUND};

/* About the hash map of local dnstable */
void init_hosts_file();

int get_hash(char temp_url[]);

int find_in_local(char* domain, char* res);

void add_item(char *url, char *ipv4, time_t expire_time);

void print_hashtable(void);

/* About the service of local and remote dns servers. */
void deal_with_query(Dns_Header* header, char* buffer, struct sockaddr_in *clientAddr);

void deal_with_response(Dns_Header* header, char* buffer);

int get_Qname(char* domain, Dns_Header* header, char* buffer);

void change_to_char_arr(unsigned long input, char *IP);

void ret_packet_to_log(char send_buffer[], int original_len, char url[]);

void check_timeout();

void debug(char *string, int level);

/* Check and parse the input params. */
int check_input(int argc, char *argv[]);