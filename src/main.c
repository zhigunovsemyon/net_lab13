#include "smtp.h"
#include "socks.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/*
	Для тестирования создаваемого почтового приложения необходимо
использовать собственный почтовый сервер, настроенный студентом в
процессе выполнения предыдущих лабораторных работ. Это связано с тем, что
почтовые сервера в интернете работают по протоколу ESMTP, требующего
авторизацию. При этом для выполнения лабораторной работы достаточно
реализации только базовых команд, входящих в SMTP.
	Задание по варианту: Рассылка спама. Программа отсылает некоторое
рекламное сообщение пользователю, при этом в конец сообщения добавляется
несколько строк случайного текста.
 */

char const * data[] = {
	"Date: Tue, 02 Dec 2025 12:04:34", //
	"From: support@apple.com",	   //
	"Subject: Розыгрыш iPhone 3GS",	   //
	"To: user1@localdomain",	   //
	"Для получения отправьте ваш номер банковской карты, её срок действия ",
	"и CVC код на следующий почтовый адрес:", //
	"notascam@disposabledomain.io",		  //
	"Случайные символы:"			  //
};

int main()
{
	srand((unsigned int)time(NULL));

	// Структура с адресом и портом сервера
	struct sockaddr_in server_addr;

	if (set_destination(&server_addr)) {
		fputs("Неправильно указан адрес!\n", stderr);
		return 1;
	}

	// Входящий сокет
	fd_t conn_sock = create_connected_socket(&server_addr);
	if (-1 == conn_sock) {
		perror("Failed to create command socket");
		return -1;
	}

	CommErr comm_err = check_connection_response(conn_sock);
	if (COMMERR_OK != comm_err) {
		goto end;
	}

	comm_err = send_sender_and_recipient(conn_sock);
	if (COMMERR_OK != comm_err) {
		goto end;
	}

	comm_err = send_data(conn_sock, data, sizeof(data) / sizeof(*data));
	if (COMMERR_OK != comm_err) {
		goto end;
	}

end:
	switch (comm_err) {
	case COMMERR_OK:
		puts("Завершение работы");
		close(conn_sock);
		return 0;
	case COMMERR_BAD_SERVER_HELLO:
		fprintf(stderr, "Ошибка при подключении к серверу!");
		break;
	case COMMERR_BAD_MAIL_SOURCE:
		fprintf(stderr, "Ошибка при указании отправителя!");
		break;
	case COMMERR_BAD_MAIL_DEST:
		fprintf(stderr, "Ошибка при указании получателя!");
		break;
	case COMMERR_SEND:
	case COMMERR_RECV:
		perror("");
		break;
	case COMMERR_DATA_TRANSFER_NOT_CONFIRMED:
		fprintf(stderr, "Почтовый сервер отказался получать данные!");
		break;
	case COMMERR_BAD_DATA:
		fprintf(stderr, "Переданы некорректные данные!");
		break;
	case COMMERR_DATA_NOT_ACCEPTED:
		fprintf(stderr, "Почтовый сервер отказался передавать данные!");
		break;
	default:
		fputs("Неизвестная ошибка!", stderr);
		break;
	}
	close(conn_sock);
	return -1;
}
