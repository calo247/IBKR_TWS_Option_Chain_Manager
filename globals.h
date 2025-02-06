/*
Tyler Pelling
tpell114@mtroyal.ca
COMP3659
December 6, 2024
*/

#ifndef GLOBALS_H
#define GLOBALS_H

#include "my_wrapper.h"
#include "optionChainManager.h"

#define DELAYED_DATA_TYPE 3
#define FUTURES_CODE "FUT"
#define FUTURES_OPTION_CODE "FOP"
#define DEFAULT_EXCHANGE "CME"
#define DEFAULT_CURRENCY "USD"
#define PORT_LIVE 7497
#define PORT_PAPER 7496

extern My_wrapper my_wrapper;
extern unique_ptr<OptionChainManager> optionChainManager;
extern int logFileFd;
extern mutex logFileMutex;

#endif