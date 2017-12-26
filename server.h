#ifndef SERVER_H
#define SERVER_H



void diep(char *s);

int create_socket(void);

void read_all(World *w);

void send_field(World *w);

#endif
