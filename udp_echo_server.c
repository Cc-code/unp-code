#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef OPEN_MAX
#define OPEN_MAX 1024
#endif

int main(int argc, char *argv[])
{
	const int			on = 1;
	int					opt, sockfd, maxi = 0, allfd[OPEN_MAX], n = 0, nready, i;
	struct ifaddrs		*ifap;
	struct sockaddr_in	servaddr, cliaddr;
	socklen_t			clilen;
	char				buf[BUFSIZ], *s_port;
	ssize_t				nread;
	fd_set				rdset, tmpset;

	s_port = NULL;

	while ((opt = getopt(argc, argv, "p:")) != -1) {
		switch (opt) {
		case 'p':
			s_port = optarg;
			break;
		default:
			fprintf(stderr, "Usage: %s [<local port>]\n", argv[0]);
			exit(1);
		}
	}
	if (optind != argc) {
		fprintf(stderr, "Usage: %s [<local port>]\n", argv[0]);
		exit(1);
	}

	if (s_port == NULL)
		s_port = "2333";

	if (getifaddrs(&ifap) < 0) {
		perror("getifaddrs error");
		exit(1);
	}

	memset(&rdset, 0, sizeof(rdset));
	for ( ; ifap != NULL; ifap = ifap->ifa_next) {
		if (ifap->ifa_addr == NULL || ifap->ifa_addr->sa_family != AF_INET)
			continue;

		if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
			perror("socket error");
			exit(1);
		}
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
			perror("setsockopt error");
			exit(1);
		}

		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(atoi(s_port));
		servaddr.sin_addr = ((struct sockaddr_in *)ifap->ifa_addr)->sin_addr;

		if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
			perror("bind error");
			exit(1);
		}

		if (sockfd > maxi)
			maxi = sockfd;
		FD_SET(sockfd, &rdset);
		allfd[n++] = sockfd;
	}

	for ( ; ; ) {
		tmpset = rdset;
		while ((nready = select(maxi + 1, &tmpset, NULL, NULL, NULL)) < 0) {
			if (errno == EINTR) {
				errno = 0;
				continue;
			}
			perror("select error");
			exit(1);
		}
		
		for (i = 0; i != n; ++i) {
			if (!FD_ISSET(allfd[i], &tmpset))
				continue;

			clilen = sizeof(cliaddr);
			if ((nread = recvfrom(allfd[i], buf, BUFSIZ, 0, (struct sockaddr *)&cliaddr, &clilen)) < 0) {
				perror("recvfrom error");
				exit(1);
			}
			if (sendto(allfd[i], buf, nread, 0, (struct sockaddr *)&cliaddr, clilen) < 0) {
				perror("sendto error");
				exit(1);
			}

			if (--nready == 0)
				break;
		}
	}

	exit(0);
}
