#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#define CMD_BG "bg"
#define CMD_BGLIST "bglist"
#define CMD_BGKILL "bgkill"
#define CMD_BGSTOP "bgstop"
#define CMD_BGSTART "bgstart"
#define CMD_PSTAT "pstat"
struct cmd_type {
	char *type;
	char *target_file;
	pid_t target_pid;
	char *args[10];
};
struct pnode {
	struct pnode* next;
	char * name;
	pid_t pid;
};
void check_zombieProcess(void);
const char * get_name_from_pid(pid_t pid);
struct cmd_type parse_cmd(char cmd[]);
struct cmd_type bg_entry(struct cmd_type);
struct pnode * add_node(struct pnode *, char *name, pid_t pid);
void bglist_entry(struct pnode *head);
void bgkill(pid_t pid);
struct pnode * delete_head();
void bgstop(pid_t pid);
void bgstart(pid_t pid);
void pstat(pid_t pid);
// main control loop
struct pnode *head = NULL;
int main(){
	struct cmd_type cmd_struct;
	while(1){	
		printf("\nPMan: >");
		char cmd[30];
		fgets(cmd, 30, stdin); //read line from user
		cmd[strcspn(cmd, "\n")] = 0; // remove newline from input
		cmd_struct = parse_cmd(cmd);	//store information about command in a struct
		if(strlen(cmd) == 0) { //if user just hits enter without typing go back to the top
			continue;
		}
		if (strcmp(cmd_struct.type,CMD_BG)== 0 ){
			cmd_struct = bg_entry(cmd_struct);
		}
		else if(strcmp(cmd_struct.type, CMD_BGLIST) == 0){
			bglist_entry(head);
		}
		else if(strcmp(cmd_struct.type, CMD_BGKILL) == 0){
			if(cmd_struct.target_file != NULL) {
				bgkill(atoi(cmd_struct.target_file));
			}
		}
		else if (strcmp(cmd_struct.type, CMD_BGSTOP) == 0) {
			if(cmd_struct.target_file != NULL) {
				bgstop(atoi(cmd_struct.target_file));
			}
		}
		else if (strcmp(cmd_struct.type, CMD_BGSTART) == 0 ) {
			if(cmd_struct.target_file != NULL) {
				bgstart(atoi(cmd_struct.target_file));
			}
		}
		else if(strcmp(cmd_struct.type,CMD_PSTAT) == 0 ){
			if(cmd_struct.target_file != NULL) {
				pstat(atoi(cmd_struct.target_file));
			}
		}
	//	else {
			//usage_pman();
	//	}
	//
		else {
			printf("\nPMan: > %s: command not found", cmd_struct.type);
		}
		check_zombieProcess();
	}
	return 0;
}
void bglist_entry(struct pnode *head) {
	struct pnode *ptr = head;
	char *name;
	int bgjobs = 0;
	while(ptr != NULL) {
	//	char buf[PATH_MAX];
		name = get_name_from_pid(ptr->pid);
	//	if(name[0] == '.' || name[0] == '~' || name[0] == '/') {
	//		name = realpath(name, buf);
	//		if(name) {
	//			printf("%d: %s\n", ptr->pid, buf);
	//		}
	//		else {
	//			perror("realpath");
	//			exit(EXIT_FAILURE);
	//		}
	//	}
	//	else {
			printf("%d: %s\n", ptr->pid, name);
	//	}
		ptr = ptr->next;
		bgjobs++;
	}
	printf("total background jobs: %d\n", bgjobs);
}
struct pnode * add_node(struct pnode *head, char name[], pid_t pid) {
	struct pnode *t = (struct pnode*) malloc(sizeof(struct pnode));
	t->name = name;
	t->pid = pid;
	t->next = head;
	return t;
}

void remove_node(struct pnode *head, pid_t pid) {
	struct pnode* cur = head;
	struct pnode* prev = NULL;

	if(head==NULL) {
		return;
	}

	while(cur->pid != pid) {
		if(cur->next == NULL) {
			return;
		}
		else {
			prev = cur;
			cur = cur->next;
		}
	}
	if(cur == head) {
		head = delete_head();
	}
	else {
		prev->next = cur->next;
	}
}

struct pnode * delete_head() {
	struct pnode *t = head;
	head = head->next;
	return t;
}

void bgkill(pid_t pid) {
	struct stat sts;
  	char fname[100];	
	sprintf(fname, "/proc/%d/stat", pid);
	if(stat(fname, &sts) == -1) {
		printf("no such process\n");
		return;
	}
	kill(pid, 15); //term signal
	remove_node(head, pid); //remove from linked list
	return;
}

void bgstop(pid_t pid) {
	//send stop signal
	
	struct stat sts;
  	char fname[100];	
	sprintf(fname, "/proc/%d/stat", pid);
	if(stat(fname, &sts) == -1) {

		printf("no such process\n");
		return;
	}
	kill(pid, 19);
}

