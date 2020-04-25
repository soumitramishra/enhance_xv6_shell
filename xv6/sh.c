// Shell.

#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "stat.h"
// Parsed command representation
#define EXEC  1
#define REDIR 2
#define PIPE  3
#define LIST  4
#define BACK  5
#define AND 6
#define OR 7

#define MAXARGS 10
#define MAX_NO_OF_ALIASES 20
#define MAX_LEN_OF_ALIASES 100
char alias_name[MAX_NO_OF_ALIASES][MAX_LEN_OF_ALIASES];
char cmd_name[MAX_NO_OF_ALIASES][MAX_LEN_OF_ALIASES];
int current_index = 0;
int script_check = 0;

struct cmd {
  int type;
};

struct execcmd {
  int type;
  char *argv[MAXARGS];
  char *eargv[MAXARGS];
};

struct redircmd {
  int type;
  struct cmd *cmd;
  char *file;
  char *efile;
  int mode;
  int fd;
};

struct pipecmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct listcmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct backcmd {
  int type;
  struct cmd *cmd;
};

int fork1(void);  // Fork but panics on failure.
void panic(char*);
struct cmd *parsecmd(char*);

int script(char*);

// Execute cmd.  Never returns.
void
runcmd(struct cmd *cmd)
{
  int final_status;
  // temporary variables to store the status
  int status1, status2;
  int p[2];
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    exit1(0);

  switch(cmd->type){
  default:
    panic("runcmd");

  case EXEC:
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit();
    exec(ecmd->argv[0], ecmd->argv);
    printf(2, "exec %s failed\n", ecmd->argv[0]);
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    close(rcmd->fd);
    if(open(rcmd->file, rcmd->mode) < 0){
      printf(2, "open %s failed\n", rcmd->file);
      exit();
    }
    runcmd(rcmd->cmd);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    if(fork1() == 0)
      runcmd(lcmd->left);
    wait();
    runcmd(lcmd->right);
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    if(pipe(p) < 0)
      panic("pipe");
    if(fork1() == 0){
      close(1);
      dup(p[1]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->left);
    }
    if(fork1() == 0){
      close(0);
      dup(p[0]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->right);
    }
    close(p[0]);
    close(p[1]);
    wait1(&status1);
    wait1(&status2);
    final_status = status1 | status2;
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    if(fork1() == 0)
      runcmd(bcmd->cmd);
    break;

  case AND:
    pcmd = (struct pipecmd*)cmd;
    if(pipe(p)<0)
      panic("pipe");
    if(fork1() == 0)
    	runcmd(pcmd->left);
    wait1(&final_status);
    if(final_status==0)
      runcmd(pcmd->right);
    break;
  
  case OR:
    pcmd = (struct pipecmd*)cmd;
    if(pipe(p)<0)
      panic("pipe");
    if(fork1()==0)
      runcmd(pcmd->left);
    wait1(&final_status);
    if(final_status!=0)
      runcmd(pcmd->right);
    break;
  }

 if(final_status == 0)
   exit();
 else
   exit1(1);
}

