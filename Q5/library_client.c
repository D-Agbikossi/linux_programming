/*
 * Library reservation client — TCP to server, line-based protocol.
 */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

enum { BUF_SZ = 512, PORT = 9090 };

static ssize_t
read_line(int fd, char *buf, size_t cap)
{
	size_t i = 0;
	ssize_t r;

	while (i + 1 < cap) {
		char c;

		r = read(fd, &c, 1);
		if (r <= 0)
			return r;
		if (c == '\n')
			break;
		if (c != '\r')
			buf[i++] = c;
	}
	buf[i] = '\0';
	return (ssize_t)i;
}

int
main(int argc, char **argv)
{
	const char *host = "127.0.0.1";
	char id[64];
	char buf[BUF_SZ];
	struct sockaddr_in addr;
	int fd;
	ssize_t n;

	if (argc > 1)
		host = argv[1];

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket");
		return 1;
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
		fprintf(stderr, "bad address\n");
		return 1;
	}
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		return 1;
	}

	n = read_line(fd, buf, sizeof(buf));
	if (n < 0) {
		perror("read");
		close(fd);
		return 1;
	}
	printf("Server: %s\n", buf);

	printf("Enter library ID: ");
	fflush(stdout);
	if (scanf("%63s", id) != 1) {
		close(fd);
		return 1;
	}

	snprintf(buf, sizeof(buf), "AUTH:%s", id);
	if (write(fd, buf, strlen(buf)) < 0 || write(fd, "\n", 1) < 0) {
		perror("write");
		close(fd);
		return 1;
	}
	n = read_line(fd, buf, sizeof(buf));
	if (n < 0) {
		perror("read");
		close(fd);
		return 1;
	}
	printf("Auth result: %s\n", buf);
	if (strncmp(buf, "AUTH_OK", 7) != 0) {
		printf("Session closed. Goodbye, %s\n", id);
		close(fd);
		return 0;
	}

	if (write(fd, "LIST\n", 5) < 0) {
		perror("write");
		close(fd);
		return 1;
	}
	n = read_line(fd, buf, sizeof(buf));
	if (n < 0) {
		perror("read");
		close(fd);
		return 1;
	}
	printf("Available books: %s\n", buf);

	printf("Enter book index to reserve (0-%d): ", 5);
	fflush(stdout);
	int bi = -1;
	if (scanf("%d", &bi) != 1) {
		close(fd);
		return 1;
	}
	snprintf(buf, sizeof(buf), "RESERVE:%d", bi);
	if (write(fd, buf, strlen(buf)) < 0 || write(fd, "\n", 1) < 0) {
		perror("write");
		close(fd);
		return 1;
	}
	n = read_line(fd, buf, sizeof(buf));
	if (n < 0) {
		perror("read");
		close(fd);
		return 1;
	}
	printf("Reservation: %s\n", buf);

	(void)write(fd, "QUIT\n", 5);
	(void)read_line(fd, buf, sizeof(buf));
	close(fd);
	printf("Session closed. Goodbye, %s\n", id);
	return 0;
}
