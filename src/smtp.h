#ifndef SMTP_H_
#define SMTP_H_
#include "socks.h"

int set_destination(struct sockaddr_in * addr_to_set);

int communication_cycle(fd_t fd);

int login(fd_t fd);

bool check_connection_response(fd_t fd);

#endif // !SMTP_H_
