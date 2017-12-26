#include <stdlib.h> 
#include <string.h> 
#include <stdio.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <stdio.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <unistd.h> 
 
#include "game.h" 
#include "server.h" 
#include "queue.h" 
 
#define HEIGHT 20 
#define WIDTH 60
 
//Creates and returns a pointer to a World struct
World *createWorld(void) 
{ 
	State **arr; 
		World *W = calloc(1,sizeof(World)); 
		arr = malloc(HEIGHT * sizeof(State*)); 
		for (unsigned int j = 0; j < HEIGHT; j++) { 
			arr[j] = malloc(WIDTH *sizeof(State)); 
			for(unsigned int i = 0; i < WIDTH; i++) { 
				arr[j][i] = EMPTY; 
			} 
		} 
	W->cells = arr; 
	return W; 
} 

//Releases the memory allocated for World
void releaseWorld(World *W) 
{
	free(W->candy_list);
  	for (unsigned int j = 0; j < HEIGHT; j++){ 
    	free(W->cells[j]); 
  	} 
  	free(W->cells); 
  	free(W); 
}

//Creates and returns a pointer to a Worm struct
Worm *createWorm(World *W) 
{ 
	Worm *worm = malloc(sizeof(Worm)); 
	unsigned int y, x; 
	Coordinates *co_list = calloc(4,sizeof(Coordinates)); 
	worm->co_list = co_list; 
	int place = 1; 
	int i = 0; 
	while (place) { 
		y = (rand() % (HEIGHT - 5)) + 3; 
		x = rand() % (WIDTH - 10); 
		if (W->cells[y][x] == EMPTY && W->cells[y][x+1] == EMPTY && W->cells[y][x+2] == EMPTY) { 
			for ( ; i < 3; i++) { 
			    worm->co_list[i].x = x; 
			    worm->co_list[i].y = y; 
				W->cells[y][x] = WORM;
			    x++; 
			    place = 0;
			} //for 
		} //if
	} //while

	worm->head = worm->co_list[0];
	return worm; 
} 

//Returns a player id from string
int get_id(char *info) 
{
	int id = info[0];
	return id;
}

//Creates a new candy to the world after one has been eaten 
void new_candy(World *w, unsigned int i)
{
	unsigned int done = 0;
	unsigned int x = w->candy_list[i].x;
	unsigned int y = w->candy_list[i].y;
	while (!done) {
		x = rand() % (WIDTH - 2) + 1;
		y = rand() % (HEIGHT - 2) + 1;
		if (w->cells[y][x] == EMPTY) {
			w->cells[y][x] = CANDY;
			w->candy_list[i].x = x;
			w->candy_list[i].y = y;
			done = 1;
		} //if
	} //while
}

//Moves the Players worm by its given direction and updates the cell field
void move_worm(World *w, Player *temp) 
{
	unsigned int x, y;
	for(unsigned int i = 0; i < temp->worm_length; i++) {
		x = temp->worm->co_list[i].x;
		y = temp->worm->co_list[i].y;
		w->cells[y][x] = EMPTY;
	}
	Coordinates *list = malloc(temp->worm_length*sizeof(Coordinates));
	int j = 0;
	for(int i = 1; i < temp->worm_length; i++) {
		list[i].x = temp->worm->co_list[j].x;
		list[i].y = temp->worm->co_list[j].y;
		j++;
	}
	list[0].x = temp->worm->head.x;
	list[0].y = temp->worm->head.y;
	free(temp->worm->co_list);
	temp->worm->co_list = list;
	for(unsigned int i = 0; i < temp->worm_length; i++) {
		x = temp->worm->co_list[i].x;
		y = temp->worm->co_list[i].y;
		w->cells[y][x] = WORM;
	}
}

//Checks if the players move is legal 
int legal_move(Coordinates c) {
	if (c.x == 0 || c.y == 0)
		return 0;
	else if (c.x == WIDTH - 1 || c.y == HEIGHT - 1)
		return 0;
	else
		return 1;
}

/*Goes through all the worms, changes their 'head' coordinates, checks if the move is legal */
int one_turn(World *w)
{
	unsigned int drop = 0;
	unsigned int i = 0;
	unsigned int j = 0;
	Player *temp = w->first;
	Player *prev = w->first;

	while (temp != NULL) {

		switch(temp->dir) {

			case NORTH:
				temp->worm->head.y--;
				if (!legal_move(temp->worm->head)) {
					drop++;
				}
				break;

			case EAST:
				temp->worm->head.x++;
				if (!legal_move(temp->worm->head)) {
					drop++;
				}
				break;

			case SOUTH:
				temp->worm->head.y++;
				if (!legal_move(temp->worm->head)) {
					drop++;
				}
				break;

			case WEST:
				temp->worm->head.x--;
				if (!legal_move(temp->worm->head)) {
					drop++;
				}
				break;

			default:
				return 0;

		} //switch

		while (i < w->candy_count){
			if (temp->worm->head.x == w->candy_list[i].x && temp->worm->head.y == w->candy_list[i].y) {
				new_candy(w, i);
				temp->worm_length++;
				temp->worm->co_list = realloc(temp->worm->co_list, temp->worm_length*sizeof(Coordinates));
				j = temp->worm_length - 1;
				temp->worm->co_list[j] = temp->worm->co_list[j - 1];
			}
			i++;
		}

		i = 0;
		prev = temp;
		temp = temp->next;

		if (drop)
			Player_drop(w, prev);
		else
			move_worm(w, prev);

	} // while (temp)

	if (!w->players)
		return 0;

	return 1;

}

//Creates candies when the game begins
void make_candies(World *w)
{
	unsigned int count = 0;
	unsigned int x, y;
	w->candy_list = malloc(w->candy_count * sizeof(Coordinates));
	
	while (count < w->candy_count) {
		x = rand() % (WIDTH - 2) + 1;
		y = rand() % (HEIGHT - 2) + 1;
		if (w->cells[y][x] == EMPTY) {
			w->cells[y][x] = CANDY;
			w->candy_list[count].x = x;
			w->candy_list[count].y = y;
			count++;
		}
	}
}


//Sets players direction according to data 'info'
int parse_moves(World *w, char *info) 
{
	int id = get_id(info);
	char move = info[1];
	Direction dir;

	switch (move) {

		case 'w':
		dir = NORTH;
		break;

		case 's':
		dir = SOUTH;
		break;

		case 'd':
		dir = EAST;
		break;

		case 'a':
		dir = WEST;
		break;

		default:
		dir = UNKNOWN;
	}

	Player *temp = w->first;
	
	while (temp) {
		if (temp->id == id) {
			temp->dir = dir;
			return 1;
		}
		temp = temp->next;
	}

	return 0;
}
