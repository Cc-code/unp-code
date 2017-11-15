#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
	int 				sockfd, opt;
	const char			*s_ip, *s_port;
	struct sockaddr_in	addr;
	char				buf[BUFSIZ];
	ssize_t				nread;

	s_ip = s_port = NULL;

	while ((opt = getopt(argc, argv, "s:p:")) != -1) {
		switch (opt) {
		case 's':
			s_ip = optarg;
			break;
		case 'p':
			s_port = optarg;
			break;
		default:
			fprintf(stderr, "Usage: %s [-s <server ip>] [-p <server port>]\n", argv[0]);
			exit(1);
		}
	}
	if (optind != argc) {
		fprintf(stderr, "Usage: %s [-s <server ip>] [-p <server port>]\n", argv[0]);
		exit(1);
	}

	if (s_ip == NULL)
		s_ip = "127.0.0.1";
	if (s_port == NULL)
		s_port = "2333";

	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket error");
		exit(1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(s_port));
	if (inet_pton(AF_INET, s_ip, &addr.sin_addr) != 1) {
		fprintf(stderr, "inet_pton: %s: prase failure\n", s_ip);
		exit(1);
	}
	if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect error");
		exit(1);
	}

	while (fgets(buf, BUFSIZ, stdin) != NULL) {
		if (sendto(sockfd, buf, strlen(buf), 0, NULL, 0) < 0) {
			perror("sendto error");
			exit(1);
		}
		alarm(3);
		if ((nread = recvfrom(sockfd, buf, BUFSIZ, 0, NULL, NULL)) < 0) {
			perror("recvfrom error");
			exit(1);
		}
		alarm(0);

		buf[nread] = 0;
		fputs(buf, stdout);
	}

	exit(0);
}
