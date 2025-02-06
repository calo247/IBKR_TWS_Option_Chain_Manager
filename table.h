/*
Tyler Pelling
tpell114@mtroyal.ca
COMP3659
December 6, 2024
*/

#ifndef TABLE_H
#define TABLE_H

#include <ncurses.h>
#include <string>
#include <string.h>
#include <map>
#include <set>
#include <iomanip>
#include "Contract.h"
#include "terminal.h"

using namespace std;

#define HEADER_HEIGHT 1
#define HEADER_WIDTH 70
#define HEADER_START_Y 0
#define HEADER_START_X 1
#define TABLE_HEIGHT 33 //keep odd number for symmetry
#define TABLE_WIDTH 70
#define TABLE_START_Y 1
#define TABLE_START_X 1
#define FOOTER_HEIGHT 2
#define FOOTER_WIDTH 70
#define FOOTER_START_Y 35
#define FOOTER_START_X 1
#define MAX_ROWS 33
#define DATA_COLUMNS 7
#define COLUMN_WIDTH TABLE_WIDTH / DATA_COLUMNS
#define CALL_BID_COLUMN 0
#define CALL_ASK_COLUMN 1
#define CALL_LAST_COLUMN 2
#define STRIKE_COLUMN 3
#define PUT_BID_COLUMN 4
#define PUT_ASK_COLUMN 5
#define PUT_LAST_COLUMN 6

class Table {

private:
    WINDOW* headerWindow;
    WINDOW* tableWindow;
    WINDOW* footerWindow;
    mutex tableMutex;

    void drawBorders();
    void drawFooter();
    void drawHeader();
    void drawStrikes(int closestStrike);
    WINDOW* getHeaderWindow();
    WINDOW* getTableWindow();
    
public:
    map<TickerId, pair<bool, int>> tickerToRowIndex;
    set<double> strikes;
    map<double, int> activeStrikes;

    ~Table();

    void drawCell(int rowIndex, int columnIndex, const string text);
    void initializeTable(set<double> strikes, int closestStrike);
    int getRowIndex(double strike);
    string formatNumber(double number);
    string formatNumber2(double number);
};

#endif
