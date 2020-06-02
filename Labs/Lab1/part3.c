#define _XOPEN_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef unsigned char bitmap8;

struct user {
	char username [50];
	char password [50];
	bitmap8 permissions;
};

struct user *createUsers (int max_number_of_users) {
	struct user *users = (struct user*) malloc(max_number_of_users * sizeof(users));
	return users;
}
void initUsers (void* users, int max_number_of_users) {
	struct user* users_copy = (struct user*) users;

	for (int i = 0; i < max_number_of_users; i++) {
		strcpy((users_copy + i)->username, "default");
		strcpy((users_copy + i)->password, "default");
		(users_copy + i)->permissions = 0;
	}
}

void addUser1 (struct user* users, char* username, char* password,
	int admin, int* count) {
	strcpy((users + *count)->username, username);
	strcpy((users + *count)->password, password);
	(users + *count)->permissions = admin;

	*count += 1;
}

void addUser2 (struct user* users, struct user* newUser, int* count) {
	strcpy((users + *count)->username, (newUser->username));
	strcpy((users + *count)->password, (newUser->password));
	(users + *count)->permissions = newUser->permissions;

	*count += 1;
}

void printUser (struct user* users, int max_number_of_users) {
	for (int i = 0; i < max_number_of_users; i++) {
		printf("\nusername%d: %s\n", i, (users + i)->username);
		printf("\npassword%d: %s\n", i, (users + i)->password);
	}
}

void authorizePermission (int bitIndex, struct user* user) {
    // Sets bit to 1
	user->permissions |= 1 << bitIndex;
}

void revokePermission (int bitIndex, struct user* user) {
	// Clears bit to 0
	user->permissions &= ~(1 << bitIndex);
}

int hasPermission (int bitIndex, struct user* user) {
	// Return true if bit == 1, return false if bit == 0 
	return (user->permissions & (1 << bitIndex));
}

void setPermissions (int new_permissions, struct user *user) {
	switch(new_permissions) {
    	// Get rid of all permissions
		case 0: 
			for (int i = 0; i < 3; ++i) {
				revokePermission(i, user);
			}
			break;
        // Set execute permission
		case 1: 
			for (int i = 0; i < 3; ++i) {
				revokePermission(i, user);
			}
			authorizePermission(2, user);
			break;
        // Set write permission
		case 2: 
			for (int i = 0; i < 3; ++i) {
				revokePermission(i, user);
			}
			authorizePermission(0, user);
		break;
        // Set write & execute permission 
		case 3: 
			for (int i = 0; i < 3; ++i) {
				revokePermission(i, user);
			}
			authorizePermission(2, user);
			authorizePermission(0, user);
			break;
        // Set read permission 
		case 4:
			for (int i = 0; i < 3; ++i) {
				revokePermission(i, user);
			}
			authorizePermission(1, user);
			break;
        // Set read & execute permission 
		case 5: 
			for (int i = 0; i < 3; ++i) {
				revokePermission(i, user);
			}
			authorizePermission(1, user);
			authorizePermission(2, user);
			break;
        // Set read & write permission 
		case 6: 
			for (int i = 0; i < 3; ++i) {
				revokePermission(i, user);
			}
			authorizePermission(1, user);
			authorizePermission(0, user);
			break;
        // Set all permissions
		case 7: 
			authorizePermission(0, user);
			authorizePermission(1, user);
			authorizePermission(2, user);
			break;
		default:
			printf("%s\n", "Enter an integer between 0 - 7: ");
			break;
	}
}

void printPermissions (struct user* user) {
	if (hasPermission (0, user)) {
		printf ("%s has write permission.\n", user->username);
	} else {
		printf ("%s doesn't have write permission.\n", user->username);
	}

	if (hasPermission (1, user)) {
		printf ("%s has read permission.\n", user->username);
	} else {
		printf ("%s doesn't have read permission.\n", user->username);
	}
	if ( hasPermission (2 , user )) {
		printf ("%s has execute permission.\n", user->username);
	}
	else {
		printf ("%s doesn't have execute permission.\n", user->username);
	}
}

int main (void) {
	struct user user;
    strcpy (user.username, "admin");
    strcpy (user.password, "s#1Pa5");
    user.permissions = 0; // Sets the permissions to 0
    authorizePermission (0, &user);
    authorizePermission (1 , &user);
    printPermissions (&user);
    revokePermission (1 , &user);
    authorizePermission (2 , &user);
    printPermissions (&user);

    printf("\n%s\n", "Setting permission to 0");
    setPermissions(0, &user);
    printPermissions (&user);

    printf("\n%s\n", "Setting permission to 1");
    setPermissions(1, &user);
    printPermissions (&user);

    printf("\n%s\n", "Setting permission to 4");
    setPermissions(4, &user);
    printPermissions (&user);

    printf("\n%s\n", "Setting permission to 7");
    setPermissions(7, &user);
    printPermissions (&user);

	return 0;
}