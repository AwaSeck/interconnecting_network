#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <pthread.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SIZE_FILE 15668   // size of the file
#define NWK_PORT 2000  // the starting port of our network because ports under 1024 are not usables

typedef struct nodes // Represent, in the file format, the edges between nodes
{
    char e;
    int n1;
    int n2;
}nodes;

void acceptClient(int server_fd);

char userID[10];
int PORT;

void connexionToServer(FILE* file) // like client
{
	//if(isConecting)
	char buffer[1024] = {0};
	int PORT_server;

	printf("\nEnter your neighbord port to send messages: ");
	scanf("%d", &PORT_server);

	// Testing whether the server is my neighbor
	nodes nd[SIZE_FILE];
	int count=0, myID=PORT;

	for (int i = 0; i < SIZE_FILE; i++)
	{
		if (fscanf(file, "%c %d %d\n", &nd[count].e, &nd[count].n1, &nd[count].n2))
		{
			count++;
		}
	}

	for (int i = 0; i < count; i++) {  
		if (NWK_PORT+nd[i].n1==myID)
		{
			if (NWK_PORT+nd[i].n2==PORT_server)
			{
				int remote_peer_socket;
				struct sockaddr_in remote_peer_address;

				if ((remote_peer_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
					perror("Error in calling socket (Client)\n");
					getchar();
					exit(1);
				}

				remote_peer_address.sin_family = AF_INET;
				remote_peer_address.sin_port = htons(PORT_server);
				remote_peer_address.sin_addr.s_addr = htonl(INADDR_ANY);

				if (connect(remote_peer_socket, (struct sockaddr*)&remote_peer_address, sizeof(remote_peer_address)) == -1) {
					perror("Connection error (Client)\n");
					close(remote_peer_socket);
					getchar();
					exit(1);
				}

				//fgets(buffer, sizeof(buffer), stdin);
				char message[1024] = {0};
				printf("\nWrite a message like: 'neighbor port + message' : ");
				scanf("%s", message);
				sprintf(buffer, "User %s (port:%d) sends: %s", userID, PORT, message);
				send(remote_peer_socket, buffer, sizeof(buffer), 0);
				
				close(remote_peer_socket);
				break;
				
			}	
		}
		else{
			if (i==count-1)
			{
				printf("\nThis port is not my neighbor. Try with another port.\n");
				break;
			}
		}
    }
}

void* client_thread(void* server_fd) // reveive messages from another peer (client)
{
	int serv_fd = *((int*)server_fd);
	while (1) 
	{
		sleep(2);
		acceptClient(serv_fd);
	}
}

void acceptClient(int server_fd)
{
	struct sockaddr_in client_address;
	char buffer[1024] = {0};
	socklen_t client_address_length = sizeof(client_address);
	fd_set current_sockets, ready_sockets;

	FD_ZERO(&current_sockets);
	FD_SET(server_fd, &current_sockets);

	int count = 0;
	while (1) 
	{
		count++;
		ready_sockets = current_sockets;

		if (select(FD_SETSIZE, &ready_sockets, NULL, NULL, NULL) < 0)
		{
			perror("Error select() (Server)\n");
			getchar();
			exit(1);
		}

		for (int i = 0; i < FD_SETSIZE; i++)
		{
			if (FD_ISSET(i, &ready_sockets))
			{
				if (i == server_fd)
				{
					int client_socket;
					if ((client_socket = accept(server_fd, (struct sockaddr*)&client_address, &client_address_length)) < 0)
					{
						perror("Error accept() (Server)\n");
						close(client_socket);
						getchar();
						exit(1);
					}
					FD_SET(client_socket, &current_sockets);
				}
				else 
				{
					recv(i, buffer, sizeof(buffer), 0);
					printf("\n%s\n", buffer);
					FD_CLR(i, &current_sockets);
				}
			}
		}
		if (count == (FD_SETSIZE * 2)) break; 
	}
}

int main (int argc, char** argv) 
{
	//printf("Enter your ID : ");
	//scanf("%s", userId);
	if (argc!=2)
	{
		printf("Use: %s + graph_file in string format\n", argv[0]);
    	exit(1);
	}
	
	printf("Enter your ID: ");
	scanf("%s", userID);

	printf("Enter your PORT: "); 
	scanf("%d", &PORT);

	int server_fd;
	struct sockaddr_in server_address;

	// Getting socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        perror("Error socket (main)\n");
        getchar();
        exit(1);
    }

    // Attaching socket
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    // Print the server socket address and port
    char server_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(server_address.sin_addr), server_ip, INET_ADDRSTRLEN);

    printf("My adress is: %s with port %d\n", server_ip, (int)ntohs(server_address.sin_port));
	

	if (bind(server_fd, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Error bind (main)\n");
		close(server_fd);
		getchar();
        exit(1);
    }

    if (listen(server_fd, 5) == -1) {
		perror("Error listen(main)\n");
		close(server_fd);
		getchar();
        exit(1);
	}


	pthread_t thread_id;
	pthread_create(&thread_id, NULL, &client_thread, &server_fd);

	FILE * file=fopen(argv[1], "r");
	if (file==NULL)
	{
		printf("Error file");
		exit(1);
	}else{
		while(1) connexionToServer(file); // getch loop
	}

	shutdown(server_fd, SHUT_RDWR);
	close(server_fd);

	return 0;
}

