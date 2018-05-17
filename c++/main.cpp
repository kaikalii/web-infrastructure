#include <SFML/Network.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <ctime>

using namespace std;

int main(int argc, char **argv) {

	sf::TcpListener listener;
	// bind the listener to a port
	if (listener.listen(53011) != sf::Socket::Done) cout << "unable to bind listener to port" << endl;
	else cout << "listener bound" << endl;
	// accept a new connection
	sf::TcpSocket in_socket;
	if (listener.accept(in_socket) != sf::Socket::Done) cout << "client error" << endl;
	else cout << "accepted client" << endl;

	char data[1000000];
	size_t received;
	bool browser_connected = false;
	while(true) {
		in_socket.receive(data, 1000000, received);
		cout << endl << data << endl;

		string req_str = string(data);
		if(!browser_connected || req_str.substr(0,7) == "CONNECT") {
			string ok = "HTTP/1.1 200 OK";
			if(in_socket.send(&ok[0], ok.size()) != sf::Socket::Done) cout << "unable to send ok to browser" << endl;
			else {
				cout << "ok sent to browser" << endl;
				browser_connected = true;
			}
		} else {
			string host;
			int i;
			for(i = 0; req_str.substr(i,5) != "Host:" && i < req_str.size(); i++);
			i += 6;
			for(; req_str[i] != '\n' && req_str[i] != ':'; i++) {
				host.push_back(req_str[i]);
			}

			sf::IpAddress host_ip(host);
			cout << "Host: " << host << endl;
			cout << "Ip: " << host_ip.toString() << endl;

			sf::TcpSocket out_socket;
			sf::Socket::Status status = out_socket.connect(host, 2225);
			if(status != sf::Socket::Done) cout << "unable to setup out socket" << endl;
			else {
				cout << "setup out socket" << endl;

				if(out_socket.send(&req_str[0], req_str.size()) != sf::Socket::Done) cout << "unable to send request" << endl;
				else {
					cout << "request sent" << endl;

					if(out_socket.receive(data, 1000000, received) != sf::Socket::Done) cout << "unable to get response" << endl;
					else {
						cout << "got response" << endl;

						string resp_str(data);
						if(in_socket.send(&resp_str[0], resp_str.size()) != sf::Socket::Done) cout << "unable to forward response" << endl;
						else cout << "response forwarded" << endl;
					}
				}
			}
		}
	}


    return 0;
}
