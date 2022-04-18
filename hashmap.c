#include "DnsHeader.h"

void init_hosts_file()
{
    FILE *fp = fopen(file_dir, "r");
    /* First we allocate the space of memory. 
        As the length of example is about 1000 lines, 
        we choose the space of the hashmap as 2048.
        It is also suitable for larger table size. */
    if(fp == NULL)
    {
        debug("No such dnsrelay file.\n", 2);
        exit(0);
    }
    debug("Open the map file successfully.\n", 1);
    hashmap = (struct map *)malloc(sizeof(struct map));
    memset(hashmap, 0, sizeof(struct map));
    hashmap->size = 2048;
    hashmap->table = (struct HashNode *)malloc(sizeof(struct HashNode)*hashmap->size);
    memset(hashmap->table, 0, sizeof(struct HashNode)*hashmap->size);
    char dns_line[300], temp_url[256], temp_ipv4[16];
    while (fgets(dns_line, 300, fp) != NULL)
    {
        memset(temp_url, 0, 256);
        memset(temp_ipv4, 0, 16);
        sscanf(dns_line, "%s %s", temp_ipv4, temp_url);
        int pos = get_hash(temp_url);
        if(*(hashmap->table[pos].url) == '\0')
        {
            //no conflict
            sscanf(dns_line, "%s %s", hashmap->table[pos].ipv4, hashmap->table[pos].url);
            hashmap->table[pos].expire_time = -1;
        }
        else
        {
            //conflict
            struct HashNode *temp, *loc;
            temp = (struct HashNode *)malloc(sizeof(struct HashNode));
            memset(temp, 0, sizeof(struct HashNode));
            sscanf(dns_line, "%s %s", temp->ipv4, temp->url);
            temp->expire_time = -1;
            loc = &(hashmap->table[pos]);
            while (loc->next != NULL)
                loc = loc->next;
            loc->next=temp;
        }
    }
    fclose(fp);
}

int get_hash(char temp_url[])
{
    int res = 0, loc = 0;
    while (temp_url[loc] != EOF && loc < 10)       //find any hash function you want as fast as possible
    {
        int temp = res << 5;
        res = temp - res;
        res = res + temp_url[loc];
        loc++;
    }
    return res&0x7FF;                               //%2048
}

int find_in_local(char *url, char *res)
{
    int hash = get_hash(url);
    //printf("%d\n", hash);
    struct HashNode *ptr = &(hashmap->table[hash]);
    while (ptr != NULL)
    {
        //printf("%d %s %s\n", strcmp(url, ptr->url), url, ptr->url);
        if(strcmp(url, ptr->url) == 0)
        {
            strcpy(res,ptr->ipv4);
            if(strcmp(res, "0.0.0.0") == 0)
                return 2;
            else
                return 1;
        }
        ptr = ptr->next;
    }
    strcpy(res, "");
    return 3;
}

void print_hashtable(void)              //debugger
{
    for(int i = 0; i < 2048; i++)
    {
        printf("%d:", i);
        struct HashNode *ptr = &(hashmap->table[i]);
        while (ptr !=NULL)
        {
            if(*(ptr->url) != '\0')
                printf("Key: %s, Value:%s.\t-->\t", ptr->url, ptr->ipv4);
            ptr=ptr->next;
        }
        printf("\n");
    }
}

void add_item(char *url, char *ipv4, time_t exp_time)
{
    /*
    FILE *fp = fopen(file_dir, "a");               //we do not maintain all input entries to text as there is TTL.
    fprintf(fp, "%s %s\n", ipv4, url);
    fclose(fp);
    */

    int pos = get_hash(url);
    if(*(hashmap->table[pos].url) == '\0')
    {
        //no conflict
        strcpy(hashmap->table[pos].url, url);
        strcpy(hashmap->table[pos].ipv4, ipv4);
        hashmap->table[pos].expire_time = exp_time;
    }
    else
    {
        //conflict
        struct HashNode *temp, *loc;
        temp = (struct HashNode *)malloc(sizeof(struct HashNode));
        memset(temp, 0, sizeof(struct HashNode));
        strcpy(temp->url, url);
        strcpy(temp->ipv4, ipv4);
        temp->expire_time = exp_time;
        loc = &(hashmap->table[pos]);
        while (loc->next != NULL)
            loc = loc->next;
        loc->next=temp;
    }
}