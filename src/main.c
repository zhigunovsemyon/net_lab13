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

[[maybe_unused]] constexpr uint32_t LOCALHOST = (127 << 24) + 1;

int main()
{
	// Структура с адресом и портом сервера
	struct sockaddr_in server_addr;
	if (set_destination(&server_addr)) {
		fputs("Неправильно указан адрес!\n", stderr);
		return 1;
	}
	// Входящий сокет
	fd_t cmd_sock = create_connected_socket(&server_addr);
	if (-1 == cmd_sock) {
		perror("Failed to create command socket");
		return -1;
	}

	int login_bad = login(cmd_sock);
	if (login_bad == -1) {
		perror("login failed");
		close(cmd_sock);
		return 1;
	} else if (login_bad == -2) {
		fprintf(stderr, "Неверный пароль\n");
		close(cmd_sock);
		return 1;
	}

	int communication_cycle_bad = communication_cycle(cmd_sock);
	if (communication_cycle_bad < 0) {
		perror("cycle failed");
		close(cmd_sock);
		return 1;
	}

	// Штатное завершение работы
	puts("Клиент прервал соединение");
	close(cmd_sock);
	return 0;
}
