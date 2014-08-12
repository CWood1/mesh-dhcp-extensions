#ifndef __PROTO_H__
#define __PROTO_H__

#include <stdint.h>

typedef struct {
	uint32_t filelen;
	char* file;
} leasefile;

typedef struct nodelist {
	char* ip;
	struct nodelist* next;
} nodelist;

typedef struct {
	uint8_t ident;
	uint32_t flags;
	uint32_t magic;

	leasefile* l;
	nodelist* n;
} heartbeat;

typedef struct {
	uint8_t ident;
	uint32_t flags;
	uint32_t magic;

	uint32_t numnodes;
	uint8_t shutdown;
		// Only ever used if D flag is set
} response;

heartbeat* craftHeartbeat(int);
char* serializeHeartbeat(heartbeat*, int*);
heartbeat* deserializeHeartbeat(char*, int);
void printHeartbeat(heartbeat*);

response* craftResponse(heartbeat*);
char* serializeResponse(response*, int*);
response* deserializeResponse(char*, int);
void printResponse(response*);

int isHeartbeat(char*, int);

#endif
