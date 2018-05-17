#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#define BROWSER_PORT "53011"
#define OUT_PORT "53012"
#define DEFAULT_BUFLEN 1000000

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
		    WSACleanup();
		    return 1;
		}
		// Create a SOCKET for the server to listen for client connections
		listener = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
		if (listener == INVALID_SOCKET) {
		    printf("Error at socket(): %ld\n", WSAGetLastError());
		    freeaddrinfo(result);
		    WSACleanup();
		    return 1;
		}
		// Setup the TCP listening socket
	    i_res = bind(listener, result->ai_addr, (int)result->ai_addrlen);
	    if (i_res == SOCKET_ERROR) {
	        printf("bind failed with error: %d\n", WSAGetLastError());
	        freeaddrinfo(result);
	        closesocket(listener);
	        WSACleanup();
	        return 1;
	    }
		freeaddrinfo(result);
	}
	// Listen
	if (listen(listener, SOMAXCONN) == SOCKET_ERROR) {
	    printf( "Listen failed with error: %ld\n", WSAGetLastError());
	    closesocket(listener);
	    WSACleanup();
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
	    WSACleanup();
	    return 1;
	}
	u_long one = 1;
	ioctlsocket(browser_socket, 0, &one);

	char data[DEFAULT_BUFLEN];
	int i_send_res;
	int datalen = DEFAULT_BUFLEN;

	// Main loop
	while(1) {
		// get the request
		// do {
		    i_res = recv(browser_socket, data, datalen, 0);
		    if (i_res > 0) {
		        printf("Bytes received: %d\n", i_res);
				printf("Request:\n\n%s", data);
		    } else if (i_res == 0) printf("Connection closing...\n");
		    else {
		        printf("recv failed: %d\n", WSAGetLastError());
		        closesocket(browser_socket);
		        WSACleanup();
		        return 1;
		    }
		// } while(i_res > 0);
		printf("request received\n");

		// determine the host name
		char host[300];
		int i;
		for(i = 0; i < datalen - 5; i++) {
			if(data[i] == 'H' && data[i+1] == 'o' && data[i+2] == 's' && data[i+3] == 't' && data[i+4] == ':') break;
		}
		i += 6;
		int j;
		for(j = 0; data[i+j] != '\n' && data[i+j] != ':' && i+j < 300; j++) {
			host[j] = data[i+j];
		}
		host[j] = '\0';
		int port = 80;
		char port_buff[6] = "80";
		// if(data[i+j] == ':') {
		// 	int k;
		// 	for(k = 0;data[i+j+k] != '\n' && k < 6; k++) port_buff[k] = data[i+j+k];
		// 	port = atoi(port_buff);
		// }

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
				WSACleanup();
				return 1;
			} else printf("socket setup\n");
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
				WSACleanup();
				return 1;
			}

			// forward the request
			i_res = send(out_socket, data, datalen, 0);
			if (i_res == SOCKET_ERROR) {
				printf("Unable to forward request!\n");
				WSACleanup();
				return 1;
			}
			// get the response
			else {
				printf("request forwarded\n");
				// get the request
				i_res = recv(out_socket, data, datalen, 0);
				if (i_res > 0) {
					printf("Bytes received: %d\n", i_res);
					printf("Response:\n%s\n", data);
				} else if (i_res == 0) printf("Connection closing...\n");
				else {
					printf("recv failed: %d\n", WSAGetLastError());
					closesocket(out_socket);
					WSACleanup();
					return 1;
				}
				printf("response received\n");

			}
			// forward the response
			i_res = send(browser_socket, data, datalen, 0);
			if (i_res == SOCKET_ERROR) {
				printf("Unable to forward response!\n");
				WSACleanup();
				return 1;
			} else printf("response forwarded\n");
		}

	}

	return 0;
}
