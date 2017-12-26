#ifndef QUEUE_H
#define QUEUE_H
#include "game.h"


Player *Player_init(World *w);

void Player_add(World *w, Player *p);

int Player_drop(World *q, Player *p);

void Player_free(Player *p);

void Player_dequeue(World *w);

#endif
