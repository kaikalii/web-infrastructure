#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#define BROWSER_PORT "53011"
#define OUT_PORT "53012"
#define DEFAULT_BUFLEN 10000
#define CACHE_MAX 1000

static char* cache_requests[CACHE_MAX];
static char* cache_responses[CACHE_MAX];
static unsigned cache_size;

int main() {
	WSADATA wsaData;
	// Initialize Winsock
	int i_res;
	i_res = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (i_res != 0) {
		printf("WSAStartup failed: %d\n", i_res);
		return 1;
	}
	// Initialize tcp listener
	SOCKET listener = INVALID_SOCKET;
	{
		struct addrinfo *result = NULL, *ptr = NULL, hints;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		// Resolve the local address and port to be used by the server
		i_res = getaddrinfo(NULL, BROWSER_PORT, &hints, &result);
		if (i_res != 0) {
		    printf("getaddrinfo failed: %d\n", i_res);
		    // WSACleanup();
		    return 1;
		}
		// Create a SOCKET for the server to listen for client connections
		listener = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (listener == INVALID_SOCKET) {
		    printf("Error at socket(): %ld\n", WSAGetLastError());
		    freeaddrinfo(result);
		    // WSACleanup();
		    return 1;
		}
		// Setup the TCP listening socket
	    i_res = bind(listener, result->ai_addr, (int)result->ai_addrlen);
	    if (i_res == SOCKET_ERROR) {
	        printf("bind failed with error: %d\n", WSAGetLastError());
	        freeaddrinfo(result);
	        closesocket(listener);
	        // WSACleanup();
	        return 1;
	    }
		freeaddrinfo(result);
	}
	// Listen
	if (listen(listener, SOMAXCONN) == SOCKET_ERROR) {
	    printf( "Listen failed with error: %ld\n", WSAGetLastError());
	    closesocket(listener);
	    // WSACleanup();
	    return 1;
	}
	// Setup browser socket
	SOCKET browser_socket;
	browser_socket = INVALID_SOCKET;

	// Accept the browser socket
	browser_socket = accept(listener, NULL, NULL);
	if (browser_socket == INVALID_SOCKET) {
	    printf("accept failed: %d\n", WSAGetLastError());
	    closesocket(listener);
	    // WSACleanup();
	    return 1;
	}
	u_long one = 1;
	ioctlsocket(browser_socket, 0, &one);

	char data[DEFAULT_BUFLEN];
	int i_send_res;
	int datalen = DEFAULT_BUFLEN;

	// Main loop
	while(1) {
		char request[1000000];
		int reqlen = 0;
		// get the request
		printf("Waiting for a request from the browser...\n");
		do {
		    i_res = recv(browser_socket, data, datalen, 0);
		    if (i_res > 0) {
		        printf("Bytes received: %d\n", i_res);
				int i;
				for(i = 0; i < i_res; i++) request[reqlen + i] = data[i];
				reqlen += i_res;
		    } else if (i_res == 0) printf("Connection closing...\n");
		    else {
		        printf("request recv failed: %d\n", WSAGetLastError());
		        closesocket(browser_socket);
		        // WSACleanup();
		        // return 1;
		    }
		} while(i_res == datalen);
		printf("\nRequest:\n%s", request);

		if(request[0] != 'G') continue;

		// Check cache for request
		int i;
		int response_cached = -1;
		for(i = 0; i < cache_size; i++) {
			if(!strcmp(cache_requests[i], request)) {
				printf("Found cached request\n");
				response_cached = i;
				goto send_response;
			}
		}
		// Cache the request
		if(cache_requests[cache_size]) free(cache_requests[cache_size]);
		cache_requests[cache_size] = malloc(strlen(&request[0]));
		strcpy(cache_requests[cache_size], &request[0]);

		// determine the host name
		char host[1000] = "";
		char port_buff[6] = "80";
		for(i = 0; i < reqlen - 5; i++) {
			if(request[i] == 'H' && request[i+1] == 'o' && request[i+2] == 's' && request[i+3] == 't' && request[i+4] == ':') {
				i += 6;
				int j;
				for(j = 0; request[i+j] != '\r' && request[i+j] != ':' && j < 1000; j++) {
					host[j] = request[i+j];
				}
				host[j] = '\0';
				int k;
				if(request[i+j] == ':') {
					j += 1;
					for(k = 0; request[i+j+k] != '\r' && k < 5; k++) {
						port_buff[k] = request[i+j+k];
					}
					port_buff[5] ='\0';
				}
			}
		}

		printf("Host: %s\n", host);
		printf("Port: %s\n", port_buff);

		// setup the out socket
		struct addrinfo *result = NULL,  *ptr = NULL, hints;
		SOCKET out_socket = INVALID_SOCKET;

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		// Resolve the server address and port
		i_res = getaddrinfo(host, port_buff, &hints, &result);
		if (i_res != 0) {
			printf("getaddrinfo failed: %d\n", i_res);
			// WSACleanup();
			// return 1;
		}
		else {
			// Attempt to connect to the first address returned by the call to getaddrinfo
			ptr = result;

			// Create a SOCKET for connecting to server
			out_socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

			if (out_socket == INVALID_SOCKET) {
				printf("Error at socket(): %ld\n", WSAGetLastError());
				freeaddrinfo(result);
				// WSACleanup();
				// return 1;
			} else {
				printf("socket setup\n");
				// Connect to server
				i_res = connect(out_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
				if (i_res == SOCKET_ERROR) {
					closesocket(out_socket);
					out_socket = INVALID_SOCKET;
					printf("unable to connect out socket\n");
				} else printf("socket connected\n");

				// Should really try the next address returned by getaddrinfo if the connect call failed
				// But for this simple example we just free the resources returned by getaddrinfo and print an error message

				freeaddrinfo(result);

				if (out_socket == INVALID_SOCKET) {
					printf("Unable to connect to server!\n");
					// WSACleanup();
					// return 1;
				}

				// forward the request
				i_res = send(out_socket, request, reqlen, 0);
				if (i_res == SOCKET_ERROR) {
					printf("Unable to forward request!\n");
					// WSACleanup();
					// return 1;
				}
				// get the response
				else {
					printf("request forwarded\n");
					char response[1000000];
					int resplen = 0;

					printf("Waiting for a response from the server...\n");
					do {
						i_res = recv(out_socket, data, datalen, 0);
						if (i_res > 0) {
							printf("Bytes received: %d\n", i_res);
							int i;
							for(i = 0; i < i_res; i++) response[resplen + i] = data[i];
							resplen += i_res;
						} else if (i_res == 0) printf("Connection closing...\n");
						else {
							printf("response recv failed: %d\n", WSAGetLastError());
							closesocket(out_socket);
							// WSACleanup();
							// return 1;
						}
					} while(i_res == datalen);

					send_response:
					if(response_cached >= 0) {
						strcpy(&response[0], cache_responses[response_cached]);
					}
					printf("\nResponse:\n%s\n", response);

					// Cache the response
					if(response_cached == -1) {
						if(cache_responses[cache_size]) free(cache_responses[cache_size]);
						cache_responses[cache_size] = malloc(strlen(&response[0]));
						strcpy(cache_responses[cache_size], &response[0]);
						cache_size++;
					}

					// forward the response
					i_res = send(browser_socket, response, resplen, 0);
					if (i_res == SOCKET_ERROR) {
						printf("Unable to forward response!\n");
						// WSACleanup();
						// return 1;
					} else printf("response forwarded\n");
				}
			}
		}

	}

	return 0;
}
