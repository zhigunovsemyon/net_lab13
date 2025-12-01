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
	COMMERR_DATA_TRANSFER_NOT_CONFIRMED,
	COMMERR_BAD_DATA,
	COMMERR_DATA_NOT_ACCEPTED,
} CommErr;

int set_destination(struct sockaddr_in * addr_to_set);

CommErr check_connection_response(fd_t fd);

CommErr send_sender_and_recipient(fd_t);

CommErr send_data(fd_t fd, char const * data[], size_t lines);

#endif // !SMTP_H_
