#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void echo_cli(FILE *fp, int sockfd);

int main(int argc, char *argv[])
{
	int					sockfd, opt;
	const char			*s_ip, *s_port, *l_ip, *l_port;
	struct sockaddr_in	cliaddr, servaddr;

	s_ip = s_port = l_ip = l_port = NULL;

	while ((opt = getopt(argc, argv, "s:p:b:l:")) != -1) {
		switch (opt) {
		case 's':
			s_ip = optarg;
			break;
		case 'p':
			s_port = optarg;
			break;
		case 'b':
			l_ip = optarg;
			break;
		case 'l':
			l_port = optarg;
			break;
		default:
			fprintf(stderr, "Usage: %s [-s <server ip>] [-p <server port>] "
							"[-b <local ip>] [-l <local port>]\n", argv[0]);
			exit(1);
		}
	}
	if (optind != argc) {
		fprintf(stderr, "Usage: %s [-s <server ip>] [-p <server port>] "
						"[-b <local ip>] [-l <local port>]\n", argv[0]);
		exit(1);
	}

	if (s_ip == NULL)
		s_ip = "127.0.0.1";
	if (s_port == NULL)
		s_port = "2333";
	if (l_ip == NULL)
		l_ip = "127.0.0.1";
	if (l_port == NULL)
		l_port = "2444";

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		exit(1);
	}

	memset(&cliaddr, 0, sizeof(cliaddr));
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_port = htons(atoi(l_port));
	if (inet_pton(AF_INET, l_ip, &cliaddr.sin_addr) != 1) {
		fprintf(stderr, "inet_pton: %s: prase failure\n", l_ip);
		exit(1);
	}
	if (bind(sockfd, (struct sockaddr *)&cliaddr, sizeof(cliaddr)) < 0) {
		perror("bind error");
		exit(1);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(s_port));
	if (inet_pton(AF_INET, s_ip, &servaddr.sin_addr) != 1) {
		fprintf(stderr, "inet_pton: %s: prase failure\n", s_ip);
		exit(1);
	}
	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("connect error");
		exit(1);
	}

	echo_cli(stdin, sockfd);
	close(sockfd);

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

void echo_cli(FILE *fp, int sockfd)
{
	char	buf[BUFSIZ];
	fd_set	rdset;
	int		nready, maxfd;
	ssize_t	nread;

	nready = 2;
	while (nready != 0) {
		FD_ZERO(&rdset);
		FD_SET(sockfd, &rdset);
		if (nready == 2) {
			FD_SET(fileno(fp), &rdset);
			maxfd = sockfd > fileno(fp) ? sockfd : fileno(fp);
		} else {
			maxfd = sockfd;
		}
		if (select(maxfd + 1, &rdset, NULL, NULL, NULL) < 0) {
			perror("select error");
			exit(1);
		}

		if (FD_ISSET(sockfd, &rdset)) {
			if ((nread = read(sockfd, buf, BUFSIZ)) < 0) {
				perror("read error");
				exit(1);
			} else if (nread == 0) {
				if (nready == 1) {
					return;
				} else {
					fprintf(stderr, "echo_cli: server terminated prematurely\n");
					exit(1);
				}
			}

			writen(STDOUT_FILENO, buf, nread);
		}

		if (FD_ISSET(fileno(fp), &rdset)) {
			if ((nread = read(fileno(fp), buf, BUFSIZ)) < 0) {
				perror("read error");
				exit(1);
			} else if (nread == 0) {
				shutdown(sockfd, SHUT_WR);
				--nready;
				continue;
			}

			writen(sockfd, buf, nread);
		}
	}
}