void bgstart(pid_t pid) {
	// send cont signal
	
	struct stat sts;
  	char fname[100];	
	sprintf(fname, "/proc/%d/stat", pid);
	if(stat(fname, &sts) == -1) {
		printf("no such process\n");
		return;
	}
	kill(pid, 18);
}

struct cmd_type parse_cmd(char *cmd) {
	char delim[2] = " ";
	char *token;
	token = strtok(cmd, delim);
	struct cmd_type retval;
	int i = 1;
	char dotslash[30] = "./";
	while(token != NULL) {
		if(i == 1) { //store command type
			retval.type = token;
			i++;
		}
		else if(i == 2) { // store file name, store ./filename in args[0] for execvp to use later
			retval.target_file = token;
			strcat(dotslash, token);
			retval.args[0] = dotslash;
			i++;
		}
		else { //store arguments for execvp to use later
			retval.args[i -2] = token;
			i++;
		}
		token = strtok(NULL, delim); //step forward with strtok
	}
	return retval;
}
struct cmd_type bg_entry(struct cmd_type cmd){
	pid_t pid;
	pid = fork();	
	if(pid == 0){
		execvp(cmd.target_file, cmd.args);
		cmd.args[0] = cmd.target_file;
		if(execvp(cmd.target_file, cmd.args) < 0){
			perror("Error on execvp");
		}
	}
	else if(pid > 0) {
		// store information of the background child process in your data structures
		char name[30];
		strcpy(name, cmd.args[0]);
		cmd.target_pid = pid;
		head = add_node(head, name, pid);
	}
	else {
		perror("fork failed");
		exit(EXIT_FAILURE);
	}
	return cmd;
}

const char * get_name_from_pid(pid_t pid) {
	char fname[100];
	char name[100];
	sprintf(fname, "/proc/%d/stat", pid);
	FILE *f = fopen(fname, "r");
	int dontcare;
	fscanf(f, "%d %s", &dontcare, name);
	char *pname = name;
	pname++;
	pname[strcspn(pname, ")")] = 0;
	return pname;
}

void pstat(pid_t pid) {
	char fname[100];
	sprintf(fname, "/proc/%d/stat", pid);
	FILE *f = fopen(fname, "r");
	if(f) {
		char pname[100];
		char state;
		pid_t ppid_idc;//dont care what this is, just for fscanf formatting
		int pid_idc;// same as above
		pid_t group_pid_idc; //same as above
		pid_t session_pid_idc; //same as above
		int cterminal_idc; //same as above
		unsigned fpg_idc; // same as above
		unsigned long kernel_flags; //same as above
		unsigned long minor_faults; //same as above
		unsigned long utime; //DO CARE ABOUT THIS, time scheduled in user mode
		signed long stime; //DO CARE ABOUT THIS, time scheduled in kernel mode
		signed long dummy;
		unsigned long long dummy2;
		unsigned long rss;
		//honestly i have no idea what the right syntax is to get things out of /proc, theres very few resources online
		//and this is what I was able to find. the dummy values are grabbing things that we dont need, i have no idea
		//how to grab just comm, state, utime, stime, rss, and the ctxt switches on their own
		fscanf(f, "%d %s %c %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld %llu %lu %ld %lu", &pid_idc, pname, &state, &ppid_idc,&group_pid_idc,
			&session_pid_idc, &cterminal_idc, &fpg_idc, &kernel_flags, &minor_faults, &minor_faults,
	        	&minor_faults, &minor_faults, &utime, &stime, &dummy, &dummy, &dummy, &dummy, &dummy, &dummy2, &dummy, &rss, &rss
			 );
		printf("comm: %s\n", pname);
		printf("state: %c\n", state);
		printf("utime: %lu\n", utime);
		printf("stime: %lu\n", stime);
		printf("rss: %ld\n", rss);
		fclose(f);
		char fname2[100];
		sprintf(fname2, "/proc/%d/status", pid);
		FILE *f2 = fopen(fname2, "r");
		char buf[100];
		int linecount = -2;
		while(fgets(buf, 100, f2) != NULL) {
			if(linecount == 54) {
				printf("%s\n", buf);
				linecount++;
			}
			else if (linecount == 55) {
				printf("%s\n", buf);
				linecount++;
			}
			else {
				linecount++;
			}
		}
		fclose(f2);
	}
	else {
		printf("Error: Process %d doesn't exit", pid);
	}
}


void check_zombieProcess(void){
	int status;
	int retVal = 0;
	struct pnode *headPnode = head; // so it compiles	
	while(headPnode != NULL) {
		usleep(1000);
		retVal = waitpid(-1, &status, WNOHANG);
		if(retVal > 0) {
			//remove the background process from your data structure
			printf("pid: %d terminated", headPnode->pid);
			remove_node(head, headPnode->pid);
		}
		else if(retVal == 0){
			break;
		}
		else{
			perror("waitpid failed");
			exit(EXIT_FAILURE);
		}
		headPnode = headPnode->next;
	}
	return ;
}
