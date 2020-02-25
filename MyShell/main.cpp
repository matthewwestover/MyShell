#include "shelpers.hpp"
#include <readline/readline.h>

// C++ program functions similar to a command shell.
// Takes in command line commands + any flags
int main(int argc, const char * argv[]) {
    std::vector<pid_t> background; // allows background processes to run. Not 100% if this is working right
    char* buffer; //readline autocompletes with a char* not a string
    
    // Tab completion for filepaths added (Bells & Whistles)
    while((buffer = readline("> "))) {
        
        // Convert buffer to a string to be used
        std::string line(buffer);
        // Easy shell termination
        if(line == "exit" || line == "Exit" || line == "quit" || line == "Quit"){
            std::cout << "Exiting Shell\n";
            exit(0);
        }
        
        // Accidental blank line handling
        if(line == "") {
            continue;
        }
    
        std::vector<std::string> tokens = tokenize(line);
        std::vector<Command> commands = getCommands(tokens);
        std::vector<pid_t> pids;
        
        for (int i = 0; i < commands.size(); i++) {
            // Don't go further if command is to cd or set env var
            if (commands[i].exec == "cd") {
                continue;
            }
            
            pid_t pid = fork();
            // Check if background process or not
            if(commands[i].background) {
                background.push_back(pid);
            } else {
                pids.push_back(pid);
            }
            
            // Forking Error
            if (pid < 0) {
                perror("Fork Error");
                exit(1);
                
            // Child process
            } else if (pid == 0) {
                int d1 = dup2(commands[i].fdStdin, 0);
                int d2 = dup2(commands[i].fdStdout, 1);
                if (d1 < 0 || d2 < 0) {
                    perror("File Descriptor Failure in Child Process");
                    exit(1);
                }
                for (Command c : commands) {
                    if (c.fdStdin != 0) {
                        close(c.fdStdin);
                    }
                    if (c.fdStdout != 1) {
                        close(c.fdStdout);
                    }
                }
                
                Command c = commands[i];
                if (execvp(c.exec.c_str(), const_cast<char* const *>(c.argv.data())) < 0) {
                    perror("Bad Command");
                }
            }
        }
        
        // Clean up fd stuff
        for (Command c : commands) {
            if (c.fdStdin != 0) {
                close(c.fdStdin);
            }
            if (c.fdStdout != 1) {
                close(c.fdStdout);
            }
        }

        // Wait for children to finish before doing anything else
        for (pid_t pid : pids) {
            int status;
            waitpid(pid, &status, 0);
            if (status < 0) {
                perror("Waiting for Children Error");
                exit(1);
            }
        }
    }
    free(buffer);
}
