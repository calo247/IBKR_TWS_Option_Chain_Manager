/*
Tyler Pelling
tpell114@mtroyal.ca
COMP3659
December 6, 2024
*/

#ifndef TERMINAL_H
#define TERMINAL_H

#include "ncurses.h"
#include <unistd.h>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <set>

#define TERMINAL_HEIGHT 36
#define TERMINAL_WIDTH 72

using namespace std;

extern string selectedSymbol;

void resizeTerminal(int x, int y);
WINDOW* createWindow(int height, int width, int start_y, int start_x);
string getDefaultGateway();
string getSymbol();
string getExpiry();


#endif