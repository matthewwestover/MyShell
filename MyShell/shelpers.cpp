#include "shelpers.hpp"

// splitOnSymbol is a method provided for this assignment
// Searches to see if a symbol is present in a vector of strings.
// Returns true if it is present.
bool splitOnSymbol(std::vector<std::string>& words, int i, char c){
    if(words[i].size() < 2) { return false; }
    
    int pos;
    if((pos = (int) words[i].find(c)) != std::string::npos) { // added cast to int to clear warning flag
        if(pos == 0) {
            //starts with symbol
            words.insert(words.begin() + i + 1, words[i].substr(1, words[i].size() -1));
            words[i] = words[i].substr(0,1);
        } else {
            //symbol in middle or end
            words.insert(words.begin() + i + 1, std::string{c});
            std::string after = words[i].substr(pos + 1, words[i].size() - pos - 1);
            if(!after.empty()){
                words.insert(words.begin() + i + 2, after);
            }
            words[i] = words[i].substr(0, pos);
        }
        return true;
    } else {
        return false;
    }
}

// tokenize is a method provided for this assignment
// Splits incoming string into individual tokens.
// Splits on space, <, >, |, and &
std::vector<std::string> tokenize(const std::string& s){
    std::vector<std::string> ret;
    int pos = 0;
    int space;
    
    while((space = (int) s.find(' ', pos)) != std::string::npos) { // added cast to int to clear warning flag
        std::string word = s.substr(pos, space - pos);
        if(!word.empty()) {
            ret.push_back(word);
        }
        pos = space + 1;
    }
    
    std::string lastWord = s.substr(pos, s.size() - pos);
    if(!lastWord.empty()) {
        ret.push_back(lastWord);
    }
    
    for(int i = 0; i < ret.size(); ++i) {
        for(auto c : {'&', '<', '>', '|'}) {
            if(splitOnSymbol(ret, i, c)) {
                --i;
                break;
            }
        }
    }
    return ret;
}

// This is an overloaded << operator.
// Provided for this assignment
std::ostream& operator<<(std::ostream& outs, const Command& c){
    outs << c.exec << " argv: ";
    for(const auto& arg : c.argv) { if(arg) {outs << arg << ' ';}}
    outs << "fds: " << c.fdStdin << ' ' << c.fdStdout << ' ' << (c.background ? "background" : "");
    return outs;
}

// Parts of getCommands was provided for this assignment
// Parts were implemented by me.
std::vector<Command> getCommands(const std::vector<std::string>& tokens){
    std::vector<Command> ret(std::count(tokens.begin(), tokens.end(), "|") + 1);  //1 + num |'s commands
    int first = 0;
    int last = (int)(std::find(tokens.begin(), tokens.end(), "|") - tokens.begin()); // added cast to int to clear warning flag
    bool error = false;
    
    for(int i = 0; i < ret.size(); ++i) {
        // Provided Error check that command doesnt start with one of these symbols
        if((tokens[first] == "&") || (tokens[first] == "<") || (tokens[first] == ">") || (tokens[first] == "|")) {
            error = true;
            break;
        }
        
        ret[i].exec = tokens[first];
        ret[i].argv.push_back(tokens[first].c_str()); // argv0 = program name
        std::cout << "exec start: " << ret[i].exec << std::endl;
        ret[i].fdStdin = 0;
        ret[i].fdStdout = 1;
        ret[i].background = false;
        
        for(int j = first + 1; j < last; ++j){
            
            // I/O Redirection. Implemented by me.
            if(tokens[j] == ">" || tokens[j] == "<" ) {
                std::string filePath = tokens[j + 1];
                int fd = -1;
                if (tokens[j] == ">") {
                    fd = open(filePath.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
                    ret[i].fdStdout = fd;
                    ret[i].argv.push_back(nullptr);
                }
                if (tokens[j] == "<") {
                    fd = open(filePath.c_str(), O_RDONLY, 0666);
                    ret[i].fdStdin = fd;
                }
                if (fd < 0) {
                    perror("I/O Error");
                    continue;
                }
            // Background commands.
            } else if(tokens[j] == "&") {
                ret[i].background = true;
            // Normal Argument
            } else {
                ret[i].argv.push_back(tokens[j].c_str());
            }
        }
        
        // 'cd' Command functionality. Implemented by me.
        if (ret[i].exec == "cd") {
            std::string rootDirectory = getenv("HOME");
            if (ret[i].argv.size() > 1) {
                rootDirectory = ret[i].argv[1];
            }
            int res = chdir(rootDirectory.c_str());
            if (res < 0) {
                perror("Change Directory Error");
                continue;
            }
        }
        
        // Create pipes for multiple commands. Implemented by me.
        if(i > 0){
            int fds[2];
            int res = pipe(fds);
            if (res < 0) {
                perror("Multiple Commands Error");
                error = true;
                exit(1);
            }
            
            // Connect the two ends of the pipes
            ret[i].fdStdin = fds[0];
            ret[i - 1].fdStdout = fds[1];
        }
        
        
        // Exec wants a nullptr at end. Provided for assignment.
        ret[i].argv.push_back(nullptr);
        
        // Find next pipe character. Provided for assignment.
        first = last + 1;
        if(first < tokens.size()){
            last = (int) (std::find(tokens.begin() + first, tokens.end(), "|") - tokens.begin()); // added cast to int to clear warning flag
        }
    }
    
    // If a must close program error, clean up file descriptors before exiting. Implemented by me.
    if(error){
        //close any file descriptors you opened in this function!
        for(Command c : ret) {
            if (c.fdStdin != 0) {
                close(c.fdStdin);
            }
            if (c.fdStdout != 1) {
                close(c.fdStdout);
            }
        }
        std::cout << "Fatal error in processing getCommands()\n";
        exit(1);
    }
    return ret;
}
