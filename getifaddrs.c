#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main()
{
	struct ifaddrs		*ifap;
	struct sockaddr_in	*inaddrp;
	struct sockaddr_in6	*in6addrp;
	char				inaddr_buf[INET_ADDRSTRLEN], in6addr_buf[INET6_ADDRSTRLEN];

	if (getifaddrs(&ifap) < 0) {
		perror("getifaddrs error");
		exit(1);
	}

	for ( ; ifap != NULL; ifap = ifap->ifa_next) {
		if (ifap->ifa_addr->sa_family == AF_INET) {
			inaddrp = (struct sockaddr_in *)ifap->ifa_addr;
			if (inet_ntop(AF_INET, &inaddrp->sin_addr, inaddr_buf, INET_ADDRSTRLEN) == NULL) {
				perror("inet_ntop error");
				exit(1);
			}

			fprintf(stderr, "%s: %s\n", ifap->ifa_name, inaddr_buf);
		} else if (ifap->ifa_addr->sa_family == AF_INET6) {
			in6addrp = (struct sockaddr_in6 *)ifap->ifa_addr;
			if (inet_ntop(AF_INET6, &in6addrp->sin6_addr, in6addr_buf, INET6_ADDRSTRLEN) == NULL) {
				perror("inet_ntop error");
				exit(1);
			}

			fprintf(stderr, "%s: %s\n", ifap->ifa_name, in6addr_buf);
		} 
	}

	exit(0);
}
