#include <stdio.h>  
#include <stdlib.h> 
#include <signal.h>
#include <pthread.h>

#include "idl_export.h" 
#include "readline/readline.h"
#include "readline/history.h"

#include "ridl.h"

static pthread_t execute_thread;

static int ridl_options = IDL_INIT_CLARGS;

static int execute_cmd = 0;
static char *cmd;

static int execute_batch_file = 0;
static char *batch_file;

static int logging = 0;
static char *log_file;
static FILE *log_fp;

static IDL_MSG_DEF msg_arr[] = {  
#define M_RIDL_SIGNAL_REG       0
  {  "M_RIDL_SIGNAL_REG",   "%NSignal registration problem." }, 
};
static IDL_MSG_BLOCK msg_block; 

// TODO: need to figure out what this should be
static char *history_file_location = "/Users/mgalloy/.idl/itt/rbuf/history";

int ridl_really_exit = 1;


/*
   Called when the initial IDL startup text has finished printing.
*/
void ridl_inittextdone(void) {
  
}


char *ridl_command_generator(const char *text, int state) {
  if (!state) {
    char *result = (char *)malloc(6);
    strcpy(result, "print");
    return(result);
  } else { 
    return((char *)NULL);
  }
}


char **ridl_completion(const char *text, int start, int end) {
  char **matches = (char **) NULL;
  // determines state and calls the correct generator
    
  //matches = rl_completion_matches(text, ridl_command_generator);
  //return(matches);
  
  //printf("\ntext = '%s', start = %d, end = %d\n", text, start, end);
  return((char **) NULL);
}


void ridl_logoutput(int flags, char *buf, int n) {
  char *output = (char *)malloc(strlen(buf) + 1);
  strncpy(output, buf, n);
  output[n] = '\0';
  
  printf("%s", output);
  if (flags & IDL_TOUT_F_NLPOST) printf("\n");

  fprintf(log_fp, "%s", output);
  if (flags & IDL_TOUT_F_NLPOST) fprintf(log_fp, "\n");
  
  free(output);
}


void ridl_updateprompt(void) {
  sprintf(ridl_expandedprompt, ridl_prompt, ridl_cmdnumber);
}


/*
   Registered to be called when the !prompt changes.
*/
void ridl_changeprompt(IDL_STRING *prompt) {
  ridl_prompt = IDL_STRING_STR(prompt);
  ridl_updateprompt();
}


/* 
   Registered to be called when the current working directory changes.
*/
void ridl_changewdir(char *dir) {
  // TODO: modify prompt
  //printf("Changing dirs to %s\n", dir);
}


/*
   Reads IDL history file and populates the Readline history.
*/
void ridl_populatehistory(void) {
  FILE *fp = fopen(history_file_location, "r");
  int i, line_number = 0, rline_number;
  char history[RIDL_RBUF_SIZE][RIDL_MAX_LINE_LENGTH];
  char line[RIDL_MAX_LINE_LENGTH];
  
  while (fgets(line, RIDL_MAX_LINE_LENGTH, fp) != NULL) {
    for (i = strlen(line); i > 0; i--) {
      if (line[i] == '<') { 
        line[i] = '\0';
      }
    }
    strcpy(history[line_number++], line);
  }
  
  for (rline_number = line_number - 1; rline_number >= 0; rline_number--) {
    add_history(history[rline_number]);
    ridl_cmdnumber++;
  }
  
  fclose(fp);
}

//void *ridl_pexecutestr(void *cmd) {
//  printf("cmd in pthread = '%s'\n", (char *)cmd);
//  int result = IDL_ExecuteStr((char *)cmd);
//}


/*
   Execute an IDL command typed at the command line by the user.
*/
int ridl_executestr(char *cmd) {
  if (logging) fprintf(log_fp, "%s%s\n", ridl_prompt, cmd);

  //int rc = pthread_create(&execute_thread, NULL, ridl_pexecutestr, (void *)cmd);
  int result = IDL_ExecuteStr(cmd);
  
  //pthread_cancel(execute_thread);

  add_history(cmd);
  ridl_updateprompt();
  // TODO: also add to IDL history

  fprintf(stderr, "\e[0m");   // reset colors if there was a compile error
  return(0);
}


