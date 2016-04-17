#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <signal.h>
#include "friends.h"

#ifndef PORT
    #define PORT 57614
#endif

// Struct points to a user, which represents the user attached to the name that
// the current client is accessing.
struct client {
	int fd;
	struct client *next;
	struct user *person;
} Client ;
// The listen file descriptor for the server.
static int listenfd;
int port = PORT;
char *message = "What is your user name?\n";
// Initialize the linked list of clients as empty.
struct client *head = NULL;

static void addclient(int fd) {
	// Freed later when client is deleted.
    struct client *newclient = malloc(sizeof(struct client));
    if (!newclient) {
        fprintf(stderr, "out of memory!\n");  /* highly unlikely to happen */
        exit(1);
    }
	// Adds client to linked list
    newclient->fd = fd;
    newclient->next = head;
    head = newclient;
}

int setup(void) {
	int on = 1;
	struct sockaddr_in self;
	// Create the socket for the server
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	// Make sure we can reuse the port immediately after the server terminates.
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
				 &on, sizeof(on)) == -1) {
		perror("setsockopt -- REUSEADDR");
	}

	memset(&self, '\0', sizeof(self));
	self.sin_family = AF_INET;
	self.sin_addr.s_addr = INADDR_ANY;
	self.sin_port = htons(PORT);

	// Bind.
	if (bind(listenfd, (struct sockaddr *)&self, sizeof(self)) == -1) {
		perror("bind"); // probably means port is in use
		exit(1);
	}

	// Listen.
	if (listen(listenfd, 5) == -1) {
		perror("listen");
		exit(1);
	}
	return listenfd;
}

void newconnection() {
	socklen_t socklen;
	struct sockaddr_in peer;
	socklen = sizeof(peer);
	int fd;
	// Note that we're passing in valid pointers for the second and third
	// arguments to accept here, so we can actually store and use client
	// information.
	if ((fd = accept(listenfd, (struct sockaddr *)&peer, &socklen)) < 0) {
		perror("accept1");
	} else {
		// add client to linked list.
		addclient(fd);
		// write username message to client.
		write(fd, message, strlen(message));
	}
}

int find_network_newline(char *buf, int inbuf) {
  // Step 1: write this function
  for (int x = 0; x < inbuf; x++) {
	  if (buf[x] == '\r') {
		  return x;
	  }
  }

  return -1;
}

static void removeclient(int fd) {
    struct client **p;
    for (p = &head; *p && (*p)->fd != fd; p = &(*p)->next);
	if (*p) {
		// Remove all traces of the client from linked list.
		struct client *t = (*p)->next;
		// put person as null to incur user checking for every new client.
		(*p)->person = NULL;
		free(*p);
		*p = t;
	} else {
		fprintf(stderr, "Trying to remove fd %d, but I don't know about it\n",
 fd);
		fflush(stderr);
	}    
}

// Helper function that finds the client struct given a name of a user.
struct client *find_client(char *name) {
	struct client *p = head;
	while (p != NULL && strcmp(name, p->person->name) != 0) {
        p = p->next;
    }
    return (struct client *)p;
}

/* 
 * Read and process commands from the chat client.
 * Return:  -1 for quit command
 *          0 otherwise
 */
int process_args(int cmd_argc, char **cmd_argv, User **user_list_ptr, struct 
client *p) {
    User *user_list = *user_list_ptr;
    if (cmd_argc <= 0) {
        return 0;
    } else if (strcmp(cmd_argv[0], "quit") == 0 && cmd_argc == 1) {
		// Sends the return back to process, which deletes the client.
        return -1;
    } else if (strcmp(cmd_argv[0], "list_users") == 0 && cmd_argc == 1) {
		// Implemented in friend.c
		char *templist = list_users(user_list);
		write(p->fd, templist, strlen(templist));
		write(p->fd, "\n", strlen("\n"));
		free(templist);
    } else if (strcmp(cmd_argv[0], "make_friends") == 0 && cmd_argc == 2) {
		// Handles all potential errors.
        switch (make_friends(p->person->name, cmd_argv[1], user_list)) {
            case 1: ;
                char *error1 = "You are already friends.\n";
				write(p->fd, error1, strlen(error1));
                break;
            case 2: ;
                char *error2 = "One of you has the max number of friends\n";
				write(p->fd, error2, strlen(error2));
                break;
            case 4: ;
                char *error4 = "The user you entered does not exist\n";
				write(p->fd, error4, strlen(error4));
                break;
			case 5: ;
				char *error5 = "You can't friend yourself\n";
				write(p->fd, error5, strlen(error5));
                break;
			default: ;
				// Returns success message.
				char friend[64];
				strcpy(friend, "You are now friends with ");
				strcat(friend, cmd_argv[1]);
				strcat(friend, ".\n");
				write(p->fd, friend, strlen(friend));
				// Check to see if the target client is open, if so, post them
				// a message.
				struct client *temp = find_client(cmd_argv[1]);
				char success[64];
				strcpy(success, "You have been friended by ");
				strcat(success, p->person->name);
				strcat(success, ".\n");
				if (temp != NULL) {
					write(temp->fd, success, strlen(success));
				}
        }
    } else if (strcmp(cmd_argv[0], "post") == 0 && cmd_argc >= 3) {
        // first determine how long a string we need
        int space_needed = 0;
        for (int i = 2; i < cmd_argc; i++) {
            space_needed += strlen(cmd_argv[i]) + 1;
        }

        // allocate the space
        char *contents = malloc(space_needed);
        if (contents == NULL) {
            perror("malloc");
            exit(1);
        }

        // copy in the bits to make a single string
        strcpy(contents, cmd_argv[2]);
        for (int i = 3; i < cmd_argc; i++) {
            strcat(contents, " ");
            strcat(contents, cmd_argv[i]);
        }

        User *author = find_user(p->person->name, user_list);
        User *target = find_user(cmd_argv[1], user_list);
		//Handle all errors.
        switch (make_post(author, target, contents)) {
            case 1: ;
				char *error1 = "You can only post to your friends\n";
				write(p->fd, error1, strlen(error1));
                break;
            case 2: ;
                char *error2 = "The user you want to post to does not exist\n";
				write(p->fd, error2, strlen(error2));
                break;
			default: ;
				// Print message to the target client if it is up.
				struct client *temp = find_client(cmd_argv[1]);
				char success[256 + 41];
				strcpy(success, "From: ");
				strcat(success, p->person->name);
				strcat(success, " ");
				strcat(success, contents);
				strcat(success, "\n");
				if (temp != NULL) {
					write(temp->fd, success, strlen(success));
				}
        }
    } else if (strcmp(cmd_argv[0], "profile") == 0 && cmd_argc == 2) {
		// Implemented in friends.c
        User *user = find_user(cmd_argv[1], user_list);
		char *buffer = print_user(user);
		// Checks for user existing.
        if (strcmp(buffer, "\0") == 0) {
			char *not_found_mess = "user not found";
            printf("%s\n", not_found_mess);
			write(p->fd, not_found_mess, strlen(not_found_mess));
			write(p->fd, "\n", strlen("\n"));
        } else {
			write(p->fd, buffer, strlen(buffer));
			write(p->fd, "\n", strlen("\n"));
			free(buffer);
		}
    } else {
		// Catches incorrect syntax otherwise.
		char *error = "Incorrect syntax\n";
        write(p->fd, error, strlen(error));
    }
    return 0;
}

