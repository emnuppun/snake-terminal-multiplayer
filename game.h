#ifndef GAME_H
#define GAME_H


typedef enum{
	EMPTY,
	WORM,
	CANDY
} State;

typedef enum {
	NORTH,
	EAST,
	SOUTH,
	WEST,
	UNKNOWN
} Direction;

typedef struct {
	unsigned int x;
	unsigned int y;
} Coordinates;

typedef struct {
	Coordinates *co_list;
	Coordinates head;
	Direction direction;
} Worm;


typedef struct player_st{
	char name[20];
	char id;  //33 - 125
	unsigned int answer;
	Direction dir;
	char color;
	struct sockaddr_in addr;
	int addr_len;
	struct player_st *next;
	int worm_length;
	Worm *worm;

} Player;


typedef struct world_st{
	unsigned int candy_count;
	Coordinates *candy_list;
	unsigned int fd;
	State **cells;
	Player *first;
	Player *last;
	unsigned int players;
} World;


World *createWorld(void);

void releaseWorld(World *W);

Worm *createWorm(World *W);

int get_id(char *info);

void new_candy(World *w, unsigned int i);

void move_worm(World *w, Player *temp);

int legal_move(Coordinates c);

int one_turn(World *w);

void make_candies(World *w);

int parse_moves(World *w, char* info);

#endif
