#include "smtp.h"
#include "socks.h"

#include <assert.h>
#include <stdio.h>
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

int main()
{
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

	int login_bad = login(conn_sock);
	if (login_bad == -1) {
		perror("login failed");
		close(conn_sock);
		return 1;
	} else if (login_bad == -2) {
		fprintf(stderr, "Неверный пароль\n");
		close(conn_sock);
		return 1;
	}

	int communication_cycle_bad = communication_cycle(conn_sock);
	if (communication_cycle_bad < 0) {
		perror("cycle failed");
		close(conn_sock);
		return 1;
	}

	// Штатное завершение работы
	puts("Клиент прервал соединение");
	close(conn_sock);
	return 0;
}
