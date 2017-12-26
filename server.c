#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "time.h"

#include "game.h"
#include "server.h"
#include "queue.h"

#define PORT 9596
#define BUFLEN 1024

// Error handling
void diep(char *s)
{
	perror(s);
	exit(1);
}

//Creates a socked and returns it
int create_socket(void)
{
	int sock_fd;
	struct sockaddr_in my_addr;
	if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		diep("socket");
	
	memset((char *) &my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(PORT);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock_fd, (const struct sockaddr*)&my_addr, sizeof(my_addr)) < 0)
        diep("bind");
		
	return sock_fd;
}

//Sends data from buf to all players
void send_all(World *w, char *buf) 
{
	Player *temp = w->first;
	int counter = 0;
	while (temp) {
		if (sendto(w->fd, buf, strlen(buf) + 1, 0, (struct sockaddr *)&(temp->addr), temp->addr_len) == -1)
			diep("sendto");
		temp = temp->next;
	}
}
//Reads data from socket but returns after one second if theres no data
void read_once(World *w, char *buf)
{
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(w->fd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    	diep("setsockopt()");
	}
	recvfrom(w->fd, buf, BUFLEN, 0, NULL, NULL);
}

//Reads players moves and drops them if no answer in 20 seconds
void read_all(World *w)
{
	unsigned int answered = 0;
	unsigned int counter = 0;
	unsigned int id;
	char *buf = calloc(BUFLEN, 1);

	//Initialize after each turn
	Player *temp = w->first;
	while (temp) {
		temp->dir = UNKNOWN;
		temp->answer = 0;
		temp = temp->next;
	}

	time_t start = time(NULL);
	time_t now = time(NULL);

	while (answered < w->players && difftime(now, start) < 20) {
		read_once(w, buf);
		if (strlen(buf)) {
			id = get_id(buf);
			temp = w->first;
			while (temp && temp->id != id)
				temp = temp->next;
			if (temp && !temp->answer) {
				if (parse_moves(w, buf)) {
					answered++;
					temp->answer = 1;
				}
			}
		}
		memset(buf, 0, BUFLEN);
		now = time(NULL);
	}
	free(buf);

	//Goes through player list and if no answer -> drop
	temp = w->first;
	while(temp) {
		if (!temp->answer) {
			Player_drop(w, temp);
			temp = w->first;
		}
		else
			temp = temp->next;
	}
}

//Sends the current field
void send_field(World *w) {
	char *data = malloc(6);
	int how_many = 0;			//how many pieces overall
	Player *temp = w->first;
	unsigned int counter = 0;
	unsigned int i;
	char cx[2];
	char cy[2];
	unsigned int ix;
	unsigned int iy;
	while (temp) {
		data = realloc(data, (how_many + temp->worm_length) * 6 + 1);
		
		counter = 0;			//counter for one worm
		while (counter < temp->worm_length) {
			i = how_many * 6;

			if (temp->worm->co_list[counter].x == temp->worm->head.x && temp->worm->co_list[counter].y == temp->worm->head.y)
				data[i] = 'H';
			else
				data[i] = 'B';

			data[i + 1] = temp->color;

			ix = temp->worm->co_list[counter].x;
			
			sprintf(cx, "%02d", ix);

			data[i + 2] = cx[0];
			data[i + 3] = cx[1];

			iy = temp->worm->co_list[counter].y;
			sprintf(cy, "%02d", iy);

			data[i + 4] = cy[0];
			data[i + 5] = cy[1];

			counter++;
			how_many++;
			
				
		}
		temp = temp->next;
	}
	data = realloc(data, (how_many + w->candy_count) * 6 + 1);
	for (unsigned int j = 0; j < w->candy_count; j++) {
		i = how_many * 6;
		data[i] = 'C';
		data[i + 1] = 'N';
		ix = w->candy_list[j].x;
		sprintf(cx, "%02d", ix);
		data[i + 2] = cx[0];
		data[i + 3] = cx[1];
		iy = w->candy_list[j].y;
		sprintf(cy, "%02d", iy);
		data[i + 4] = cy[0];
		data[i + 5] = cy[1];
		how_many++;
		
	}
	data[how_many * 6] = 0;
	send_all(w, data);
	free(data);
}

int main(void)
{	
	World *my_world = createWorld();
		
	char *buf = calloc(BUFLEN, 1);
	printf("Waiting on port %d \n", PORT);
	int players = 0;
	int fd = create_socket();
	Player *temp;
	my_world->fd = fd;

	time_t start = time(NULL);
	time_t now = time(NULL);
	printf("Waiting for players:\n");
	
	while(difftime(now, start) < 30) {
		temp = Player_init(my_world);
		if (temp) { 
			Player_add(my_world, temp);
			players++;
			sprintf(buf, "Number of players in the game: %d", players);
			send_all(my_world, buf);
		}
		else
			Player_free(temp);
		now = time(NULL);	
	}
	
	my_world->candy_count = my_world->players * 2;
	make_candies(my_world);
	send_all(my_world, "GAME BEGINS");	

	while(1) {
		send_field(my_world);
		read_all(my_world);
		if (!one_turn(my_world))
			break;
	}
	
	send_all(my_world, "quit");
	close(my_world->fd);
	free(buf);
	Player_dequeue(my_world);
	releaseWorld(my_world);
	return 0;
}
