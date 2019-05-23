#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>


typedef struct cmd CMD;
int REDIRECTION = 0;

/*
 * Declaration of the Command struct
 */
struct cmd
{
	char **command;
	//points to the next command
	CMD *next;
};

/*
*	Insert a CMD to the linked list
*	Takes a linked list, and the command to add
*	Returns the linked list
*/
CMD* insert(CMD* list, CMD* new_cmd) 
{
	//if we are at the end of the list stick the item at the end
	if (list == NULL) {
		return new_cmd;
	}
	// move to the next item
	list->next = insert(list->next, new_cmd);
	return list;
}

/*
 * Checks for pipes and redirect symbols
 * Takes a command as a string and an array to store each command
 * Returns the number of splits in the string
 */
int check_pipe_red(char *line, char** commandArray) 
{
	int i;
	// make a copy of the line
	char* copy = line;
	// character counter
	int c = 0;
	// pipe array counter
	int p = 0;
	// ignore pipe flag because " or '
	int ignorePipe = 0;
	// single/double quote flag
	int singleQ = 0;
	int doubleQ = 0;
	// go through the string
	for (i = 0; i < strlen(line); i++) {
		// if it is a | or > and aren't within quotes
		if ((copy[c] == '|' || copy[c] == '>') && !ignorePipe) {
			// if > set the redirection flag
			if (copy[c] == '>') {
				REDIRECTION = 1;
			}
			// create space for the string and copy to array
			commandArray[p] = malloc(c + 1); 
			strncpy(commandArray[p], copy, c);
			// NULL terminate the string
			commandArray[p][c] = '\0';
			// move up in the line and reset the char and pipe counter
			copy+=(c+1);
			c = -1;
			p++;
		}
		// if we encounter quotes set/unset flags accordingly
		else if (copy[c] == '\'' && !singleQ) {
			singleQ = 1;
			ignorePipe = 1;
		}
		else if (copy[c] == '\'' && singleQ) {
			singleQ = 0;
			ignorePipe = 0;
		}
		else if (copy[c] == '"' && !doubleQ) {
			doubleQ = 1;
			ignorePipe = 1;
		}
		else if (copy[c] == '"' && doubleQ) {
			doubleQ = 0;
			ignorePipe = 0;
		}
		// inc the character counter
		c++;
	}
	// copy the last command into the array
	commandArray[p] = copy;
	p++;
	// NULL terminate the array
	commandArray[p] = NULL;
	p--;
	// Return the number of pipes
	return p;
}

/* Reads in a line of user input
 * Returns the line as a string
 */
char* read_line() 
{
	// get the input from the user using getline
	char *line = NULL;
	size_t length = 0;
	if ( getline(&line, &length, stdin) == -1) {
		perror("Error reading in line");
		exit(1);
	}
	// remove the newline character
	line[strcspn(line, "\n")] = 0;
	return line;
}

/*
 *	Removes a character from a string
 *	Takes the string to be evaluated and the character to remove
 */
void remove_char(char* cmd, char c)
{	
	// makes two copies of the string
	char* copy1 = cmd;
	char* copy2 = cmd;
	// while there are still chars to evaluate 
	while (*copy1 != 0) {
		// remove the char
		*copy2 = *copy1;
		*copy1++;
		if(*copy2 != c) {
			copy2++;
		}
	}
	*copy2 = '\0';
}

/*
 *	Splits commands by spaces and stores them into a linked list of commands
 *	Takes a linked list for commands, a string array of commands, no. of pipes
 *	Returns the linked list of commands
 */
