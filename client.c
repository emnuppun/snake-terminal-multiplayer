#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "client.h"

#define BUFLEN 1024
#define PORT 9596

#define HEIGHT 20
#define WIDTH 80

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"


// Error handling
void diep(char *s)
{
   	perror(s);
   	exit(1);
}

//Creates a socket for the server
void create_socket(Server *s)
{
	struct sockaddr_in si_other;
	int slen=sizeof(si_other);
	int fd;

	if ((fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
		diep("socket");
	s->fd = fd;
	memset((char *) &si_other, 0, slen);
	si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);
	s->addr = si_other;
	s->addr_len = slen;
}

//Send data from "buf" to server 
void send_data(Server *s, char *buf)
{
	if (sendto(s->fd, buf, strlen(buf), 0, (struct sockaddr *)&(s->addr), s->addr_len) == -1)
		diep("sendto");
}

/*Get data from the server and save it to buffer 
and if "p" is "print" prints the received data*/
void get_data(Server *s, char *buf, char* p)
{
	int recvlen;
	struct timeval tv;
	tv.tv_sec = 15;
	tv.tv_usec = 0;
	if (setsockopt(s->fd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    	diep("setsockopt()");
	}
	recvlen = recvfrom(s->fd, buf, BUFLEN, 0, NULL, NULL);
	if (recvlen >= 0) {
		if (!strcmp(p, "print"))
   			printf("\n%s\n", buf);
	}
}

/*Parses the string from data, six chars equal one Cell struct*/
Cell* parse_data(char *data) 
{
	int length = strlen(data)/6;
	Cell *list = malloc(length * sizeof(Cell));
	char x[3];
	long int x1;
	long int y1;
	char y[3];
	for (int i = 0; i < length; i++){ 
	    char *part = calloc(7, sizeof(char)); 
	    strncpy(part, data, 6);
	    x[0] = part[2];
	    x[1] = part[3];
	    x[2] = 0;
	    x1 = strtol(x, 0, 10);
	    y[0] = part[4];
	    y[1] = part[5];
	    y[2] = 0;
	    y1 = strtol(y, 0, 10);
	    list[i].State = part[0]; 
	    list[i].player_color = part[1]; 
	    list[i].x = x1; 
	    list[i].y = y1; 
	    data += 6;
		free(part); 
  }
  return list;
}
// Comparing for the qsort function
int co_compare(const void *a, const void *b)
{
    const Cell *cell_a = a;
    const Cell *cell_b = b;

    if (cell_a->y < cell_b->y)
        return 0;
    else if (cell_a->y == cell_b->y){
    	if(cell_a->x <= cell_b->x)
    		return 0;
    	else
    		return 1;
    }
    else
        return 1;
}

//Sorting list of struct Cells by their coordinates
void sort_data(Cell* list, char *data)
{
	qsort(list, strlen(data)/6, sizeof(Cell), co_compare);
}

//Prints the playing field
void print_field(Cell* list)
{
	int l_i = 0;
	for (int i = 0; i < HEIGHT; i++) {
		for (int j = 0; j < WIDTH; j++) {
			if (i == 0 || j == 0 || i == HEIGHT-1 || j == WIDTH-1){
				printf("%s*", KNRM);
			}
			else if (i == list[l_i].y && j==list[l_i].x) {
				if (list[l_i].player_color == 'R')
					printf("%s", KRED);
				else if (list[l_i].player_color == 'G')
					printf("%s", KGRN);
				else if (list[l_i].player_color == 'Y')
					printf("%s", KYEL);
				else if (list[l_i].player_color == 'B')
					printf("%s", KBLU);
				else if (list[l_i].player_color == 'M')
					printf("%s", KMAG);
				else if (list[l_i].player_color == 'C')
					printf("%s", KCYN);
				else if (list[l_i].player_color == 'N')
					printf("%s", KNRM);
				if (list[l_i].State == 'H')
					printf("@");
				else if (list[l_i].State == 'C')
					printf("+");
				else
					printf("o");
				l_i++;
			}
			else
				printf(" ");
		}
		printf("\n");
	}
}

//Returns player id from the data from the server
char* get_my_id(Server *s) {
	char *buf = calloc(BUFLEN, 1);
	get_data(s, buf, "not");
	if (strlen(buf) == 1) {
		char* id = buf;
		return id;
	}
	free(buf);
	return NULL;
}
//Return the color of the player's worm from the data from the server
char* get_my_color(Server *s)
{
	char *buf = calloc(BUFLEN, 1);
	get_data(s, buf, "not");
	char* color = calloc(10, 1);
	if (strlen(buf) == 1) {
		switch(buf[0]){

		case 'R':
		strcpy(color, "red");
		break;

		case 'G':
		strcpy(color, "green");
		break;

		case 'Y':
		strcpy(color, "yellow");
		break;

		case 'B':
		strcpy(color, "blue");
		break;

		case 'M':
		strcpy(color, "magenta");
		break;

		case 'C':
		strcpy(color, "cyan");
		break;
		}
	}
	free(buf);
	return color;
}

int main(void)
{
    char *s_buf = calloc(BUFLEN, 1);
	char *g_buf = calloc(BUFLEN, 1);
	char *color;
	char *begin_str = "GAME BEGINS";
	char *end_str = "quit";
	char *moves = calloc(10, 1);
	Cell *list;
	
    Server *s = calloc(1, sizeof(Server));
	create_socket(s);
	printf("Please name your worm (max 20 characters):\n");
    scanf("%s", s_buf);
	send_data(s, s_buf);
	s_buf[0] = 0;
	char *id;
	id = get_my_id(s); 
	color = get_my_color(s);
	
	while (strcmp(g_buf, begin_str) != 0) {
		if (strlen(s_buf)) {
			send_data(s, s_buf);
			memset(s_buf, 0, BUFLEN);
			memset(moves, 0, 10);
		}
		get_data(s, g_buf, "print");
	}
	while (1) {
		get_data(s, g_buf, "not");
		if (!strcmp(g_buf, end_str))
			break;
		list = parse_data(g_buf);
    	sort_data(list, g_buf);
    	print_field(list);
		printf("Your color is %s, guess which one!\n", color);
		printf("w - up, a - left, s - down, d - right\nWe don't have forever so think fast and\ngive your move:\n");
		scanf("%s", moves);
		strcpy(s_buf, id);
		strcat(s_buf, moves);
		printf("%s\n", s_buf);
		send_data(s, s_buf);
		memset(s_buf, 0, BUFLEN);
		memset(moves, 0, 10);
		free(list);
		
	}
    printf("GAME OVER!!\n");
	free(s_buf);
	free(g_buf);
	free(color);
	free(moves);
	free(id);
    close(s->fd);
	free(s);
    return 0;
}
