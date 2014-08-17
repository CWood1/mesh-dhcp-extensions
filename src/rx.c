#include "rx.h"
#include "pc.h"
#include "common.h"
#include "stream.h"
#include "proto.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

void* rxmain(void* stream) {
	int sd, rc;
	struct sockaddr_in selfaddr, senderaddr, replyaddr;
		// Structs for the sender of the heartbeat, and receiver of the heartbeat

	char buffer[100];
		// TODO: Clean up size here, to maximum needed size

	tStream* cmdStream = (tStream*)stream;

	tStream** pPcStream = malloc(stream_wait_full(cmdStream));
	stream_rcv(cmdStream, 0, (char*)pPcStream);

	tStream* pcStream = *pPcStream;
	free(pPcStream);

	if((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Heartbeat receive - socket error.");
		pthread_exit(NULL);
	}

	memset(&selfaddr, 0, sizeof(selfaddr));
	selfaddr.sin_family = AF_INET;
	selfaddr.sin_port = htons(PORT);
	selfaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	memset(&replyaddr, 0, sizeof(replyaddr));
	replyaddr.sin_family = AF_INET;
	replyaddr.sin_port = htons(PORT);

	if((rc = bind(sd, (struct sockaddr*)&selfaddr, sizeof(selfaddr))) < 0) {
		perror("Heartbeat receive - bind error.");
		close(sd);
		pthread_exit(NULL);
	}

	while(1) {
		int senderaddrlen = sizeof(senderaddr);
		rc = recvfrom(sd, (char*)buffer, sizeof(buffer), MSG_DONTWAIT,
			(struct sockaddr*)&senderaddr, &senderaddrlen);

		if(rc < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
			perror("recvfrom error");
			close(sd);
			pthread_exit(NULL);
		} else if(rc >= 0) {
			message* m = malloc(sizeof(message));
			m->buffer = malloc(rc);
			memcpy(m->buffer, (void*)buffer, rc);
			m->addrv4 = senderaddr.sin_addr.s_addr;
			m->bufferSize = rc;
			
			stream_send(pcStream, (char*)m, sizeof(message));

/*			if(isHeartbeat((char*)buffer, rc)) {
				printf("Heartbeat received (%s):\n",
					inet_ntoa(senderaddr.sin_addr));

				replyaddr.sin_addr.s_addr = senderaddr.sin_addr.s_addr;

				heartbeat* h = deserializeHeartbeat((char*)buffer, rc);
				printHeartbeat(h);

				stream_send(pcStream, h, sizeof(heartbeat*));

				int size;

				response* r = craftResponse(h);
				char* b = serializeResponse(r, &size);

				rc = sendto(sd, b, size, 0,
					(struct sockaddr*)&replyaddr, sizeof(replyaddr));

				free(r);
				free(b);

				if(rc < 0) {
					perror("sendto error");
					close(sd);
					pthread_exit(NULL);
				}
			} else {
				printf("Response received (%s):\n",
					inet_ntoa(senderaddr.sin_addr));

				response* r = deserializeResponse((char*)buffer, rc);
				printResponse(r);
				free(r);
			}*/
		}

		int size = stream_length(cmdStream);
		if(size != 0) {
			char* cmd = malloc(stream_wait_full(cmdStream));
			stream_rcv(cmdStream, 0, cmd);

			char* t = strtok(cmd, " ");

			if(strcmp(t, "shutdown") == 0) {
				printf("rx shutting down.\n");
				close(sd);
				pthread_exit(NULL);
			}
		}
	}
}
