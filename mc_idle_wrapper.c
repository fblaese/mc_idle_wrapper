#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

static void die(const char *msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}

static int packet_len;

static char *read_packet(FILE *r) {
	int c = fgetc(r);
	if (c == EOF || (c & 0b10000000) != 0) {
		fprintf(stderr, "Received unprocessable packet\n");
		return NULL;
	}

	char * buffer = malloc(c + 1);
	if (buffer == NULL) {
		perror("malloc");
		return NULL;
	}

	if (NULL == fgets(buffer, c + 1, r)) {
		if (ferror(r)) {
			perror("fgets");
		}
		free(buffer);
		return NULL;
	}

	packet_len = c;
	return buffer;
}

static void communicate(FILE *r, FILE *w) {
	char *buffer;

	if ((buffer = read_packet(r)) == NULL) {
		return;
	}
	if (*buffer != 0x0) {
		fprintf(stderr, "Received packet %x instead of handshake packet\n", *buffer);
		free(buffer);
		return;
	}
	free(buffer);


	if ((buffer = read_packet(r)) == NULL) {
		return;
	}
	if (*buffer != 0x0) {
		fprintf(stderr, "Received packet %x instead of login packet\n", *buffer);
		free(buffer);
		return;
	}
	if (packet_len > 2) {
		buffer += 2;
		char username[strlen(buffer)];
		strcpy(username, buffer);
		buffer -= 2;
		fprintf(stdout, "User connected: %s\n", username);
		fflush(stdout);

		if (strcmp(username, "user1") == 0 || strcmp(username, "user2") == 0) {
			fputc(0x1, w);
			fputc(0x0, w);
			exit(EXIT_SUCCESS);
		}
	}
	free(buffer);

	fputc(0x1, w);
	fputc(0x0, w);
	fflush(w);
}

int main(void) {
	int sock = socket(AF_INET6, SOCK_STREAM, 0);
	if (sock == -1) {
		die("socket");
	}

	int flag = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

	struct sockaddr_in6 bind_addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(25569),
		.sin6_addr = in6addr_any,
	};

	int ret = bind(sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr));
	if (ret == -1) {
		die("bind");
	}

	ret = listen(sock, SOMAXCONN);
	if (ret == -1) {
		die("listen");
	}

	for (;;) {
		int rsock = accept(sock, NULL, NULL);
		if (rsock == -1) {
			perror("accept");
			continue;
		}

		int wsock = dup(rsock);
		if (wsock == -1) {
			perror("dup");
			continue;
		}

		FILE *read = fdopen(rsock, "r");
		FILE *write = fdopen(wsock, "a");
		if (read == NULL || write == NULL) {
			perror("fdopen");
			continue;
		}

		communicate(read, write);

		fclose(read);
		fclose(write);
	}
}