/*
   Registered to be called before a compiler error is shown.
*/
void ridl_show_compile_error(void) {
  fprintf(stderr, "\e[31m");
}


/*
   Registered to be called after IDL_Cleanup is called. 
*/
void ridl_deathhint(void) {
}


/*
   Called when EXIT routine is encountered.
*/
void ridl_exit(void) {
  int result = IDL_Cleanup(IDL_FALSE);
}


/*
   Handle any rIDL cleanup before exiting.
*/
void ridl_exit_handler(void) {
  if (logging) fclose(log_fp);

  exit(EXIT_SUCCESS);
}


/*
   Print rIDL welcome message if not quiet.
*/
void ridl_welcome(void) {
  if (!(ridl_options & IDL_INIT_QUIET)) {
    printf("rIDL 0.1: Really Interactive Data Language\n");  
  }
}


/*
   Return the index of the first non-space character on the line.
*/
int ridl_firstchar(char *line) {
  int i;
  for (i = 0; i < strlen(line); i++) {
    if (line[i] != ' ') return(i);
  }
}


/*
   Return the word starting from index start on line. The result is 
   dynamically allocated, so should be freed by the calling routine.
*/
char *ridl_getnextword(char *line, int start) {
  int i;
  for (i = start; i < strlen(line); i++) {
    if (line[i] == ' ') break;
  }
  
  int end = i;
  
  char *result = (char *)calloc(i - start + 1, 1);
  for (i = start; i < end; i++) {
    result[i - start] = line[i];
  }
  
  return(result);
}


/*
   Launch editor in $EDITOR environment variable.
*/
void ridl_launcheditor(char *filename) {
  char *cmdFormat = "ridl_launcheditor, '%s'";
  char *cmd = (char *)malloc(strlen(filename) + strlen(cmdFormat) + 1);
  sprintf(cmd, cmdFormat, filename);
  int result = IDL_ExecuteStr(cmd);
  free(cmd);
}


/*
   Prints version information about rIDL and IDL to stdout.
*/
void ridl_printversion(void) {
  IDL_STRING *version = IDL_SysvVersionRelease();
  IDL_STRING *os = IDL_SysvVersionOS();
  IDL_STRING *arch = IDL_SysvVersionArch();
  printf("rIDL %s: Really Interactive Data Language\n", ridl_version);
  printf("IDL %s %s %s\n", 
         IDL_STRING_STR(version), 
         IDL_STRING_STR(os), 
         IDL_STRING_STR(arch));
}


void ridl_printmagichelp(void) {
  char *indent = "  ";
  int magic_width = 16;
  
  ridl_printversion();
  
  printf("\nmagic commands:\n");
  
  printf("%s%-*s %s\n", indent, magic_width, ":doc routine", 
         "show calling syntax and comment header for the routine");
  printf("%s%-*s %s\n", indent, magic_width, ":help", 
         "show this help message");
  printf("%s%-*s %s\n", indent, magic_width, ":log filename", 
         "start logging all commands and output to filename");
  printf("%s%-*s %s\n", indent, magic_width, ":unlog", 
         "stop logging");
}


