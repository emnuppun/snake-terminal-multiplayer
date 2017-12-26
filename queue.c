#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "game.h"
#include "server.h"
#include "time.h"
#include "queue.h"

/*Creates a player and returns it. Sends id and color back to client*/
Player *Player_init(World *w) 
{
	int recvlen;
	char name_buf[20] = {0};
	char *colors = "RGYBMC";

	struct timeval tv;
	struct sockaddr_in rem_addr;
	int addrlen = sizeof(rem_addr);
	memset(&rem_addr, 0, addrlen);

	Player *p = calloc(1, sizeof(Player));


	tv.tv_sec = 2;
	tv.tv_usec = 0;
	if (setsockopt(w->fd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0)
   		diep("setsockopt()");
	recvlen = recvfrom(w->fd, name_buf, 20, 0, (struct sockaddr *)&rem_addr, &addrlen);
	if (!(recvlen > 0))
		return NULL; 

	strcpy(p->name, name_buf);
	printf("Adding player: %s \n", p->name);
	p->addr = rem_addr;
	p->addr_len = addrlen;
	p->next = NULL; 
	p->id = 33 + w->players;
	p->worm = createWorm(w);
	p->worm_length = 3;
	p->color = colors[rand() % 6];

	char *buf;
	buf = &p->id; 
	if (sendto(w->fd, buf, strlen(buf) + 1, 0, (struct sockaddr *)&(p->addr), p->addr_len) == -1)
		diep("sendto");
	buf = &p->color;
	if (sendto(w->fd, buf, strlen(buf) + 1, 0, (struct sockaddr *)&(p->addr), p->addr_len) == -1)
		diep("sendto");
	
	return p;
}

//Adds player to a World
void Player_add(World *w, Player *p)
{
	if (w->last) 
		w->last->next = p;
	w->last = p;
	if (!(w->first))
		w->first = w->last;
	w->players++;
}

//Drops a player from worlds linked list
int Player_drop(World *w, Player *p)
{
	Player *temp = w->first;
	Player *prev = NULL;
	char *buf2 = "quit";
	while (temp) {
		if (!strcmp(p->name, temp->name)) {
			if (prev)
				prev->next = temp->next;
			if (temp == w->first)
				w->first = temp->next;
			if (temp && !temp->next)
				w->last = prev;
			printf("\nDropping player: %s\n", temp->name);
			if (sendto(w->fd, buf2, strlen(buf2) + 1, 0, (struct sockaddr *)&(p->addr), p->addr_len) == -1)
				diep("sendto");
			
			Player_free(temp);
			w->players--;
			return 1;
		}
		prev = temp;
		temp = temp->next;
	}
	w->players--;
	return 0;
}

//Frees the memory allcoated for player
void Player_free(Player *p)
{
	if (p) {
		free(p->worm->co_list);
		free(p->worm);
		free(p);
	}	
}

//Frees all the players from world
void Player_dequeue(World *w)
{
	Player *temp;

	while(w->first){
		temp = w->first;
		w->first = w->first->next;
		Player_free(temp);	
		if (!w->first)
			w->last = NULL;
	}
}

