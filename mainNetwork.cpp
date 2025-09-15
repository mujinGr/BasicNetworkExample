
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

static void server() {
	// create socket
	const int fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket error:");
		return;
	}

	// allow socket reuse to avoid "Address already in use" errors
	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	// bind to open port
	struct sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = 0; // let system choose port

	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr))) {
		perror("bind error:");
		close(fd);
		return;
	}

	// read port
	socklen_t addr_len = sizeof(addr);
	getsockname(fd, (struct sockaddr*)&addr, &addr_len);
	printf("server is on port %d\n", (int)ntohs(addr.sin_port));

	// start listening
	if (listen(fd, 5)) {
		perror("listen error:");
		close(fd);
		return;
	}

	printf("Server listening for connections...\n");

	// accept connections in a loop
	while (1) {
		struct sockaddr_storage caddr;
		socklen_t caddr_len = sizeof(caddr);

		printf("Waiting for connection...\n");
		const int cfd = accept(fd, (struct sockaddr*)&caddr, &caddr_len);
		if (cfd < 0) {
			perror("accept error:");
			continue; // try again
		}

		printf("Client connected!\n");

		// Keep receiving messages from this client until they disconnect
		while (1) {
			char buf[1024];
			memset(buf, 0, sizeof(buf)); // clear buffer

			ssize_t bytes_received = recv(cfd, buf, sizeof(buf) - 1, 0);
			if (bytes_received > 0) {
				buf[bytes_received] = '\0'; // ensure null termination
				printf("client says: %s\n", buf);
			}
			else if (bytes_received == 0) {
				printf("Client disconnected\n");
				break; // client closed connection
			}
			else {
				perror("recv error:");
				break; // error occurred
			}
		}

		close(cfd);
		printf("Connection closed\n\n");
	}

	close(fd);
}

static void client(int port) {
	const int fd = socket(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		perror("socket error:");
		return;
	}

	struct sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons((short)port);

	// connect to local machine at specified port
	if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
		perror("inet_pton error:");
		close(fd);
		return;
	}

	// connect to server
	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr))) {
		perror("connect error:");
		close(fd);
		return;
	}

	printf("Connected to server on port %d\n", port);
	printf("Type messages to send to server (Ctrl+C to quit):\n");

	// Keep reading from console and sending to server
	char input[1024];
	while (1) {
		printf("> ");
		fflush(stdout); // ensure prompt is displayed

		// Read line from console
		if (fgets(input, sizeof(input), stdin) == NULL) {
			printf("\nExiting...\n");
			break; // EOF or error
		}

		// Remove newline character if present
		size_t len = strlen(input);
		if (len > 0 && input[len - 1] == '\n') {
			input[len - 1] = '\0';
			len--;
		}

		// Skip empty messages
		if (len == 0) {
			continue;
		}

		// Check for quit command
		if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
			printf("Exiting...\n");
			break;
		}

		// Send message to server
		ssize_t bytes_sent = send(fd, input, strlen(input), 0);
		if (bytes_sent < 0) {
			perror("send error:");
			break;
		}
	}

	close(fd);
}

int main(int argc, char* argv[]) {
	if (argc > 1 && !strcmp(argv[1], "client")) {
		if (argc != 3) {
			fprintf(stderr, "Usage: %s client <port>\n", argv[0]);
			return -1;
		}

		int port;
		if (sscanf(argv[2], "%d", &port) != 1 || port <= 0 || port > 65535) {
			fprintf(stderr, "Invalid port number\n");
			return -1;
		}

		client(port);
	}
	else {
		printf("Starting server...\n");
		server();
	}

	return 0;
}