/*
   Print usage and options for rIDL.
*/
void ridl_printhelp(void) {
  char *indent = "  ";    // prefix for each line of switch help
  int switch_width = 25;  // width of switch column in spaces
  
  ridl_printversion();
  
  printf("\nusage: ridl [options]\n\n");

  printf("options:\n");
  printf("%s%-*s %s\n", indent, switch_width, "-32", 
         "start in 32-bit mode");
  printf("%s%-*s %s\n", indent, switch_width, "-arg value", 
         "saves a single command line argument");
  printf("%s%-*s %s\n", indent, switch_width, "-args value1 value2 ...", 
         "saves multiple command line arguments");
  printf("%s%-*s %s\n", indent, switch_width, "-demo", 
         "start in 7-minute demo mode");
  printf("%s%-*s %s\n", indent, switch_width, "-e command", 
         "executes command and exit");
  printf("%s%-*s %s\n", indent, switch_width, "-em=FILENAME", 
         "execute the given .sav file containing an embedded license");
  printf("%s%-*s %s\n", indent, switch_width, "-h", 
         "display this help message");
  printf("%s%-*s %s\n", indent, switch_width, "-novm", 
         "use 7-minute demo mode instead of virtual machine with -rt");
  printf("%s%-*s %s\n", indent, switch_width, "-pref=FILENAME", 
         "load the given preference file");
  printf("%s%-*s %s\n", indent, switch_width, "-queue", 
         "wait for a license when none are available");
  printf("%s%-*s %s\n", indent, switch_width, "-quiet", 
         "suppress printing information about IDL on startup");
  printf("%s%-*s %s\n", indent, switch_width, "-rt=FILENAME", 
         "start with a runtime license on the given .sav file");
  printf("%s%-*s %s\n", indent, switch_width, "-student", 
         "start in student mode");
  printf("%s%-*s %s\n", indent, switch_width, "-ulicense", 
         "start IDL with a unique license");
  printf("%s%-*s %s\n", indent, switch_width, "-v", 
         "display version information");
  printf("%s%-*s %s\n", indent, switch_width, "-vm=FILENAME", 
         "start the virtual machine with the given .sav file");
}


