#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <progbase/net.h>

#define BUFFER_LEN 1024

/**

Сервер містить фіксований масив строк. При старті сервера строки у масиві порожні.

Клієнт може присилати запити серверу на зміну строки по певному індексу в масиві:
	* push <index> <char> - додати символ у кінець строки
	* pop <index> - забрати символ з кінця строки
	* clear <index> - очистити строку
	* get <index> - отримати строку за індексом
	* count - отримати кількість строк у масиві (розмір масиву)
	* getall - отримати масив всіх строк

У відповідь на правильні запити сервер відправляє клієнту строку або масив строк, 
кожна строка у відповіді записується у окремому рядку

*/

typedef struct {
	char command[100];
	int index;
	char character;
} Request;

Request parseRequest(const char * msgStr) {
	Request req = {
		.command = "",
		.index = 0,
		.character = '\0'
	};

	int n = 0;
	while(isalpha(msgStr[n])) n++;

	strncpy(req.command, msgStr, n);
	req.command[n] = '\0';

	char * next = NULL;
	int index = strtol(msgStr + n, &next, 10);
	req.index = index;

	if (NULL != next && *next == ' ') {
		req.character = *(next + 1);
	}

	return req;
}

void printRequest(Request * req) {
	printf("Request: `%s` `%i` `%c`\n",
		req->command,
		req->index,
		req->character);
}

#define ARRAY_LEN 4
#define STRING_LEN 10

int main(void) {

	char strings[ARRAY_LEN][STRING_LEN] = {
		"One",
		"Two 2",
		"3 3 3",
		"Server!"
	};

	//
    // create UDP server
    UdpClient * server = UdpClient_init(&(UdpClient){});
    IpAddress * address = IpAddress_initAny(&(IpAddress){}, 9999);
    if (!UdpClient_bind(server, address)) {
        perror("Can't start server");
        return 1;
    }
    printf("Udp server started on port %d\n", 
        IpAddress_port(UdpClient_address(server)));
    
    NetMessage * message = NetMessage_init(
        &(NetMessage){},  // value on stack
        (char[BUFFER_LEN]){},  // array on stack 
        BUFFER_LEN);

    IpAddress clientAddress;
    while (1) {
        puts("Waiting for data...");
        //
        // blocking call to receive data
        // if someone send it
        if(!UdpClient_receiveFrom(server, message, &clientAddress)) {
			perror("recv");
			return 1;
		}

        printf("Received message from %s:%d (%d bytes): `%s`\n",
            IpAddress_address(&clientAddress),  // client IP-address
            IpAddress_port(&clientAddress),  // client port
            NetMessage_dataLength(message),
            NetMessage_data(message));

		const char * msgStr = NetMessage_data(message);
		Request req = parseRequest(msgStr);
		printRequest(&req);

		int index = req.index;
		if (index < 0 || index >= ARRAY_LEN) {
			NetMessage_setDataString(message, "Error: index out of bounds");	
		} else {
			if (0 == strcmp(req.command, "count")) {
				char tmp[10] = "";
				sprintf(tmp, "%i", ARRAY_LEN);
				NetMessage_setDataString(message, tmp);
			} else if (0 == strcmp(req.command, "get")) {
				
				NetMessage_setDataString(message, strings[index]);
			} else {
				NetMessage_setDataString(message, "Error: unrecognized request");
			}
		}

        //
        // send echo response
        if (!UdpClient_sendTo(server, message, &clientAddress)) {
			perror("send");
			return 1;
		}
    }
    //
    // close server
    UdpClient_close(server);
	return 0;
}
