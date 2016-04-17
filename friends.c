#include "friends.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


/*
 * Create a new user with the given name.  Insert it at the tail of the list 
 * of users whose head is pointed to by *user_ptr_add.
 *
 * Return:
 *   - 0 on success.
 *   - 1 if a user by this name already exists in this list.
 *   - 2 if the given name cannot fit in the 'name' array
 *       (don't forget about the null terminator).
 */
int create_user(const char *name, User **user_ptr_add) {
    if (strlen(name) >= MAX_NAME) {
        return 2;
    }

    User *new_user = malloc(sizeof(User));
    if (new_user == NULL) {
        perror("malloc");
        exit(1);
    }
    strncpy(new_user->name, name, MAX_NAME); // name has max length MAX_NAME-1

    for (int i = 0; i < MAX_NAME; i++) {
        new_user->profile_pic[i] = '\0';
    }

    new_user->first_post = NULL;
    new_user->next = NULL;
    for (int i = 0; i < MAX_FRIENDS; i++) {
        new_user->friends[i] = NULL;
    }

    // Add user to list
    User *prev = NULL;
    User *curr = *user_ptr_add;
    while (curr != NULL && strcmp(curr->name, name) != 0) {
        prev = curr;
        curr = curr->next;
    }

    if (*user_ptr_add == NULL) {
        *user_ptr_add = new_user;
        return 0;
    } else if (curr != NULL) {
        free(new_user);
        return 1;
    } else {
        prev->next = new_user;
        return 0;
    }
}


/* 
 * Return a pointer to the user with this name in
 * the list starting with head. Return NULL if no such user exists.
 *
 * NOTE: You'll likely need to cast a (const User *) to a (User *)
 * to satisfy the prototype without warnings.
 */
User *find_user(const char *name, const User *head) {
/*    const User *curr = head;
    while (curr != NULL && strcmp(name, curr->name) != 0) {
        curr = curr->next;
    }

    return (User *)curr;
*/
    while (head != NULL && strcmp(name, head->name) != 0) {
        head = head->next;
    }

    return (User *)head;
}


/*
 * Print the usernames of all users in the list starting at curr.
 * Names should be printed to standard output, one per line.
 */
char *list_users(const User *curr) {
	char *user_string = malloc((num_friends(curr) * MAX_NAME) + 12);
	if (user_string == NULL) {
        perror("malloc");
        exit(1);
    }
	user_string[0] = 0;
    strcat(user_string, "User List\n");
    while (curr != NULL) {
        strcat(user_string, "\t");
		strcat(user_string, curr->name);
		strcat(user_string, "\n");
        curr = curr->next;
    }
	return user_string;
}

int num_friends(const User *curr) {
	int counter = 0;
	while (curr != NULL) {
        counter++;
        curr = curr->next;
    }
	return counter;
}

int post_bytes(const Post *curr) {
	int counter = 0;
	while (curr != NULL) {
		counter += 7 + (int) strlen(curr->contents) + (int) strlen(curr->
author) + 35 + 2 + 1 + 1;
        curr = curr->next;
        if (curr != NULL) {
            counter += 7;
        }
    }
	return counter;
}

/* 
 * Make two users friends with each other.  This is symmetric - a pointer to 
 * each user must be stored in the 'friends' array of the other.
 *
 * New friends must be added in the first empty spot in the 'friends' array.
 *
 * Return:
 *   - 0 on success.
 *   - 1 if the two users are already friends.
 *   - 2 if the users are not already friends, but at least one already has
 *     MAX_FRIENDS friends.
 *   - 3 if the same user is passed in twice.
 *   - 4 if at least one user does not exist.
 *
 * Do not modify either user if the result is a failure.
 * NOTE: If multiple errors apply, return the *largest* error code that 
applies.
 */
int make_friends(const char *name1, const char *name2, User *head) {
    User *user1 = find_user(name1, head);
    User *user2 = find_user(name2, head);
	// Only change here is to compare if the users are the same person, then a 
	// special error message is printed.
	if (user1 == user2) {
		return 5;
	}
    if (user1 == NULL || user2 == NULL) {
        return 4;
    } else if (user1 == user2) { // Same user
        return 3;
    }

    int i, j;
    for (i = 0; i < MAX_FRIENDS; i++) {
        if (user1->friends[i] == NULL) { // Empty spot
            break;
        } else if (user1->friends[i] == user2) { // Already friends.
            return 1;
        }
    }

    for (j = 0; j < MAX_FRIENDS; j++) {
        if (user2->friends[j] == NULL) { // Empty spot
            break;
        } 
    }

    if (i == MAX_FRIENDS || j == MAX_FRIENDS) { // Too many friends.
        return 2;
    }

    user1->friends[i] = user2;
    user2->friends[j] = user1;
    return 0;
}