int
getcmd(char *buf, int nbuf)
{
  if(script_check == 0)
    printf(2, "$ ");
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

int
linecount(char *file)
{
  int count=0;
  while(*file != '\0'){
	  if(*file == '\n')
	  	count++;
	  file++;
  }
  return count;
}

int
cmdline(char *line,char file[])
{
  int end=0,i=0;
  while(file[i] != '\n')
  {
  	*line = file[i];
	i++;
	line++;
	end++;
  }
  return end;
}

int
script(char *file)
{
  struct stat st;
  if(stat(file, &st) != 0) {
      return 0;
  }
  int i, count, endline=0, end;
  int fd = open(file,O_RDWR);
  if(fd >= 0){
  	char cmd[st.size];
  	int st_lines = 0;
  	read(fd,cmd,st.size);
  	count = linecount(cmd);
  	for(i=0; i<count; i++){
  		char lines[50];
  		end = cmdline(lines,cmd+endline) + 1;
  		endline += end;
  		lines[end-1] = '\0';
  		if(fork1()==0)
  			runcmd(parsecmd(lines)); 
  		wait1(&st_lines);
  	}
  	exit1(st_lines);
  }
  else{
    printf(2,"File does not exist fd: %d buf: %s\n",fd,file); 
    return 1;
  }
  return 0;
}
void 
print_all_aliases(){
	printf(1, "Total aliases: %d\n", current_index);
	for(int i = 0; i< current_index; i++) {
		printf(1, "alias %s = \'%s\'\n", alias_name[i], cmd_name[i]);
	}
}

int
find_alias_by_name(char *alias){
	for(int i = 0; i< current_index; i++) {
		if (strcmp(alias_name[i], alias)==0)
			return i;
	}
	return -1;
}

// Function to add, update or print alias
void
handle_alias(char *buf) {
	if(buf[5] == '\n') {
		      print_all_aliases();
	      }
		else if (buf[5] == ' ') {
		if ((buf[6] == '-' && buf[7] == 'h') ||  (buf[6] == '-' && buf[7] == '-'  && buf[8] == 'h' && buf[9] == 'e' && buf[10]=='l' && buf[11] == 'p') ) {
			printf(1, "Usage:\nalias - to print all aliases\nalias alias_name - to print particular alias\nalias alias_name = cmd_name- to add/update alias\n");
			return;
		}
		int index_to_update = current_index;
	      	buf[strlen(buf) -1] = 0;
	      	char* alias_value = buf + 6;
	      	int i = 0;

	      	while(*alias_value!='='){
		      if(*alias_value == 0) {
			      int idx = find_alias_by_name(alias_name[current_index]);
		      	      if(idx==-1) {
				      printf(2, "alias: %s: not found\n", alias_name[current_index]);
			      }
		      	      else {
				      printf(1, "alias %s = \'%s\'\n", alias_name[idx], cmd_name[idx]);
			      }
			      break;
		      }
		      if (*alias_value != ' ') {
			      alias_name[current_index][i++] = *alias_value;
		      }
		      alias_value += 1;
		}
	      	alias_name[current_index][i] = 0;
		
		int idx = find_alias_by_name(alias_name[current_index]);
		if(idx>=0){
			index_to_update = idx;
		}
	      	if(*alias_value == 0)
		      return;
	        alias_value += 1;
		while(*alias_value== ' ') {
			alias_value += 1;
		}

	      	i = 0;
	      	while(*alias_value!=0) {
		    cmd_name[index_to_update][i++] = *alias_value;  
		    alias_value += 1;
		}
	      	cmd_name[index_to_update][i] = 0;
		if(index_to_update == current_index ) {
			if(current_index>=MAX_NO_OF_ALIASES) {
				printf(2, "Cannot store more than %d aliases", MAX_NO_OF_ALIASES);
				return;
			}
	      		printf(1,"alias::Added alias %s\n", alias_name[index_to_update]);
	        	current_index++;	
		}
		else {

	      		printf(1,"alias::Updated alias %s\n", alias_name[index_to_update]);
		}
	      }
}

int
main(int argc, char *argv[])
{
  static char buf[100];
  int fd;

  if(argc == 2){
    script_check =1;
    script(argv[1]);
  }
  else {
    // Ensure that three file descriptors are open.
    while((fd = open("console", O_RDWR)) >= 0) {
      if(fd >= 3){
        close(fd);
        break;
      }
    }
    // Read and run input commands.
    while(getcmd(buf, sizeof(buf)) >= 0) {
      // If first 5 characters in input buffer makes word alias, execute handle_alias function
      if(buf[0] == 'a' && buf[1] == 'l' && buf[2] == 'i' && buf[3] == 'a' && buf[4] == 's') {
	     handle_alias(buf); 
      }
      else {
      	if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
        	// Chdir must be called by the parent, not the child.
        	buf[strlen(buf)-1] = 0;  // chop \n
        	if(chdir(buf+3) < 0)
          		printf(2, "cannot cd %s\n", buf+3);
        	continue;
      	}
      	if(fork1() == 0){
		char orig_buf[MAX_LEN_OF_ALIASES*2];
		strcpy(orig_buf,buf);
		orig_buf[strlen(buf)-1] = 0;
		// if the given command is an alias is an alias,
		// run command associated with this alias
		int idx = find_alias_by_name(orig_buf);
		if(idx>=0) {
				char cmd[MAX_NO_OF_ALIASES];
				strcpy(cmd, cmd_name[idx]);
				int n = strlen(cmd);
				cmd[n] = '\n';
				cmd[n+1] = 0;
				printf(1,", Running command %s", cmd);
				runcmd(parsecmd(cmd));
			}
		else
        		runcmd(parsecmd(buf));
	}
      	wait();
    	}
    }
    exit();
  }
}

void
panic(char *s)
{
  printf(2, "%s\n", s);
  exit();
}

int
fork1(void)
{
  int pid;

  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}

