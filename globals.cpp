/*
Tyler Pelling
tpell114@mtroyal.ca
COMP3659
December 6, 2024
*/

#include "globals.h"

My_wrapper my_wrapper;
unique_ptr<OptionChainManager> optionChainManager = make_unique<OptionChainManager>();
int logFileFd;
mutex logFileMutex;