// Tokenize the input from clients.
void process(char *buf, struct client *p, User **user_list){
	char *cmd[256];
	int cmd_len = 0;
	char *curr_cmd = strtok(buf, " ");
	while(curr_cmd != NULL){
		cmd[cmd_len] = curr_cmd;
		cmd_len++;
		curr_cmd = strtok(NULL, " ");
	}
	// Shutdown if quit is called.
	if(cmd_len > 0 && process_args(cmd_len, cmd, user_list, p) == -1){
		shutdown(p->fd, 2);
		removeclient(p->fd);
	}
	
}

int main(void) {
	// Prevents server kill when ctrl C is used to exit a client.
	signal(SIGPIPE,SIG_IGN);
	// Initialize linked list of all users.
	User *user_list = NULL;
	struct client *p;
	// Activate listening server.
	listenfd = setup();
	while (1) {
		//Adapted from sample server code.
		int maxfd = listenfd;
		fd_set fdlist;
		FD_ZERO(&fdlist);
		FD_SET(listenfd, &fdlist);
		//Set FDs and find maxfd for select.
		for (p = head; p; p = p->next) {
			FD_SET(p->fd, &fdlist);
			if (p->fd > maxfd) {
				maxfd = p->fd;
			}
		}
		if (select(maxfd + 1, &fdlist, NULL, NULL, NULL) < 0) {
			perror("select");
		} else {
			// Find a client that is set.
			for (p = head; p; p = p->next){
				if (FD_ISSET(p->fd, &fdlist)) {
					break;
				}
			}
			if (p) {
				int nbytes;
				char buf[256];
				int inbuf;
				int room;
				char *after;
				int where;
				inbuf = 0;
				room = sizeof(buf);
				after = buf;
				while ((nbytes = read(p->fd, after, room)) > 0) {
					inbuf = inbuf + nbytes;
					where = find_network_newline(buf, inbuf);
					if (where >= 0) {
						buf[where] = '\0';
						buf[where + 1] = '\0';
						break;
					}
					after = buf + inbuf;
					room = sizeof(buf) - inbuf;
				}
				// shuts down client on error, or ctrl c.
				if (inbuf <= 0) {
					shutdown(p->fd, 2);
					removeclient(p->fd);
				}
				// Links new clients with a user new or old, otherwise parse
				// input.
				if (p->person == NULL && inbuf > 0) {
					// Case 1 handles usernames that already exist. Case 2
					// truncates input that is too long.
					// Otherwise create the user. All cases link the client to
					// a user.
					switch (create_user(buf, &user_list)) {
						case 1:
							p->person = find_user(buf, user_list);
							char *message_return = 
							"Welcome back.\n\
Go ahead and enter user commands>\n";
							write(p->fd, message_return, strlen(message_return
));
							break;
						case 2: ;
							char nametemp[32];
							strncpy(nametemp, buf, 31);
							int longname = create_user(nametemp, &user_list);
							if (longname != 2) {
								p->person = find_user(nametemp, user_list);
								char *message = 
								"Username too long, truncated to 31 chars.\nGo\
ahead and enter user commands>\n";
								write(p->fd, message, strlen(message));
								break;
							} else {
								perror("namelength error");
								exit(1);
							}
						default:
							p->person = find_user(buf, user_list);
							char *message = 
							"Welcome.\nGo ahead and enter user commands>\n";
							write(p->fd, message, strlen(message));
					}
				} else {
					// If a user is already linked, process input.
					process(buf, p, &user_list);
				}
			}
			// Create a new connection.
			if (FD_ISSET(listenfd, &fdlist)) {
				newconnection();
			}
		}
}
	return 0;
}