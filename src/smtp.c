#include "smtp.h"

#include <arpa/inet.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

constexpr in_port_t DEFAULT_PORT = 25;

static char const * CRLF = "\r\n";
static char const * DATA_END = "\r\n.\r\n";

static char const * CONNECT_OK_CODE = "220";
static char const * COMMAND_OK_CODE = "250";
static char const * DATA_TRANSFER_CONFIRMED = "354";

[[maybe_unused]]
static size_t make_random_line(char * buf, size_t max_line_len)
{
	for (size_t i = 0; i < max_line_len; ++i) {
		buf[i] = (char)(rand() % (127 - 33));
		if (buf[i] == 0) {
			return i;
		}
		buf[i] += 32;
	}

	return max_line_len;
}

static CommErr send_random_data(fd_t fd)
{
	constexpr size_t max_line_len = 100;
	constexpr int max_line_count = 10;

	char buf[max_line_len + 1];
	buf[max_line_len] = '\0';

	int lines = rand() % max_line_count;
	for (int i = 0; i < lines; ++i) {
		make_random_line(buf, max_line_len);
		if (send(fd, buf, strlen(buf), 0) < 0) {
			return COMMERR_SEND;
		}
		if (send(fd, CRLF, strlen(CRLF), 0) < 0) {
			return COMMERR_SEND;
		}
	}
	return COMMERR_OK;
}

static CommErr check_data_send_responce(fd_t fd)
{
	constexpr size_t buflen = 99;
	char buf[buflen];

	ssize_t recieved = recv(fd, buf, buflen, 0);
	if (recieved < 0) {
		return COMMERR_RECV;
	}
	buf[recieved] = '\0';

	puts(buf);

	if (strncmp(buf, COMMAND_OK_CODE, strlen(COMMAND_OK_CODE))) {
		return COMMERR_DATA_NOT_ACCEPTED;
	}
	return COMMERR_OK;
}

static CommErr confirm_send_data(fd_t fd)
{
	constexpr size_t buflen = 99;
	char buf[buflen];

	if (send(fd, "DATA\r\n", strlen("DATA\r\n"), 0) < 0) {
		return COMMERR_SEND;
	}

	ssize_t recieved = recv(fd, buf, buflen, 0);
	if (recieved < 0) {
		return COMMERR_RECV;
	}
	buf[recieved] = '\0';

	puts(buf);

	if (strncmp(buf, DATA_TRANSFER_CONFIRMED,
		    strlen(DATA_TRANSFER_CONFIRMED))) {
		return COMMERR_DATA_TRANSFER_NOT_CONFIRMED;
	}

	return COMMERR_OK;
}

CommErr send_data(fd_t fd, char const * data[], size_t lines)
{
	if (data == nullptr) {
		return COMMERR_BAD_DATA;
	}
	CommErr res = confirm_send_data(fd);
	if (res != COMMERR_OK) {
		return res;
	}

	for (size_t i = 0; i < lines; ++i) {
		if (data[i] == nullptr) {
			return COMMERR_BAD_DATA;
		}
		if (send(fd, data[i], strlen(data[i]), 0) < 0) {
			return COMMERR_SEND;
		}
		if (send(fd, CRLF, strlen(CRLF), 0) < 0) {
			return COMMERR_SEND;
		}
	}
	res = send_random_data(fd);
	if (res != COMMERR_OK) {
		return res;
	}

	if (send(fd, DATA_END, strlen(DATA_END), 0) < 0) {
		return COMMERR_SEND;
	}

	return check_data_send_responce(fd);
}

CommErr check_connection_response(fd_t fd)
{
	char buf[99];

	ssize_t recieved = recv(fd, buf, sizeof(buf), 0);
	if (recieved < 0) {
		return COMMERR_RECV;
	}

	buf[recieved] = '\0';
	puts(buf);

	if (strncmp(buf, CONNECT_OK_CODE, strlen(CONNECT_OK_CODE))) {
		return COMMERR_BAD_SERVER_HELLO;
	}

	return COMMERR_OK;
}

int set_destination(struct sockaddr_in * addr_to_set)
{
	constexpr size_t dest_str_size = 24;
	char dest_str[dest_str_size + 1];
	printf("IPv4-адрес[:порт]: ");
	if (!fgets(dest_str, dest_str_size, stdin))
		return -1;

	memset(addr_to_set, 0, sizeof(*addr_to_set));
	addr_to_set->sin_family = AF_INET;

	char * port_str = strchr(dest_str, ':');
	// Если порт не указан
	if (!port_str) {
		// Установка стандартного порта
		addr_to_set->sin_port = htons(DEFAULT_PORT);
	} else {
		// Непосредственно установка порта
		int new_port = atoi(1 + port_str);
		addr_to_set->sin_port = (new_port < 1 || new_port > UINT16_MAX)
						? htons(DEFAULT_PORT)
						: htons((in_port_t)new_port);

		// При установке IP :порт считываться не будет
		*port_str = '\0';
	}

	// Установка IP адреса
	addr_to_set->sin_addr.s_addr = inet_addr(dest_str);
	if (addr_to_set->sin_addr.s_addr == (in_addr_t)-1)
		return -1;

	return 0;
}

static CommErr send_recicpient(fd_t fd)
{
	constexpr size_t buflen = 99;
	char buf[buflen + 1];

	fputs("Почтовый адрес получателя: ", stdout);
	if (scanf("%95s", buf) < 0) {
		return COMMERR_EOF;
	}
	strcat(buf, CRLF);

	if (send(fd, "RCPT TO: ", 9, 0) < 0) {
		return COMMERR_SEND;
	}
	if (send(fd, buf, strlen(buf), 0) < 0) {
		return COMMERR_SEND;
	}

	ssize_t recieved = recv(fd, buf, buflen, 0);
	if (recieved < 0) {
		return COMMERR_RECV;
	}
	buf[recieved] = '\0';

	if (!strncmp(buf, COMMAND_OK_CODE, strlen(COMMAND_OK_CODE))) {
		return COMMERR_OK;
	}
	return COMMERR_BAD_MAIL_DEST;
}

static CommErr send_sender(fd_t fd)
{
	constexpr size_t buflen = 99;
	char buf[buflen + 1];

	fputs("Почтовый адрес отправителя: ", stdout);
	if (scanf("%95s", buf) < 0) {
		return COMMERR_EOF;
	}
	strcat(buf, CRLF);

	if (send(fd, "MAIL FROM: ", 11, 0) < 0) {
		return COMMERR_SEND;
	}
	if (send(fd, buf, strlen(buf), 0) < 0) {
		return COMMERR_SEND;
	}

	ssize_t recieved = recv(fd, buf, buflen, 0);
	if (recieved < 0) {
		return COMMERR_RECV;
	}
	buf[recieved] = '\0';

	if (strncmp(buf, COMMAND_OK_CODE, strlen(COMMAND_OK_CODE))) {
		return COMMERR_BAD_MAIL_SOURCE;
	}

	return COMMERR_OK;
}

CommErr send_sender_and_recipient(fd_t fd)
{
	CommErr res = send_sender(fd);
	return (res != COMMERR_OK) ? res : send_recicpient(fd);
}
