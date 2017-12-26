#ifndef CLIENT_H
#define CLIENT_H

//Struct for the server
typedef struct server_st {
	struct sockaddr_in addr;
	int addr_len;
	int fd;
} Server;

//Struct for one cell of the playing field
typedef struct { 
  char State; 
  char player_color; 
  long int x; 
  long int y; 
} Cell; 

void diep(char *s);

void create_socket(Server *s);

void send_data(Server *s, char *buf);

void get_data(Server *s, char *buf, char* p);

Cell* parse_data(char *data);

char* get_my_id(Server *s);

void sort_data(Cell* list, char *data);

int co_compare(const void *a, const void *b);

#endif
