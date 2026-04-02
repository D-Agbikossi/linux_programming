/*
 * Digital Library Reservation Platform — TCP server.
 * - Up to 5 concurrent clients (thread per connection).
 * - Mutex protects book inventory and active user list.
 * - Line-based protocol; see comments near handle_client.
 */

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

enum { MAX_CLIENTS = 5, NUM_BOOKS = 6, BUF_SZ = 512, PORT = 9090 };

static const char *const valid_ids[] = {
	"LIB1001", "LIB2002", "LIB3003", "LIB4004", "LIB5005"
};
static const char *const book_titles[NUM_BOOKS] = {
	"Systems Programming",
	"Computer Networks",
	"Operating Systems",
	"Algorithms",
	"Databases",
	"Embedded Linux"
};

static int book_owner[NUM_BOOKS];

static char active_users[MAX_CLIENTS][32];
static int active_count;

static pthread_mutex_t srv_mtx = PTHREAD_MUTEX_INITIALIZER;

static bool
is_valid_id(const char *id)
{
	size_t i;

	for (i = 0; i < sizeof(valid_ids) / sizeof(valid_ids[0]); i++) {
		if (strcmp(id, valid_ids[i]) == 0)
			return true;
	}
	return false;
}

static void
print_server_status_locked(void)
{
	int i;

	printf("[Server] Active users (%d):", active_count);
	for (i = 0; i < active_count; i++)
		printf(" %s", active_users[i]);
	printf("\n[Server] Books:");
	for (i = 0; i < NUM_BOOKS; i++) {
		printf(" [%d]%s:", i, book_titles[i]);
		if (book_owner[i] < 0)
			printf("available");
		else
			printf("reserved");
	}
	printf("\n");
	fflush(stdout);
}

static ssize_t
send_line(int fd, const char *s)
{
	char buf[BUF_SZ];
	size_t n = strlen(s);

	if (n + 1 >= sizeof(buf))
		return -1;
	memcpy(buf, s, n);
	buf[n] = '\n';
	buf[n + 1] = '\0';
	return write(fd, buf, n + 1);
}

static ssize_t
read_line_sock(int fd, char *buf, size_t cap)
{
	size_t i = 0;
	ssize_t r;

	while (i + 1 < cap) {
		char c;

		r = read(fd, &c, 1);
		if (r < 0)
			return r;
		if (r == 0) {
			buf[i] = '\0';
			return 0;
		}
		if (c == '\n')
			break;
		if (c != '\r')
			buf[i++] = c;
	}
	buf[i] = '\0';
	return (ssize_t)i;
}

