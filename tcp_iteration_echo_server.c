#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void echo_serv(int fd);

int main(int argc, char *argv[])
{
	int					listenfd, connfd, opt;
	const char			*s_ip, *s_port;
	socklen_t			clilen;
	struct sockaddr_in	servaddr, cliaddr;

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
		s_ip = "0.0.0.0";
	if (s_port == NULL)
		s_port = "2333";

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		exit(1);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(s_port));
	if (inet_pton(AF_INET, s_ip, &servaddr.sin_addr) != 1) {
		fprintf(stderr, "inet_pton: %s: parse failure\n", s_ip);
		exit(1);
	}
	if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("bind error");
		exit(1);
	}

	if (listen(listenfd, 10) < 0) {
		perror("listen error");
		exit(1);
	}

	for ( ; ; ) {
		clilen = sizeof(cliaddr);
		while ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0) {
			if (errno == EINTR) {
				errno = 0;
				continue;
			}
			perror("accept error");
			exit(1);
		}

		echo_serv(connfd);
		close(connfd);
	}

	exit(0);
}

ssize_t readn(int fd, void *buf, size_t n)
{
	ssize_t	nread, nleft;
	char	*ptr;

	nleft = n;
	ptr = (char *)buf;

	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR) {
				errno = 0;
				continue;
			}
			perror("read error");
			exit(1);
		} else if (nread > 0) {
			nleft -= nread;
			ptr += nread;
		} else {
			break;
		}
	}

	return (n - nleft);
}

ssize_t writen(int fd, const void *buf, size_t n)
{
	ssize_t		nwrite, nleft;
	const char	*ptr;

	nleft = n;
	ptr = (const char *)buf;

	while (nleft > 0) {
		if ((nwrite = write(fd, ptr, nleft)) <= 0) {
			if (errno == EINTR) {
				errno = 0;
				continue;
			}
			perror("write error");
			exit(1);
		} else {
			nleft -= nwrite;
			ptr += nwrite;
		} 
	}

	return (n - nleft);
}

void echo_serv(int connfd)
{
	char	buf[BUFSIZ];
	ssize_t	nread;

	for ( ; ; ) {
		while ((nread = read(connfd, buf, BUFSIZ)) < 0) {
			if (errno == EINTR) {
				errno = 0;
				continue;
			}
			perror("read error");
			exit(1);
		}
		if (nread == 0)
			break;
		writen(connfd, buf, nread);
	}
}
