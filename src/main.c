#include <dhcpext/tx.h>
#include <dhcpext/rx.h>
#include <dhcpext/pc.h>
#include <dhcpext/rp.h>
#include <dhcpext/stream.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

tStream *s_tx, *s_pc, *s_rp;
int txSock[2], rxSock[2];

void sigintHandler(int);

void sigintHandler(int signo) {
	if(signo == SIGINT) {
		send(txSock[0], "shutdown", strlen("shutdown") + 1, 0);
		send(rxSock[0], "shutdown", strlen("shutdown") + 1, 0);		
	}
}

int main(int argc, char** argv) {
	if(argc != 4) {
		printf("Usage:\n\t%s <broadcast address> <direct address> <broadcast flag>\n",
			argv[0]);
		return 0;
	}

	pthread_t tx, rx, pc, rp;

/*	s_tx = malloc(sizeof(tStream));

	if(s_tx == NULL) {
		printf("malloc error in ct\n");
		exit(EXIT_FAILURE);
	}*/

	if(socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, txSock) != 0) {
		printf("Error in setting up socket pair to TX\n");
		exit(EXIT_FAILURE);
	}

	if(socketpair(AF_LOCAL, SOCK_SEQPACKET, 0, rxSock) != 0) {
		printf("Error in setting up socket pair to RX\n");
		exit(EXIT_FAILURE);
	}

	s_pc = malloc(sizeof(tStream));

	if(s_pc == NULL) {
		printf("malloc error in ct\n");
		exit(EXIT_FAILURE);
	}

	s_rp = malloc(sizeof(tStream));

	if(s_rp == NULL) {
		printf("malloc error in ct\n");
		exit(EXIT_FAILURE);
	}

	// stream_init(s_tx);
	stream_init(s_pc);
	stream_init(s_rp);

	if(signal(SIGINT, sigintHandler) == SIG_ERR) {
		fprintf(stderr, "Unable to catch SIGINT.\n");
		exit(EXIT_FAILURE);
	}

	if(pthread_create(&tx, NULL, txmain, &txSock[1]) != 0) {
		fprintf(stderr, "Error: failed to start tx thread\n");
		exit(EXIT_FAILURE);
	}

	if(pthread_create(&rx, NULL, rxmain, &rxSock[1]) != 0) {
		fprintf(stderr, "Error: failed to start rx thread\n");
		exit(EXIT_FAILURE);
	}

	if(pthread_create(&pc, NULL, pcmain, s_pc) != 0) {
		fprintf(stderr, "Error: failed to start pc thread\n");
		exit(EXIT_FAILURE);
	}

	if(pthread_create(&rp, NULL, rpmain, s_rp) != 0) {
		fprintf(stderr, "Error: failed to start rp thread\n");
		exit(EXIT_FAILURE);
	}

	char* str_txconfig = malloc(strlen(argv[1]) + strlen(argv[2]) + 2);
	strcpy(str_txconfig, argv[1]);
	strcat(str_txconfig, " ");
	strcat(str_txconfig, argv[2]);

	send(txSock[0], str_txconfig, strlen(str_txconfig) + 1, 0);
	free(str_txconfig);

	int flags = 0;

	if(strcmp(argv[3], "1") == 0) {
		flags |= TXFLAGS_BCAST;
	}
	
	send(txSock[0], (char*)(&flags), sizeof(int), 0);

	tStream* s_tx_pc = malloc(sizeof(tStream));

	if(s_tx_pc == NULL) {
		printf("malloc error in ct\n");
		exit(EXIT_FAILURE);
	}

	tStream* s_rx_pc = malloc(sizeof(tStream));

	if(s_rx_pc == NULL) {
		printf("malloc error in ct\n");
		exit(EXIT_FAILURE);
	}

	tStream* s_rp_pc = malloc(sizeof(tStream));

	if(s_rp_pc == NULL) {
		printf("malloc error in ct\n");
		exit(EXIT_FAILURE);
	}

	stream_init(s_tx_pc);
	stream_init(s_rx_pc);
	stream_init(s_rp_pc);

	send(txSock[0], (char*)(&s_tx_pc), sizeof(tStream*), 0);
	stream_send(s_pc, (char*)(&s_tx_pc), sizeof(tStream*));

	send(rxSock[0], (void*)(&s_rx_pc), sizeof(tStream*), 0);
	stream_send(s_pc, (char*)(&s_rx_pc), sizeof(tStream*));

	stream_send(s_rp, (char*)(&s_rp_pc), sizeof(tStream*));
	stream_send(s_pc, (char*)(&s_rp_pc), sizeof(tStream*));

	pthread_join(tx, NULL);
	pthread_join(rx, NULL);
		// Wait for the threads to finish before we exit

	stream_send(s_pc, "shutdown", strlen("shutdown") + 1);
		// Wait for the other threads to end, before we close down
		// PC, due to the potential for data to be lost in a stream
		// without being properly free'd

	pthread_join(pc, NULL);
		// Wait for PC to finish as well

	stream_send(s_rp, "shutdown", strlen("shutdown") + 1);
	pthread_join(rp, NULL);
		// End RP after PC, as it will continue to send traffic through
		// until all streams have been emptied
	
	// stream_free(s_tx);
	stream_free(s_pc);
	stream_free(s_rp);

	stream_free(s_tx_pc);
	stream_free(s_rx_pc);
	stream_free(s_rp_pc);

	// free(s_tx);
	// free(s_rx);
	free(s_pc);
	free(s_rp);

	free(s_tx_pc);
	free(s_rx_pc);
	free(s_rp_pc);

	close(txSock[0]);
	close(rxSock[0]);

	return 0;
}
