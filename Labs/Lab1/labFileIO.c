#define _XOPEN_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

struct user {
	char username[50];
	char password[256];
	char firstname[50];
	char lastname[50];
	int admin;
};

char* cs336Hash(const char* password) {
	return crypt(password, "00");
}

struct user* createUsers(int* count) {
	// FILE* input_file = fopen("credential_file", "r");
	// char c;

	// if (input_file != NULL) {
	// 	for (c = getc(input_file); c != EOF; c = getc(input_file)) {
	// 		// Increment count if this character is newline 
	// 		if (c == '\n') {
	// 			*count += 1;
	// 		}
	// 	}
	// } else {
	// 	printf("File could not be opened.");
	// 	exit(1);
	// }

	// fclose(input_file);

	// struct user* users = (struct user*) malloc((*count) * sizeof(users));
	// return(users);
	FILE* input_file = fopen("credential_file", "r");
	int num_lines = 0;
	const int size = 100;
	char line[size];

	if (input_file != NULL) {
		while (fgets(line, size, input_file) != NULL) {
			num_lines++;
		}
	} else {
		exit(EXIT_FAILURE);
	}

	fclose(input_file);

	*count = num_lines;

	struct user *users = (struct user*)malloc(num_lines * sizeof(struct user));
	return users;
}

void populateUsers (void* users) {
	struct user* temp = (struct user*) users;
	FILE* input_file = fopen("credential_file", "r");
	
	int i = 0;
	const int size = 100;
	char line[size];

	if (input_file != NULL) {
		while(fgets(line, size, input_file) != NULL) {
			char *token = strtok(line, "\t");
			// Get firstname
			strcpy((temp + i)-> firstname, token);
			// Get lastname
			token = strtok(NULL, "\t");
			strcpy((temp + i)-> lastname, token);
			// Get username
			token = strtok(NULL, "\t");
			strcpy((temp + i)-> username, token);
			// Get password
			token = strtok(NULL, "\t");
			strcpy((temp + i)-> password, token);
			// Get admin var
			token = strtok(NULL, "\t");
			(temp + i)-> admin = *token - '0';

			i++;
		}
	} else {
		printf("File could not be opened.");
		exit(1);
	}

	fclose(input_file);
}

int checkAdminPassword(char* password, struct user* users, int count) {
	for (int i = 0; i < count ; ++ i) {
		if (strcmp ((users + i)->username, "admin") == 0) {
			if (strcmp((users + i)->password, cs336Hash(password)) == 0) {
				return 1;
			} else {
				return 0;
			}
		}
	}
	return 0;
}

struct user* addUser(struct user* users, int* count, char* username, char* password,
	char* firstname, char* lastname, int admin) {
	struct user* temp = (struct user*) users;
	*count += 1;
	
	temp = (struct user*)realloc(users, (*count * sizeof(users)));

	strcpy((temp + *count - 1)->firstname, firstname);
	strcpy((temp+ *count - 1)->lastname, lastname);
	strcpy((temp + *count - 1)->username, username);
	strcpy((temp+ *count - 1)->password, cs336Hash(password));
	(temp + *count - 1)->admin = admin;

	return temp;
}

void saveUsers(struct user* users, int count) {
	FILE* output_file = fopen("credential_file", "w");

	if (output_file != NULL) {
		for (int i = 0; i < count; ++i) {
			fprintf(output_file, "%s\t", (users + i)->firstname);
			fprintf(output_file, "%s\t", (users + i)->lastname);
			fprintf(output_file, "%s\t", (users + i)->username);
			fprintf(output_file, "%s\t", (users + i)->password);
			fprintf(output_file, "%d\n", (users + i)->admin);
		}
	} else {
		printf("Could not write to file.");
		exit(EXIT_FAILURE);
	}

	fclose(output_file);
}

int main(void) {
	int user_count = 0;
	struct user* users = createUsers(&user_count);
	if (users == NULL) {
		return EXIT_FAILURE;
	}
	populateUsers(users);
	printf("Enter admin password to add new users:");
	char entered_admin_password[50];
	scanf("%s", entered_admin_password);
	
	if (checkAdminPassword(entered_admin_password, users, user_count)) {
		struct user new_user;
		printf("Enter username :");
		scanf("%s", new_user.username);
		printf("Enter first name :");
		scanf("%s", new_user.firstname);
		printf("Enter last name :");
		scanf("%s", new_user.lastname);
		printf("Enter password :");
		scanf("%s", new_user.password);
		printf("Enter admin level :");
		scanf("%d", &(new_user.admin));
		users = addUser(users, &user_count, new_user.username, new_user.password,
			new_user.firstname, new_user.lastname, new_user.admin);
		
		if (users == NULL) {
			return EXIT_FAILURE ;
		}
	}
	
	saveUsers(users, user_count);
	free(users);
	return EXIT_SUCCESS ;
}