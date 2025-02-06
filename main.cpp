/*
Tyler Pelling
tpell114@mtroyal.ca
COMP3659
December 6, 2024
*/

#include "my_wrapper.h"
#include "globals.h"

int main() {

    resizeTerminal(TERMINAL_HEIGHT + 1, TERMINAL_WIDTH);
    
    string host = getDefaultGateway();
    string symbol = getSymbol();
    string expiry = getExpiry();
    int clientId = 1;    
    
    write(STDOUT_FILENO, "Loading...\n", 11);

    logFileFd = open("logFile.log", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);       
    if (logFileFd < 0) {
        write(STDERR_FILENO,"Failed to open log file", 23);
        exit(EXIT_FAILURE);
    }    
    
    if (host != "") {
        if(!my_wrapper.connect(host.c_str(), PORT_LIVE, clientId)) {
            write(STDERR_FILENO,"Failed to connect\n", 18);
            return 1;
        }
    } else {
        write(STDERR_FILENO,"Failed to get default gateway\n", 30);
        return 1;
    }
    sleep(1);                     // wait for callback from different datafarms
    my_wrapper.processMessages(); // process callbacks from datafarms

    my_wrapper.requestOptionChain(symbol, DEFAULT_EXCHANGE, FUTURES_CODE, DEFAULT_CURRENCY, expiry);
    while(optionChainManager->isInitialized == false) my_wrapper.processMessages();

    my_wrapper.requestMarketData();
    my_wrapper.processMessagesMultithreaded();
    my_wrapper.cancelMarketData();
    my_wrapper.disconnect();
    write(STDOUT_FILENO, "disconnected\n", 13);

    return 0;
}
