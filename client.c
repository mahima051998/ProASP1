#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdbool.h>
#include<fcntl.h>

#define PORT 3000
#define BUFFER_SIZE 1024

// This function takes a string input as a command and processes it, returning the result as a string.
char* process_command(char* input) {
    // Allocate memory for the result string and create an array to store command arguments.
    char* result = (char*)malloc(sizeof(char) * 1024);
    char* args[2] = {NULL, NULL};

    // If the command matches the "findfile" pattern, find the file and store the result in the result string.
    if (sscanf(input, "findfile %ms", &args[0]) == 1) {
        sprintf(result, "output=$(find ~/ -maxdepth 1 -type f -name %s -exec stat -c '%%s\\t%%w\\t%%n' {} \\; | head -n 1); if [ -z \"$output\" ]; then echo \"File not found\"; else echo -e \"$output\"; fi", args[0]);
        free(args[0]);
    } 
    // If the command matches the "sgetfiles" pattern, get files based on their size range and store the result in the result string.
    else if (sscanf(input, "sgetfiles %ms %ms%*c", &args[0], &args[1]) == 2) {
        snprintf(result, 1024, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f -size +%sc -size -%sc)", args[0], args[1]);
        free(args[0]);
        free(args[1]);
    } 
    // If the command matches the "dgetfiles" pattern, get files modified between two dates and store the result in the result string.
    else if (sscanf(input, "dgetfiles %ms %ms%*c", &args[0], &args[1]) == 2) {
        snprintf(result, 1024, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f -newermt \"%s\" ! -newermt \"%s\")", args[0], args[1]);
        free(args[0]);
        free(args[1]);
    } 
    // If the command matches the "getfiles" pattern, get files based on a list of file names and store the result in the result string.
    else if (sscanf(input, "getfiles %[^\n]", result) == 1) {
        char files[1024] = "";
        char* token = strtok(result, " ");
        int count = 0;
        while (token != NULL) {
            char tmp[256];
            if (count > 0) {
                strcat(files, "-o ");
            }
            snprintf(tmp, 256, "-name %s ", token);
            strcat(files, tmp);
            count++;
            token = strtok(NULL, " ");
        }
        sprintf(result, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f %s)", files);
    } 
    // If the command matches the "gettargz" pattern, get files based on their extension and store the result in the result string.
    else if (sscanf(input, "gettargz %[^\n]", result) == 1) {
        char files[1024] = "";
        // Use strtok to tokenize the "result" string by space
        char* token = strtok(result, " ");
        int count = 0;
        // Loop through each token
        while (token != NULL) {
            // Initialize a temporary character array to store the current file name
            char tmp[256];
            // If this is not the first file name, add "-o" to the "files" array
            if (count > 0) {
                strcat(files, "-o ");
            }
            // Use snprintf to create the command for finding files with the current extension
            snprintf(tmp, 256, "-iname \"*.%s\" ", token);
            // Append the current command to the "files" array
            strcat(files, tmp);
            count++;
             // Move on to the next token
            token = strtok(NULL, " ");
        }
        // Use sprintf to create the command for zipping files with the specified extensions
        sprintf(result, "zip temp.tar.gz $(find ~/ -maxdepth 1 -type f %s)", files);
    } 
    // Check if the input is the "quit" command
    else if (strcmp(input, "quit") == 0) {
        // Exit the program with status code 0
        exit(0);
    } 
    // If the input is not a recognized command
    else {
        // Use snprintf to create an error message
        snprintf(result, 1024, "Invalid Command");
    }
    // Return the result string
    return result;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

// Define the maximum length of user command and server output
#define MAX_COMMAND_LENGTH 1024
#define MAX_OUTPUT_LENGTH 1024

int main()
{
    int client_socket; // Declare a client socket file descriptor
    struct sockaddr_in server_address; // Declare a server address struct
    char user_command[MAX_COMMAND_LENGTH]; // Declare a buffer to store user command input
    char server_output[MAX_OUTPUT_LENGTH]; // Declare a buffer to store server output
    char *result; // Declare a pointer to store the result of the validate function
    int flags, select_result; // Declare variables for storing socket flags and select results

    // Create a TCP socket with IPv4 address family and default protocol (0)
    client_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (client_socket == -1)
    {
        perror("socket failed"); // Print an error message if the socket creation fails
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    memset(&server_address, 0, sizeof(server_address)); // Zero out the server address struct
    server_address.sin_family = AF_INET; // Set the address family to IPv4
    server_address.sin_port = htons(3000); // Set the port number to 3000 and convert it to network byte order
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1"); // Set the IP address to localhost (127.0.0.1)

    // Connect to the server using the client socket and server address
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("connect failed"); // Print an error message if the connection fails
        exit(EXIT_FAILURE);
    }

    // Set the socket to non-blocking mode
    flags = fcntl(client_socket, F_GETFL, 0); // Get the current socket flags
    fcntl(client_socket, F_SETFL, flags | O_NONBLOCK); // Set the socket to non-blocking mode

    // Enter the main loop for sending and receiving data
    while (1)
    {
        // Prompt the user to enter a command
        printf("Enter command: ");
        fgets(user_command, MAX_COMMAND_LENGTH, stdin); // Get user input from stdin

        // Remove the trailing newline character from the user input
        if (user_command[strlen(user_command) - 1] == '\n')
        {
            user_command[strlen(user_command) - 1] = '\0'; // Replace the newline character with a null terminator
        }

        // Call the validate function to validate the user input
        result = validate(user_command); // Pass the user input to the validate function and store the result in the pointer

        // Send the validated command to the server using the client socket
        if (send(client_socket, result, strlen(result), 0) == -1)
        {
            perror("send failed"); // Print an error message if the send operation fails
            exit(EXIT_FAILURE);
        }

        // Use select to check for data received from the server
        fd_set read_fds; // Declare a file descriptor set for reading
        FD_ZERO(&read_fds); // Initialize the file descriptor set to zero
        FD_SET(client_socket, &read_fds); // Add the client socket to the file descriptor set
        struct timeval timeout; // Declare a timeval struct for the timeout value
        timeout.tv_sec = 1; // Set
		timeout.tv_usec = 0;

		select_result = select(client_socket + 1, &read_fds, NULL, NULL, &timeout); // wait until there is data to read or timeout occurs

		if (select_result == -1) // check for errors in select
		{
			perror("select failed"); // print an error message
			exit(EXIT_FAILURE); // exit the program with failure status
		}
		else if (select_result == 0) // if no data is available to read
		{
			// No data received from the server, continue to the next iteration
			continue;
		}
		else // if data is available to read
		{
		// Receive data from the server and print it to the console
		memset(server_output, 0, sizeof(server_output)); // initialize server_output to all zeroes
		while (recv(client_socket, server_output, sizeof(server_output), 0) > 0) // loop through all the data received from the server
		{
			printf("%s", server_output); // print the received data to the console
			memset(server_output, 0, sizeof(server_output)); // clear the buffer for the next iteration
		}
		}

		// Close the client socket
		if (close(client_socket) == -1) // check for errors in closing the socket
		{
			perror("close failed"); // print an error message
			exit(EXIT_FAILURE); // exit the program with failure status
		}

		return 0; // exit the program with success status
		}
