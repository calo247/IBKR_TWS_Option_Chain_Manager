/*
Tyler Pelling
tpell114@mtroyal.ca
COMP3659
December 6, 2024
*/

#include "terminal.h"
#include <stdexcept>
#include <sys/wait.h>

using namespace std;

set<string> supportedSymbols = {"ES", "NQ"};
set<string> supportedExpiries = {"20241220", "20250321"};
string selectedSymbol;

/**
 * Resizes the terminal to the specified dimensions.
 *
 * @param x The new width of the terminal
 * @param y The new height of the terminal
 */
void resizeTerminal(int x, int y) {
    printf("\033[8;%d;%dt", x, y);
}

/**
 * Creates a new window with the given dimensions and position.
 *
 * @param height The height of the new window
 * @param width The width of the new window
 * @param start_y The y-coordinate of the top-left corner of the new window
 * @param start_x The x-coordinate of the top-left corner of the new window
 * @return A pointer to the new window, or nullptr if creation failed
 */
WINDOW* createWindow(int height, int width, int start_y, int start_x) {

    WINDOW* local_win = newwin(height, width, start_y, start_x);

    if (local_win == nullptr) return nullptr;
    wrefresh(local_win);
    return local_win;
}

/**
 * Retrieves the system's default gateway.
 *
 * This function attempts to determine the default gateway by executing
 * the "ip route" command and parsing its output. It creates a pipe for
 * inter-process communication, forks a child process to run the command,
 * and reads the output from the pipe. The function extracts the default
 * gateway from the command's output and returns it as a string.
 *
 * @return A string representing the default gateway IP address, or an
 *         empty string if an error occurs or if the gateway cannot be
 *         determined.
 *
 * @note Errors are logged to standard error output. If the function
 *       encounters an error during pipe creation, process forking, or
 *       command execution, it returns an empty string.
 */
string getDefaultGateway() {

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        write(STDERR_FILENO,"Failed to create pipe\n", 22);
        return "";
    }

    pid_t pid = fork();
    if (pid == -1) {
        write(STDERR_FILENO,"Failed to fork process\n", 23);
        return "";
    }

    int originalFd = dup(STDOUT_FILENO);

    if (pid == 0) {

        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execlp("ip", "ip", "route", nullptr);

        write(STDERR_FILENO,"Failed to execute command\n", 26);
        exit(1);
    } else {
        
        close(pipefd[1]);

        char buffer[256];
        string result;
        ssize_t bytesRead;

        while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            result += buffer;
        }

        close(pipefd[0]);

        int status;
        waitpid(pid, &status, 0);

        if (status != 0) {
            write(STDERR_FILENO, "Command execution failed\n", 25);
            return "";
        }

        istringstream iss(result);
        string word, gateway;

        while (iss >> word) {
            if (word == "via") {
                iss >> gateway;
                break;
            }
        }

        if (gateway.empty()) {
            write(STDERR_FILENO, "Default gateway not found\n", 26);
            return "";
        } else {
            write(STDOUT_FILENO, "Using default gateway: ", 23);
            write(STDOUT_FILENO, gateway.c_str(), gateway.length());
            write(STDOUT_FILENO, "\n", 1);
        }

        dup2(originalFd, STDOUT_FILENO);
        close(originalFd);

        return gateway;
    }
}

/**
 * Prompts the user to input a symbol until a valid symbol is entered.
 * A valid symbol is one that is in the set of supportedSymbols.
 *
 * @return The valid symbol entered by the user.
 */
string getSymbol() {

    while (true) {

        write(STDOUT_FILENO, "Supported symbols: ", 19);
        for (string symbol : supportedSymbols) {
            write(STDOUT_FILENO, symbol.c_str(), symbol.length());
            write(STDOUT_FILENO, ", ", 2);
        }
        write(STDOUT_FILENO, "\n", 1);

        string symbol;
        write(STDOUT_FILENO, "Enter symbol: ", 14);
        getline(cin, symbol);
        transform(symbol.begin(), symbol.end(), symbol.begin(), ::toupper);

        if (supportedSymbols.find(symbol) == supportedSymbols.end()) {
            write(STDERR_FILENO, "Invalid symbol\n", 15);
        } else {
            selectedSymbol = symbol;
            return symbol;
        }
    } 
}

/**
 * Prompts the user to input an expiry date until a valid expiry is entered.
 * A valid expiry is one that is in the set of supportedExpiries.
 *
 * @return The valid expiry date entered by the user.
 */
string getExpiry() {

    while (true) {

        write(STDOUT_FILENO, "Supported expiries: ", 20);
        for (string expiry : supportedExpiries) {
            write(STDOUT_FILENO, expiry.c_str(), expiry.length());
            write(STDOUT_FILENO, ", ", 2);
        }
        write(STDOUT_FILENO, "\n", 1);

        string expiry;
        write(STDOUT_FILENO, "Enter expiry: ", 14);
        getline(cin, expiry);

        if (supportedExpiries.find(expiry) == supportedExpiries.end()) {
            write(STDERR_FILENO, "Invalid expiry\n", 15);
        } else {
            return expiry;
        }
    }
}
