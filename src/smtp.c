#include "smtp.h"

#include <arpa/inet.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

constexpr in_port_t DEFAULT_PORT = 25;

static char const * CRLF = "\r\n";
static char const * CONNECT_OK_CODE = "220";
static char const * COMMAND_OK_CODE = "250";

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

/*
static int quit(fd_t cmd_sock)
{
	if (send(cmd_sock, "quit\n", strlen("quit\n"), 0) < 0)
		return -1;

	ssize_t recieved;
	constexpr ssize_t response_buf_len = 100;
	char response[response_buf_len + 1];
	response[response_buf_len] = '\0';

	recieved = recv(cmd_sock, response, response_buf_len, 0);
	if (recieved < 0)
		return -1;
	response[recieved] = '\0';

	// 221 -- код выхода по quit
	if (strncmp(response, "221 ", 4)) {
		fprintf(stderr, "Ошибка: %s\n", response);
		return 0;
	}
	return 0;
}

int communication_cycle(fd_t cmd_sock)
{
	int rc;
	for (;;) {
		printf("Введите одну из комманд:\n"
		       "l -- список файлов\n"
		       "d -- удалить папку\n"
		       "с -- создать папку\n"
		       "q/C-d -- завершение соединения\n"
		       "-> ");
		int cmd = getchar();

		switch (cmd) {
		case 'q':
		case 'Q':
			while (getchar() != '\n')
				;
			// падение в eof
		case EOF:
			if (quit(cmd_sock) < 0) {
				perror("quit failed");
				return -1;
			}
			return 0;
		case 'c':
		case 'C':
			while (getchar() != '\n')
				;
			rc = create_dir(cmd_sock);
			if (rc < 0) {
				perror("create_dir failed");
				return -1;
			}
			if (rc > 0) { // EOF
				if (quit(cmd_sock) < 0) {
					perror("quit failed");
					return -1;
				}
				return 0;
			}
			break;
		case 'd':
		case 'D':
			while (getchar() != '\n')
				;
			rc = rm_dir(cmd_sock);
			if (rc < 0) {
				perror("rm_dir failed");
				return -1;
			}
			if (rc > 0) { // EOF
				if (quit(cmd_sock) < 0) {
					perror("quit failed");
					return -1;
				}
				return 0;
			}
			break;
		case 'l':
		case 'L':
			while (getchar() != '\n')
				;
			if (list(cmd_sock) < 0) {
				perror("list failed");
				return -1;
			}
			break;
		default:
			while (getchar() != '\n')
				;
			fprintf(stderr, "Неизвестная команда\n");
		}
	}
}

int login(fd_t cmd_sock)
{
	constexpr size_t buflen = 48;
	char buf[buflen + 1];
	buf[buflen] = 0;
	ssize_t recieved;

	// Получение заголовка
	recieved = recv(cmd_sock, buf, buflen, 0);
	if (recieved < 0) {
		perror("recv failed");
		return -1;
	}
	buf[recieved] = '\0';
	printf("Подключено к:%s", strchr(buf, ' '));

	constexpr int user_maxstrlen = 100;
	char user[user_maxstrlen] = "USER ";

	printf("Пользователь: ");
	// Ввод за словом USER
	if (!fgets(user + strlen(user), user_maxstrlen - (int)strlen(user),
		   stdin))
		return -2;

	// Отправка логина
	if (send(cmd_sock, user, strlen(user), 0) < 0) {
		perror("send failed");
		return -1;
	}
	// Получение информации об ожидании пароля
	recieved = recv(cmd_sock, buf, buflen, 0);
	if (recieved < 0) {
		perror("recv failed");
		return -1;
	}
	buf[recieved] = '\0';

	bool not_asking_for_pass = strncmp(buf, "331", 3);
	if (not_asking_for_pass)
		return -1;

	char pass[101] = "PASS ";
	pass[100] = '\0';
	char * tmp_pass = getpass("Пароль: "); // не оставляет \n в stdin

	ssize_t space_in_pass = (ssize_t)sizeof(pass) - (ssize_t)strlen(pass);
	assert(space_in_pass > 0);
	strncat(pass, tmp_pass, (size_t)space_in_pass);
	strcat(pass, "\n");
	free(tmp_pass);

	// Отправление пароля
	if (send(cmd_sock, pass, strlen(pass), 0) < 0) {
		perror("send failed");
		return -1;
	}

	recieved = recv(cmd_sock, buf, buflen, 0);
	if (recieved < 0) {
		perror("recv failed");
		return -1;
	}
	buf[recieved] = '\0';

	// 230 -- successful login
	if (!strncmp("230", buf, 3))
		return 0;

	return -2;
}
*/
