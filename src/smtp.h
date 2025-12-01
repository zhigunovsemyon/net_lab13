#ifndef SMTP_H_
#define SMTP_H_
#include "socks.h"

typedef enum {
	COMMERR_OK,
	COMMERR_EOF,
	COMMERR_SEND,
	COMMERR_RECV,
	COMMERR_BAD_SERVER_HELLO,
	COMMERR_BAD_MAIL_SOURCE,
	COMMERR_BAD_MAIL_DEST,
} CommErr;

int set_destination(struct sockaddr_in * addr_to_set);

CommErr check_connection_response(fd_t fd);

CommErr send_sender_and_recipient(fd_t);

#endif // !SMTP_H_