/*
   rIDL main loop.
*/
int main(int argc, char *argv[]) {  
  int a;
  for (a = 1; a < argc; a++) {
    if (strcmp(argv[a], "-arg") == 0) {
      // TODO: handle
    } else if (strcmp(argv[a], "-args") == 0) {
      // TODO: handle
    } else if (strcmp(argv[a], "-demo") == 0) {
      // TODO: handle
    } else if (strcmp(argv[a], "-e") == 0) {
      if ((a + 1) < argc) {
        execute_cmd = 1;
        cmd = argv[++a];
      }
    } else if (strcmp(argv[a], "-em") == 0) {
      // TODO: handle
    } else if (strcmp(argv[a], "-h") == 0) {
      ridl_printhelp();
      exit(EXIT_SUCCESS);
    } else if (strcmp(argv[a], "-novm") == 0) {
      // TODO: handle
    } else if (strcmp(argv[a], "-pref") == 0) {
      // TODO: handle
    } else if (strcmp(argv[a], "-quiet") == 0) {
      ridl_options |= IDL_INIT_QUIET;
    } else if (strcmp(argv[a], "-queue") == 0) {
      ridl_options |= IDL_INIT_LMQUEUE;
    } else if (strcmp(argv[a], "-rt") == 0) {
      // TODO: handle
      // use: int IDL_RuntimeExec(char * file)
    } else if (strcmp(argv[a], "-student") == 0) {
      // TODO: handle
    } else if (strcmp(argv[a], "-ulicense") == 0) {
      // TODO: handle
    } else if (strcmp(argv[a], "-v") == 0) { 
      ridl_printversion();
      exit(EXIT_SUCCESS);
    } else if (strcmp(argv[a], "-vm") == 0) {
      // TODO: handle
    } else if (strcmp(argv[a], "-32") == 0) {
      // need to accept -32, but don't need to do anything
    } else {
      execute_batch_file = 1;
      batch_file = argv[a];
    }
  }
    
  IDL_INIT_DATA init_data;
  init_data.options = ridl_options;
  init_data.clargs.argc = argc;
  init_data.clargs.argv = argv;
  
  ridl_welcome();
  
  IDL_UicbRegInitTextDone(ridl_inittextdone);
                                               
  if (IDL_Initialize(&init_data)) {
    IDL_ExitRegister(ridl_exit_handler);
    IDL_UicbRegExitDone(ridl_exit);
    IDL_UicbRegShowCompileErr(ridl_show_compile_error);
    IDL_UicbRegPromptChange(ridl_changeprompt);
    IDL_UicbRegWorkingDirChange(ridl_changewdir);
    IDL_UicbRegDeathHint(ridl_deathhint);
    
    if (!(msg_block = IDL_MessageDefineBlock("RIDL_MSG_BLOCK", 
                                             IDL_CARRAY_ELTS(msg_arr),  
                                             msg_arr))) return(1);                                            
    
    using_history();
    ridl_populatehistory();
    rl_attempted_completion_function = ridl_completion;
    
    // TODO: add completers for routines and variables completion
    
    // handle -e option if it was present
    if (execute_cmd) {
      int error = IDL_ExecuteStr(cmd);
      return(IDL_Cleanup(IDL_FALSE));
    }
    
    if (execute_batch_file) {
      char *batch_cmd = (char *)malloc(strlen(batch_file) + 2);
      sprintf(batch_cmd, "@%s", batch_file);
      int error = IDL_ExecuteStr(batch_cmd);
      free(batch_cmd);
    }
    
    ridl_updateprompt();
    
    while (1) {      
      char *line = readline(ridl_expandedprompt);
      
      // normal exit by hitting ^D
      if (line == NULL) { 
        printf("\n");
        return(IDL_Cleanup(IDL_FALSE)); 
      };
      
      // magic lines start with :, anything else is passed to IDL
      int firstcharIndex = ridl_firstchar(line);
      char firstchar = line[firstcharIndex];
      if (firstchar == ':') {
        if ((firstcharIndex + 1 < strlen(line)) 
              && (line[firstcharIndex + 1] == '!' || line[firstcharIndex + 1] == '^')) {
          // TODO: eventually this shouldn't be a magic command, but should be
          //       activated when a space is entered
          char *expansion;
          int expansion_result;
          char *expansion_line = (char *) malloc(strlen(line) + 1);
          strcpy(expansion_line, line + firstcharIndex + 1);
          expansion_result = history_expand(expansion_line, &expansion);
          switch (expansion_result) {
            case -1: 
              printf("Error in expansion\n");
              break;
            case 2: 
              printf("%s\n", expansion);
              break;
            case 0: 
            case 1: 
              //printf("%s%s\n", ridl_prompt, expansion);
              ridl_executestr(expansion);
              break;
          }
          ridl_cmdnumber++;
          ridl_updateprompt();
          
          free(expansion_line);
          free(expansion);
        } else {                          
          char *cmd = ridl_getnextword(line, firstcharIndex);
          if (strcmp(cmd, ":doc") == 0) {
            char *routine = ridl_getnextword(line, firstcharIndex + 5);
            char *man = (char *)malloc(8 + strlen(routine));
            sprintf(man, "man, '%s'", routine);
            int error = IDL_ExecuteStr(man);
            free(man);
            free(routine);
          } else if (strcmp(cmd, ":log") == 0) {
            if (logging) fclose(log_fp);

            logging = 1;
            char *filename = ridl_getnextword(line, firstcharIndex + 5);
            log_fp = fopen(filename, "w");
            IDL_ToutPush(ridl_logoutput);
            free(filename);
          } else if (strcmp(cmd, ":unlog") == 0) {
            if (logging) {
              fclose(log_fp);
              IDL_ToutPop();
            }
            logging = 0;
          } else if (strcmp(cmd, ":help") == 0) {
            ridl_printmagichelp();
          } else {
            printf("Unknown magic command %s\n", cmd);
          }
          free(cmd);
        }
      } else {
        if (line && *line) {           
          // check for .edit
          if (firstchar == '.') {
            char *cmd = ridl_getnextword(line, firstcharIndex);
            if (strcmp(cmd, ".edit") == 0) {
              char *file = ridl_getnextword(line, firstcharIndex + strlen(cmd) + 1);
              ridl_launcheditor(file);              
              free(file);
              
              add_history(line);
              ridl_cmdnumber++;
              ridl_updateprompt();
            } else {
              int error = ridl_executestr(line);
              ridl_cmdnumber++;              
              ridl_updateprompt();
            }
            free(cmd);
          } else {
            int error = ridl_executestr(line);
            ridl_cmdnumber++;
            ridl_updateprompt();
          }
        }
      }
      
      free(line);
    }    
  } else {
    printf("Failed to initialize Callable IDL session.\n");
    return(EXIT_FAILURE);
  }
  
  return(EXIT_FAILURE);
}