/*
 *  Print a post.
 *  Use localtime to print the time and date.
 */
char *print_post(const Post *post) {
    if (post == NULL) {
        return "\0";
    }
	char *time = asctime(localtime(post->date));
	char *post_contents = malloc(strlen(post->contents) + strlen(post->author)
 + strlen(time) + 16);
	if (post_contents == NULL) {
        perror("malloc");
        exit(1);
    }
	post_contents[0] = 0;
    // Print author
    strcat(post_contents, "From: ");
	strcat(post_contents, post->author);
	strcat(post_contents, "\n");
    
    // Print date
	strcat(post_contents, "Date: ");
	strcat(post_contents, time);
	strcat(post_contents, "\n");

    // Print message
    strcat(post_contents, post->contents);
	strcat(post_contents, "\n");

    return post_contents;
}


/* 
 * Print a user profile.
 * For an example of the required output format, see the example output
 * linked from the handout.
 * Return:
 *   - 0 on success.
 *   - 1 if the user is NULL.
 */
char *print_user(const User *user) {
    if (user == NULL) {
        return "\0";
    }
	int friend_bytes = (MAX_FRIENDS * MAX_NAME) + 1;
	int spacer_bytes = (3 * 43) + 1;
	int name_header_bytes = 8 + strlen(user->name) + 1;
	int friends_header_bytes = 9 + 1;
	int posts_header_bytes = 7 + 1;
	int posts_bytes = post_bytes(user->first_post);
	int total_bytes = friend_bytes + spacer_bytes + name_header_bytes + 
friends_header_bytes + posts_header_bytes + posts_bytes + 1;
	char *total_string = malloc(total_bytes);
    // Print name
	total_string[0] = 0;
	strcat(total_string, "Name: ");
	strcat(total_string, user->name);
	strcat(total_string, "\n\n");
	strcat(total_string, "------------------------------------------\n");

    // Print friend list.
	strcat(total_string, "Friends:\n");
    for (int i = 0; i < MAX_FRIENDS && user->friends[i] != NULL; i++) {
		strcat(total_string, user->friends[i]->name);
		strcat(total_string, "\n");
    }
	strcat(total_string, "------------------------------------------\n");

    // Print post list.
	strcat(total_string, "Posts:\n");
    const Post *curr = user->first_post;
    while (curr != NULL) {
        char *buffer = print_post(curr);
		strcat(total_string, buffer);
		free(buffer);
        curr = curr->next;
        if (curr != NULL) {
			strcat(total_string, "\n===\n\n");
        }
    }
	strcat(total_string, "------------------------------------------\n");

    return total_string;
}


/*
 * Make a new post from 'author' to the 'target' user,
 * containing the given contents, IF the users are friends.
 *
 * Insert the new post at the *front* of the user's list of posts.
 *
 * Use the 'time' function to store the current time.
 *
 * 'contents' is a pointer to heap-allocated memory - you do not need
 * to allocate more memory to store the contents of the post.
 *
 * Return:
 *   - 0 on success
 *   - 1 if users exist but are not friends
 *   - 2 if either User pointer is NULL
 */
int make_post(const User *author, User *target, char *contents) {
    if (target == NULL || author == NULL) {
        return 2;
    }

    int friends = 0;
    for (int i = 0; i < MAX_FRIENDS && target->friends[i] != NULL; i++) {
        if (strcmp(target->friends[i]->name, author->name) == 0) {
            friends = 1;
            break;
        }
    }

    if (friends == 0) {
        return 1;
    }

    // Create post
    Post *new_post = malloc(sizeof(Post));
    if (new_post == NULL) {
        perror("malloc");
        exit(1);
    }
    strncpy(new_post->author, author->name, MAX_NAME);
    new_post->contents = contents;
    new_post->date = malloc(sizeof(time_t));
    if (new_post->date == NULL) {
        perror("malloc");
        exit(1);
    }
    time(new_post->date);
    new_post->next = target->first_post;
    target->first_post = new_post;

    return 0;
}