static void *
handle_client(void *arg)
{
	int cfd = *(int *)arg;
	free(arg);
	char buf[BUF_SZ];
	char user[32] = "";
	bool authed = false;
	ssize_t n;

	send_line(cfd, "HELLO Send AUTH:<library_id>");

	for (;;) {
		n = read_line_sock(cfd, buf, sizeof(buf));
		if (n < 0) {
			perror("read");
			break;
		}
		if (n == 0 && buf[0] == '\0')
			break;

		if (strncmp(buf, "AUTH:", 5) == 0) {
			char id[32];
			size_t ilen = strcspn(buf + 5, "\r\n");

			if (ilen >= sizeof(id))
				ilen = sizeof(id) - 1;
			memcpy(id, buf + 5, ilen);
			id[ilen] = '\0';

			pthread_mutex_lock(&srv_mtx);
			if (!is_valid_id(id)) {
				pthread_mutex_unlock(&srv_mtx);
				send_line(cfd, "AUTH_FAIL");
				continue;
			}
			if (active_count >= MAX_CLIENTS) {
				pthread_mutex_unlock(&srv_mtx);
				send_line(cfd, "AUTH_FAIL");
				continue;
			}
			{
				size_t ulen = strlen(id);

				if (ulen >= sizeof(user))
					ulen = sizeof(user) - 1;
				memcpy(user, id, ulen);
				user[ulen] = '\0';
			}
			if (active_count < MAX_CLIENTS) {
				snprintf(active_users[active_count],
				    sizeof(active_users[0]), "%s", user);
				active_count++;
			}
			authed = true;
			print_server_status_locked();
			pthread_mutex_unlock(&srv_mtx);
			send_line(cfd, "AUTH_OK");
			continue;
		}

		if (strcmp(buf, "LIST") == 0) {
			if (!authed) {
				send_line(cfd, "ERR:NOT_AUTH");
				continue;
			}
			{
				char out[BUF_SZ];
				size_t pos = 0;
				int i;

				pos += (size_t)snprintf(out + pos, sizeof(out) - pos,
				    "LIST:");
				for (i = 0; i < NUM_BOOKS && pos < sizeof(out) - 2;
				    i++) {
					pos += (size_t)snprintf(out + pos,
					    sizeof(out) - pos, "%s%s",
					    book_titles[i],
					    (i + 1 < NUM_BOOKS) ? "|" : "");
				}
				send_line(cfd, out);
			}
			continue;
		}

		if (strncmp(buf, "RESERVE:", 8) == 0) {
			int bi = atoi(buf + 8);

			if (!authed) {
				send_line(cfd, "ERR:NOT_AUTH");
				continue;
			}
			pthread_mutex_lock(&srv_mtx);
			if (bi < 0 || bi >= NUM_BOOKS) {
				pthread_mutex_unlock(&srv_mtx);
				send_line(cfd, "ERR:BAD_BOOK");
				continue;
			}
			if (book_owner[bi] >= 0) {
				pthread_mutex_unlock(&srv_mtx);
				send_line(cfd, "RESERVE_FAIL");
				continue;
			}
			book_owner[bi] = 1;
			print_server_status_locked();
			pthread_mutex_unlock(&srv_mtx);
			send_line(cfd, "RESERVE_OK");
			continue;
		}

		if (strcmp(buf, "QUIT") == 0) {
			send_line(cfd, "BYE");
			break;
		}
		send_line(cfd, "ERR:UNKNOWN");
	}

	pthread_mutex_lock(&srv_mtx);
	if (authed && user[0] != '\0') {
		int i, j;

		for (i = 0; i < active_count; i++) {
			if (strcmp(active_users[i], user) == 0) {
				for (j = i; j + 1 < active_count; j++)
					memcpy(active_users[j], active_users[j + 1],
					    sizeof(active_users[0]));
				active_count--;
				break;
			}
		}
	}
	print_server_status_locked();
	pthread_mutex_unlock(&srv_mtx);

	close(cfd);
	return NULL;
}

int
main(void)
{
	int listen_fd;
	int opt = 1;
	struct sockaddr_in addr;
	int slot = 0;

	memset(book_owner, 0xff, sizeof(book_owner));
	memset(&addr, 0, sizeof(addr));

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		perror("socket");
		return 1;
	}
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PORT);

	if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return 1;
	}
	if (listen(listen_fd, 16) < 0) {
		perror("listen");
		return 1;
	}
	printf("Library server listening on port %d (max %d clients)\n", PORT,
	    MAX_CLIENTS);
	fflush(stdout);

	for (;;) {
		struct sockaddr_in cli;
		socklen_t clen = sizeof(cli);
		int *cfdp;
		pthread_t th;

		int cfd = accept(listen_fd, (struct sockaddr *)&cli, &clen);
		if (cfd < 0) {
			perror("accept");
			continue;
		}
		pthread_mutex_lock(&srv_mtx);
		slot++;
		pthread_mutex_unlock(&srv_mtx);
		(void)slot;
		cfdp = malloc(sizeof(int));
		if (cfdp == NULL) {
			close(cfd);
			continue;
		}
		*cfdp = cfd;
		if (pthread_create(&th, NULL, handle_client, cfdp) != 0) {
			free(cfdp);
			close(cfd);
			perror("pthread_create");
			continue;
		}
		pthread_detach(th);
	}
}
