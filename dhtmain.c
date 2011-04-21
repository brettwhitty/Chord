/*
 *	  CSC 501 - HW5 sample code
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <openssl/md5.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>
#include <sys/file.h>

#define BUFLEN 512
#define NODEFILE	"nodelist"


int curr_host;
int TOTAL_NODES;
int portnum = 50000;

void printhash(unsigned char h[16])
{	 int i;
	 for(i=0;i<16;i++)
		 printf("%02x",h[i]);
}

/*
 *  calculates the hash and stores in h
 */
void calculatehash(char *c, int len, char *h)
{	MD5(c,len, h);
}

int get_file_length(int fd)
{
	int end = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	return end;
}

void initialize_host(int portnum) 
{
	int fd, filelen, count = 0;
	char *filestr, *ptr, *tok;
	char buf[100], md5[32];

	fd = open(NODEFILE, O_RDWR | O_CREAT, 0777);
	if (fd < 0) {
		printf("Error opening file %s\n", NODEFILE);
		exit(1);
	}
	flock(fd, LOCK_EX);

	lseek(fd, 0, SEEK_END);
	sprintf(buf, "localhost:%d\n", portnum);
	write(fd, buf, strlen(buf));

	filelen = get_file_length(fd);
	filestr = malloc(filelen + 1);
	read(fd, filestr, filelen);
	filestr[filelen] = '\0';

	tok = strtok(filestr, "\n");
	while (tok != NULL) {
		count++;
		/* ptr = strstr(tok, ":");
		while(*ptr != '\0') {
			*ptr = *(ptr + 1);
			ptr++;
		}
		printf(".%s.\n", tok);
		*/
		tok = strtok(NULL, "\n");
	}

	if (count == TOTAL_NODES) {
		printf("I am %d and I will now start init\n", portnum);
	}

	flock(fd, LOCK_UN);
	close(fd);
}

/*
 * forwards message m to port
 */
void forward_message(int port, char *m)
{
		struct sockaddr_in sock_client;
		struct hostent *hent;
		int sc, i, slen = sizeof(sock_client);

		if ((sc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			printf("socket creation failed ");
			exit(1);
		}

		hent = gethostbyname("localhost");
		if(hent == NULL)
		{	printf("gethostbyname failed ");
			exit(1);
		}

		memset((char *) &sock_client, 0, sizeof(sock_client));

		sock_client.sin_family = AF_INET;
		sock_client.sin_port = htons(port);
		sock_client.sin_addr = *(struct in_addr*)(hent ->h_addr_list[0]);

		if (connect(sc, (struct sockaddr *) &sock_client, slen) == -1) {
			printf("connect failed");
			exit(1);
		}

		if (send(sc, m, BUFLEN, 0) == -1) {
			printf("send failed ");
			exit(1);
		}
		close(sc);
}

void server_listen() {
	struct sockaddr_in sock_server, sock_client;
	int s, slen = sizeof(sock_client);
	char *command;
	char buf[BUFLEN];
	int client;

	srand(time(NULL));

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("error in socket creation");
		exit(1);
	}

	memset((char *) &sock_server, 0, sizeof(sock_server));
	sock_server.sin_family = AF_INET;
	sock_server.sin_port = htons(portnum);
	sock_server.sin_addr.s_addr = htonl(INADDR_ANY);

	/* Each server instance created should listen on a different port. Generate a random number between 1024 to 65535.
 	   Keep on generating new random numbers until bind succeeds.
 	 */
	while (bind(s, (struct sockaddr *) &sock_server, sizeof(sock_server)) == -1) {
		portnum = rand() % ( (65535-1024) + 1024);
		sock_server.sin_port = htons(portnum);
	}

	initialize_host(portnum);

	if (listen(s, 10) == -1) {
		printf("listen error");
		exit(1);
	}

	char nodehash_string[20], hash[16];

	sprintf(nodehash_string,"localhost%d",portnum);
	calculatehash(nodehash_string, strlen(nodehash_string), hash);

	printf("DHT node (");
	printhash(hash);
	printf("): Listening on port number %d . . . \n", portnum);

	while (1) { /* quit only on END message */
		if ((client = accept(s, (struct sockaddr *) &sock_client, &slen)) == -1) {
			printf("accept error");
			exit(1);
		}

		if (recv(client, buf, BUFLEN, 0) == -1) {
			printf("recv error");
			exit(1);
		}

		command = strtok(buf, ":");
		if (strcmp(command, "END") == 0) {

				printf("END message received \n");

		}
		else if (strcmp(command, "PUT") == 0) {



		}
		close(client);
	}

	close(s);
}

int main(int argc, char *argv[]) {

	if (argc != 2) {
		printf("wrong number of arguments");
		return;
	}

	TOTAL_NODES = atoi(argv[1]);
	//curr_host = atoi(argv[2]);

	//initialize_host();
	server_listen();

	return 0;
}

