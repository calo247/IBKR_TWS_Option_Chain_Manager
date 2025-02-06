/*
Tyler Pelling
tpell114@mtroyal.ca
COMP3659
December 6, 2024
*/

#include "table.h"

using namespace std;

//public methods

/**
 * Destructor. Closes the table's windows and the ncurses environment.
 */
Table::~Table() {
    delwin(headerWindow);
    delwin(tableWindow);
    delwin(footerWindow);
    endwin();
}

/**
 * Draws a cell in the table window.
 *
 * @param rowIndex The row index of the cell to draw.
 * @param columnIndex The column index of the cell to draw.
 * @param text The text to draw in the cell.
 */
void Table::drawCell(int rowIndex, int columnIndex, const string text) {

    int textStart = columnIndex * COLUMN_WIDTH + (COLUMN_WIDTH - text.length()) / 2;

    unique_lock<mutex> lockTable(tableMutex);
    mvwprintw(tableWindow, rowIndex, textStart, "%s", text.c_str());
    wrefresh(tableWindow);
    lockTable.unlock();
}

/**
 * Initializes the ncurses environment and the table's windows.
 *
 * @param strikes The set of all possible strikes to be displayed in the table.
 * @param closestStrike The strike that should be at the middle row of the table
 */
void Table::initializeTable(set<double> strikes, int closestStrike) {

    this->strikes = strikes;

    initscr();
    cbreak();
    noecho();
    refresh();

    this->headerWindow = createWindow(HEADER_HEIGHT, HEADER_WIDTH, HEADER_START_Y, HEADER_START_X);
    this->tableWindow = createWindow(TABLE_HEIGHT, TABLE_WIDTH, TABLE_START_Y, TABLE_START_X);
    this->footerWindow = createWindow(FOOTER_HEIGHT, FOOTER_WIDTH, FOOTER_START_Y, FOOTER_START_X);

    drawHeader();
    drawBorders();
    drawFooter();
    refresh();
    drawStrikes(closestStrike);      
}

/**
 * Retrieves the row index for a given strike price.
 *
 * @param strike The strike price to search for in the activeStrikes map.
 * @return The row index associated with the given strike price if found,
 *         otherwise returns -1 if the strike price is not in the map.
 */
int Table::getRowIndex(double strike) {
    
    map<double, int>::iterator it = this->activeStrikes.find(strike);

    if(it == activeStrikes.end()) {
        return -1;
    } else {
        return it->second;
    }
}

/**
 * Formats a given number to a string representation.
 *
 * If the fractional part of the number is less than 0.1,
 * it is formatted with no decimal places. Otherwise, it is
 * formatted with one decimal place.
 *
 * @param number The number to be formatted.
 * @return A string representation of the formatted number.
 */
string Table::formatNumber(double number) {

    ostringstream oss;
    double fractionalPart = number - static_cast<int>(number);
    
    if (fractionalPart >= 0.0 && fractionalPart < 0.1) {
        oss << fixed << setprecision(0) << number;
    } else {
        oss << fixed << setprecision(1) << number;
    }

    return oss.str();
}

/**
 * Formats a given number to a string representation.
 *
 * The given number is formatted to a string with two decimal
 * places.
 *
 * @param number The number to be formatted.
 * @return A string representation of the formatted number.
 */
string Table::formatNumber2(double number) {

    ostringstream oss;
    oss << fixed << setprecision(2) << number;
    return oss.str();
}

/**
 * @return The window that displays the header of the table.
 */
WINDOW* Table::getHeaderWindow() {
    return this->headerWindow;
}

/**
 * @return The window that displays the body of the table.
 */
WINDOW* Table::getTableWindow() {
    return this->tableWindow;
}

//private methods

/**
 * Draws horizontal borders in the table window.
 *
 * This function uses ncurses to draw horizontal lines at the bottom 
 * of the header and the top of the footer within the table window. 
 * It then refreshes the table window to display the changes.
 */
void Table::drawBorders() {
    mvhline(HEADER_HEIGHT, 0, ACS_HLINE, TERMINAL_WIDTH);
    mvhline(TERMINAL_HEIGHT - FOOTER_HEIGHT, 0, ACS_HLINE, TERMINAL_WIDTH);
    wrefresh(this->tableWindow);
}

/**
 * Draws the footer of the table window.
 *
 * The footer displays a message to the user on how to quit the
 * application.
 */
void Table::drawFooter() {
    mvwprintw(this->footerWindow, 0, 0, "%s", "Press 'q' to quit");
    wrefresh(this->footerWindow);
}

/**
 * Draws the header of the table window.
 *
 * This function uses ncurses to draw the header of the table window.
 * The header displays column headers for the table.
 */
void Table::drawHeader() {
    mvwprintw(headerWindow, 0, CALL_BID_COLUMN * COLUMN_WIDTH + (COLUMN_WIDTH - strlen("Bid")) / 2, "%s", "Bid");
    mvwprintw(headerWindow, 0, CALL_ASK_COLUMN * COLUMN_WIDTH + (COLUMN_WIDTH - strlen("Ask")) / 2, "%s", "Ask");
    mvwprintw(headerWindow, 0, CALL_LAST_COLUMN * COLUMN_WIDTH + (COLUMN_WIDTH - strlen("Last")) / 2, "%s", "Last");
    mvwprintw(headerWindow, 0, STRIKE_COLUMN * COLUMN_WIDTH + (COLUMN_WIDTH - strlen("Strike")) / 2, "%s", "Strike");
    mvwprintw(headerWindow, 0, PUT_BID_COLUMN * COLUMN_WIDTH + (COLUMN_WIDTH - strlen("Bid")) / 2, "%s", "Bid");
    mvwprintw(headerWindow, 0, PUT_ASK_COLUMN * COLUMN_WIDTH + (COLUMN_WIDTH - strlen("Ask")) / 2, "%s", "Ask");
    mvwprintw(headerWindow, 0, PUT_LAST_COLUMN * COLUMN_WIDTH + (COLUMN_WIDTH - strlen("Last")) / 2, "%s", "Last");
    wrefresh(headerWindow); 
}

/**
 * @brief Draws the strike prices on the table.
 *
 * Draws the strike prices on the table, starting from the closest strike
 * and going up and down. The strike prices are drawn in the strike column
 * and the text is centered in the cell.
 *
 * @param closestStrike The strike closest to the current underlying price.
 */
void Table::drawStrikes(int closestStrike) {

    set<double>::iterator itClosest = strikes.find(closestStrike);
    set<double>::iterator it = itClosest;
    string text = formatNumber(*it);

    for(int i = MAX_ROWS/2 + 1; i < MAX_ROWS; i++) {    //draw second half of strikes
        drawCell(i, STRIKE_COLUMN, text);
        activeStrikes.insert(pair<double, int>(*it, i));
        it.operator++();
        if(it == strikes.end()) break;
        text = formatNumber(*it);
    }

    it = itClosest;     //draw first half of strikes
    it.operator--();
    text = formatNumber(*it);
    for(int i = MAX_ROWS/2; i > 0; i--) {
        drawCell(i, STRIKE_COLUMN, text);
        activeStrikes.insert(pair<double, int>(*it, i));
        it.operator--();
        text = formatNumber(*it);
    } 
}