CMD* split_cmd(CMD* cmdList, char** commandArray, int splits)
{
	// counter to go through all commands
	int i = 0;
	// go through each command
	for (i = 0; i <= splits; i++) {
		// how much space to add to the array of commands when we run out of space
		int arrayIncrement = 100;
		// the size of the array, 100 initially
		int arraySize = 100;
		// create a string array big enough to hold 100 commands
		char** splitString = malloc(arraySize * sizeof(char*));
		// character and splitArray counter
		int c = 0;
		int s = 0;
		// a copy of the array of commands
		char* copy = commandArray[i];
		// flag for if the prev char was a space
		int space = 0;
		// counter for where we are in each command
		int j = 0;
		// flag to ignore spaces i.e. don't remove them
		int ignoreSpace = 0;
		// flags if we are inside single or double quotes
		int singleQ = 0;
		int doubleQ = 0;
		// while there are still characters in the command string we are evaluating
		while (commandArray[i][j] != '\0') {
			// if the char is not a space then unset the space flag
			if (copy[c] != ' ') {
				space = 0;
			}
			// remove all spaces at the start of the command
			if (copy[c] == ' ' && j == 0) {
				while(isspace(*copy)) {
					copy++;
					j++;
				}
			}
			// if we see a space, the last char wasn't a space and we are not to ignore spaces
			if (copy[c] == ' ' && !space && !ignoreSpace) {
				// create space for the command arg
				splitString[s] = malloc(c+1);
				// copy the command arg into the string array
				strncpy(splitString[s], copy, c);
				// check if we have run out of space in the array
				if (s >= arraySize) {
					//inc the arraySize
					arraySize += arrayIncrement;
					// give more space to the array
					splitString = realloc(splitString, arraySize * sizeof(char*));
				}
				// remove any " or ' we have encountered
				remove_char(splitString[s], '\'');
				remove_char(splitString[s], '"');
				// move up past what we have just copied out
				copy+=(c+1);
				// set the characters read counter to -1;
				c = -1;
				// inc the array counter and set the space flag
				s++;
				space = 1;
			}
			// if we see quotes set/unset the appropriate flags
			else if (copy[c] == ' ' && space) {
				copy++;
				c = -1;
			}
			else if (copy[c] == '\'' && !singleQ) {
				singleQ = 1;
				ignoreSpace = 1;
			}
			else if (copy[c] == '\'' && singleQ) {
				singleQ = 0;
				ignoreSpace = 0;
			}
			else if (copy[c] == '"' && !doubleQ) {
				doubleQ = 1;
				ignoreSpace = 1;
			}
			else if (copy[c] == '"' && doubleQ) {
				doubleQ = 0;
				ignoreSpace = 0;
			}
			// inc counters
			c++;
			j++;
		}
		// if we have not ended with a space copy the last arg out to the array
		if (!space) {
			remove_char(copy, '\'');
			remove_char(copy, '"');
			splitString[s] = copy;
			s++;
		}
		// NULL terminate the array
		splitString[s] = NULL;
		
		// make a new CMD item with our splitString
		CMD* cmd = (CMD*) malloc(sizeof(CMD));
		cmd->command = splitString;
		cmd->next = NULL;

		// add the splitString into our list
		cmdList = insert(cmdList, cmd);
	}
	return cmdList;
}

/*
 *	Executes commands containing pipes
 *	Takes a linked list of commands, no. of pipes and a redirection flag
 */
