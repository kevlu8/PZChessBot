#include <netinet/in.h>
// #include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

__attribute__((aligned(4096))) char buf[1 << 24];
// pthread_mutex_t m;
char *filename = "data.txt";

void handle(int c) {
	while (1) {
		// int c = (int)a;
		if (read(c, buf, 4) == 0)
			return;

		int l = *(uint32_t *)buf;
		int tot_read = 0;
		while (tot_read < l) {
			int bytes_read = read(c, buf + tot_read, l - tot_read);
			if (bytes_read == 0)
				return;
			else if (bytes_read < 0) {
				perror(NULL);
				return;
			}
			tot_read += bytes_read;
		}

		// pthread_mutex_lock(&m);
		FILE *f = fopen(filename, "a");
		if (f == NULL) {
			perror(NULL);
			break;
		}
		fwrite(buf, 1, l, f);
		fclose(f);
		*(uint32_t *)buf = tot_read;
		send(c, buf, 4, 0);
		// pthread_mutex_unlock(&m);
	}
	shutdown(c, SHUT_RDWR);
	while (read(c, buf, sizeof(buf)) > 0);
	close(c);
	return;
}

int main(int argc, char **argv) {
	if (argc > 1)
		filename = argv[1];

	// pthread_mutex_init(&m, NULL);

	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		fprintf(stderr, "Could not create socket\n");
		return 1;
	}
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(44500),
		.sin_addr = 0,
	};
	if (bind(s, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
		fprintf(stderr, "Could not bind address\n");
		return 1;
	}
	if (listen(s, 4) < 0) {
		fprintf(stderr, "Failed to listen\n");
		return 1;
	}
	while (1) {
		int c = accept(s, NULL, NULL);
		// pthread_t t;
		// pthread_create(&t, NULL, handle, (void *)c);
		handle(c);
	}
	return 0;
}