//PAGEBREAK!
// Constructors

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = EXEC;
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, char *efile, int mode, int fd)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = REDIR;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->efile = efile;
  cmd->mode = mode;
  cmd->fd = fd;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = PIPE;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
listcmd(struct cmd *left, struct cmd *right)
{
  struct listcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = LIST;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
backcmd(struct cmd *subcmd)
{
  struct backcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = BACK;
  cmd->cmd = subcmd;
  return (struct cmd*)cmd;
}

struct cmd*
andcmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = AND;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

struct cmd*
orcmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = OR;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}
//PAGEBREAK!
// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '(':
  case ')':
  case ';':
  case '&':
  case '<':
    s++;
    break;
  case '>':
    s++;
    if(*s == '>'){
      ret = '+';
      s++;
    }
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;

  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;

  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseand(char**, char*);
struct cmd *parseexec(char**, char*);
struct cmd *nulterminate(struct cmd*);

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    printf(2, "leftovers:%s\n", s);
    panic("syntax");
  }
  nulterminate(cmd);
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;
   
  cmd = parsepipe(ps,es);
        
  	while(peek(ps, es, "&")){
    		gettoken(ps, es, 0, 0);
    		cmd = backcmd(cmd);
 	}
  
  if(peek(ps, es, ";")){
    gettoken(ps, es, 0, 0);
    cmd = listcmd(cmd, parseline(ps, es));
  }
  
  return cmd;
}

struct cmd*
parseand(char **ps, char *es)
{
  struct cmd *cmd;
  int andcount=0;
  cmd = parseexec(ps, es);
  while(peek(ps, es, "&")){ 
    int tok = gettoken(ps, es, 0, 0);
    if((char)tok == '&')
	    andcount++;
    if(andcount == 2){
    	cmd = andcmd(cmd, parseand(ps, es));
    }
   }
  
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd; 
  int orcount=0;
  cmd = parseand(ps, es);
  if(peek(ps, es, "|")){


    gettoken(ps, es, 0, 0);
    orcount++;

    if(*(*ps) == '|'){ 
	orcount++;
	(*ps)++;
    }
     
    if(orcount == 1){
    	cmd = pipecmd(cmd, parsepipe(ps, es));
    }
    else{
    	cmd = orcmd(cmd, parsepipe(ps, es));
    }
  } 

  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a')
      panic("missing file for redirection");
    switch(tok){
    case '<':
      cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
      break;
    case '>':
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
      break;
    case '+':  // >>
      cmd = redircmd(cmd, q, eq, O_WRONLY|O_CREATE, 1);
      break;
    }
  }
  return cmd;
}

struct cmd*
parseblock(char **ps, char *es)
{
  struct cmd *cmd;

  if(!peek(ps, es, "("))
    panic("parseblock");
  gettoken(ps, es, 0, 0);
  cmd = parseline(ps, es);
  if(!peek(ps, es, ")"))
    panic("syntax - missing )");
  gettoken(ps, es, 0, 0);
  cmd = parseredirs(cmd, ps, es);
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;

  if(peek(ps, es, "("))
    return parseblock(ps, es);

  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|)&;")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a')
      panic("syntax");
    cmd->argv[argc] = q;
    cmd->eargv[argc] = eq;
    argc++;
    if(argc >= MAXARGS)
      panic("too many args");
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  cmd->eargv[argc] = 0;
  return ret;
}

// NUL-terminate all the counted strings.
struct cmd*
nulterminate(struct cmd *cmd)
{
  int i;
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    return 0;

  switch(cmd->type){
  case EXEC:
    ecmd = (struct execcmd*)cmd;
    for(i=0; ecmd->argv[i]; i++)
      *ecmd->eargv[i] = 0;
    break;

  case REDIR:
    rcmd = (struct redircmd*)cmd;
    nulterminate(rcmd->cmd);
    *rcmd->efile = 0;
    break;

  case PIPE:
    pcmd = (struct pipecmd*)cmd;
    nulterminate(pcmd->left);
    nulterminate(pcmd->right);
    break;

  case LIST:
    lcmd = (struct listcmd*)cmd;
    nulterminate(lcmd->left);
    nulterminate(lcmd->right);
    break;

  case BACK:
    bcmd = (struct backcmd*)cmd;
    nulterminate(bcmd->cmd);
    break;

  case AND:
    pcmd = (struct pipecmd*)cmd;
    nulterminate(pcmd->left);
    nulterminate(pcmd->right);
    break;

  case OR:
    pcmd = (struct pipecmd*)cmd;
    nulterminate(pcmd->left);
    nulterminate(pcmd->right);
    break;
  }
  return cmd;
}