void piped_command(CMD* cmdList, int pipes, int redirectFlag)
{
	// the process id
	pid_t pid;
	// handles for either end of the pipe, 2 for each pipe
	int fd[2*pipes];
	// pipe status
	int status;
	// counter variable
	int counter = 0;
	int pipeCounter = 0;
	int i = 0;
	// create pipes
	for (i = 0; i < pipes; i++) {
		status = pipe(fd + 2*i);
		if (status == -1) {
			printf("Pipe error\n");
			exit(1);
		}
	}	
	// while there are still commands in our list to execute
	while (cmdList != NULL) {
		// fork
		pid = fork();
		// check if there is a fork error
		if (pid == -1) {
			printf("Fork error\n");
			exit(1);
		}
		// check if we are in the child process
		if (pid == 0) {
			// if this is not the final command in our pipe, and there are still pipes left
			if (cmdList->next != NULL && pipeCounter != pipes) {
				// change the stdout to a pipe
				if (dup2(fd[counter + 1], STDOUT_FILENO) == -1) {
					printf("Error changing stdout to a pipe\n");
					exit(1);
				}
			}
			// if output is to be redirected
			if (cmdList->next != NULL && pipeCounter == pipes && redirectFlag) {
				// the file descriptor
				int rfd;
				// create a copy of the list and make it point to the next item
				CMD* copy = cmdList;
				copy = copy->next;
				// open file - write only, clear existing contents, create new file if non-existent
				rfd = open(copy->command[0], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
				if (dup2(rfd, STDOUT_FILENO) == -1) {
					printf("Error changing stdout to a >\n");
				}
				close(rfd);
			} 
			// if this is not the first command in our pipe
			if (counter != 0) {
				// change the stdin to a pipe
				if (dup2(fd[counter - 2], STDIN_FILENO) == -1) {
					printf("Error changing stdin to a pipe, counter is: %d\n", counter);
					exit(1);
				}
			}
			// now we want to close the child pipes
			for (i = 0; i < pipes * 2; i++) {
				close(fd[i]);
			}
			// replace the child process
			if (execvp(cmdList->command[0], cmdList->command) == -1) {
				// error
				printf("Error replacing the child process\n");
				exit(1);
			}
		}
		// PARENT
		// we need to move forward further 1 in our linkedlist if this is >
		if (cmdList->next != NULL && pipeCounter == pipes && redirectFlag) {
			cmdList = cmdList->next;
		}
		// move along to the next command in the list
		cmdList = cmdList->next;
		// inc our counter
		counter+=2;
		pipeCounter++;
	}
	// PARENT cont.
	for (i = 0; i < 2 * pipes; i++) {
		close(fd[i]);
	}
	// wait for the child to finish
	for (i = 0; i < pipes + 1; i++) {
		wait(&status);
	}
}

/*
 *	Execute commands that do not contain any pipes
 *	Takes a linked list of commands
 */
void non_piped_command(CMD* cmdList)
{
	// declare variable for the process id
	pid_t pid;
	// fork
	pid = fork();
	int status = 0;
	// check if there is a fork error
	if (pid == -1) {
		printf("Fork error\n");
		exit(1);
	}
	// if there is no error and pid == 0
	if (pid == 0) {
		// replace the child process
		if (execvp(cmdList->command[0], cmdList->command) == -1) {
			// error
			printf("Error replacing the child process\n");
			exit(1);
		}
	}
	// otherwise we are in the Parent process
	else {
		// wait for the child to finish
		wait(&status);
	}
	return;
}

/*
 *	Redirects output to a file 
 *	Takes a linked list of commands
 */
void redirect(CMD* cmdList)
{	
	// declare variable for the process id
	pid_t pid;
	// declare the file descriptor
	int fd;
	int status = 0;
	// make a copy of the command linked list
	CMD* copy = cmdList;
	copy = copy->next;
	// open the file to output into - write only, delete existing data, create file if dont exist, read rights for owner to true, 
	fd = open(copy->command[0], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
	// fork
	pid = fork();
	// check if there is a fork error
	if (pid == -1) {
		printf("Fork error\n");
		exit(1);
	}
	// if there is no error and pid == 0
	if (pid == 0) {
		// we are in the child process !!
		// stdout is now going to the output file
		dup2(fd, 1);
		// close the fd
		close(fd);
		// replace the child process
		if (execvp(cmdList->command[0], cmdList->command) == -1) {
			// error
			printf("Error calling exec\n");
			exit(1);
		}
	}
	// parent process, wait for child to finish
	else {
		wait(&status);
	}
	return;
}

/*
 *	Has an infinite loop which reads input, parses and executes commands
 */
int main (int argc, char *argv[])
{
	// DECLARE VARIABLES
	// a character array to store the line to be read
	char* lineRead;
	// string array to hold each command
	char* commandArray[10];
	// splits counter: splits made by | or >
	int splits;
	// pipes counter
	int pipes;
	// print welcome message
	printf("WELCOME!\n");
	printf("Type a command to execute or type exit to quit\n");
	
	// loop infinitely
	while(1) {
		// a list to store the commands
		CMD* cmdList = NULL;
		// print the starting character thing
		printf(">> ");
		// call the read_line method to read in a line
		lineRead = read_line();
		// check for the number of splits made by | and >
		splits = check_pipe_red(lineRead, commandArray);
		// get the list of commands as a linked list spaces removed
		cmdList = split_cmd(cmdList, commandArray, splits);
		// if there is redirection set the redirection flag
		if (REDIRECTION) {
			pipes = splits - 1;
		}
		else {
			pipes  = splits;
		}
		// if our command has pipes and maybe redirection call the pipe version of execute command
		if (pipes > 0) {
			piped_command(cmdList, pipes, REDIRECTION);
		}
		// otherwise call the non pipe version of execute command
		else {
			// check if it is a cd
			if (strcmp(cmdList->command[0], "cd") == 0) {	
				chdir(cmdList->command[1]);
			}
			// check if it is exit
			if (strcmp(cmdList->command[0], "exit") == 0) {
				printf("Bye bye\n");
				exit(0);
			}
			// check if it is just a redirect
			else if (REDIRECTION) {	
				redirect(cmdList);
			}
			// otherwise there are no pipes or redirects
			else {
				non_piped_command(cmdList);
			}
		}
		// reset the REDIRECTION flag
		REDIRECTION = 0;
	}
	return 1;
}
