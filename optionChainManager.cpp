/*
Tyler Pelling
tpell114@mtroyal.ca
COMP3659
December 6, 2024
*/

#include "optionChainManager.h"
#include <iostream>
#include "globals.h"

using namespace std;

/**
 * Increments the initialization callback count.
 */
void OptionChainManager::incrementInitCallbackCount() {
    this->initCallbackCount++;
}

/**
 * Initializes the option chain with the given set of strikes.
 *
 * This function sets up the option chain by creating OptionData entries for each strike 
 * in both call and put directions, initializing their market data values to zero. It assigns 
 * contract details and manages the mapping between ticker IDs and option pairs. The function 
 * requests contract details for each option and processes incoming messages until all callbacks 
 * are completed. It then requests market data for the underlying contract and updates the option 
 * chain's closest strike based on the received underlying price. Finally, it initializes the table 
 * with the strikes and logs the initialization process.
 *
 * @param strikes The set of strike prices for which to initialize the option chain.
 */
void OptionChainManager::initializeChain(const set<double>& strikes) {

    optionChainManager->strikes = strikes;
    optionChainManager->underlyingLast = 0;
    optionChainManager->underlyingTickerId = 0;

    int i = 1;

    for (const double& strike : strikes) {
        this->optionChain[{strike, "C"}] = make_unique<OptionData>();
        this->optionChain[{strike, "C"}]->ask = 0.0;
        this->optionChain[{strike, "C"}]->bid = 0.0;
        this->optionChain[{strike, "C"}]->last = 0.0;
        this->optionChain[{strike, "C"}]->contractDetails.contract.strike = strike;
        this->optionChain[{strike, "C"}]->contractDetails.contract.right = "C";
        this->optionChain[{strike, "C"}]->contractDetails.contract.secType = "FOP";
        this->optionChain[{strike, "C"}]->contractDetails.contract.symbol = optionChainManager->underlyingContractDetails.contract.symbol;
        this->optionChain[{strike, "C"}]->contractDetails.contract.lastTradeDateOrContractMonth = optionChainManager->underlyingContractDetails.contract.lastTradeDateOrContractMonth;
        this->tickerToPairMap[i] = {strike, "C"};
        this->pairToTickerMap[{strike, "C"}] = i;
        optionChainManager->contractCount++;
        i++;

        this->optionChain[{strike, "P"}] = make_unique<OptionData>();
        this->optionChain[{strike, "P"}]->ask = 0.0;
        this->optionChain[{strike, "P"}]->bid = 0.0;
        this->optionChain[{strike, "P"}]->last = 0.0;
        this->optionChain[{strike, "P"}]->contractDetails.contract.strike = strike;
        this->optionChain[{strike, "P"}]->contractDetails.contract.right = "P";
        this->optionChain[{strike, "P"}]->contractDetails.contract.secType = "FOP";
        this->optionChain[{strike, "P"}]->contractDetails.contract.symbol = optionChainManager->underlyingContractDetails.contract.symbol;
        this->optionChain[{strike, "P"}]->contractDetails.contract.lastTradeDateOrContractMonth = optionChainManager->underlyingContractDetails.contract.lastTradeDateOrContractMonth;
        this->tickerToPairMap[i] = {strike, "P"};
        this->pairToTickerMap[{strike, "P"}] = i;
        optionChainManager->contractCount++;
        i++;

        my_wrapper.requestContractDetails(this->optionChain[{strike, "C"}]->contractDetails.contract);
        my_wrapper.requestContractDetails(this->optionChain[{strike, "P"}]->contractDetails.contract);
    }

    while(this->initCallbackCount != this->contractCount) my_wrapper.processMessages();

    my_wrapper.requestUnderlyingMarketData();
    while(this->underlyingLast == 0) my_wrapper.processMessages();
    
    int closestStrike = findClosestStrike(this->underlyingLast);
    table.initializeTable(strikes, closestStrike);

    string toLog = "Option chain initialized for symbol: " + optionChainManager->underlyingContractDetails.contract.symbol + "\n";
    unique_lock<mutex> lockLogFile(logFileMutex);
    write(logFileFd, toLog.c_str(), toLog.length());
    lockLogFile.unlock();

    this->isInitialized = true;
}

/**
 * Sets the underlying contract details for this option chain manager.
 *
 * @param contractDetails The contract details received from TWS.
 */
void OptionChainManager::setUnderlyingContractDetails(ContractDetails contractDetails) {
    this->underlyingContractDetails = contractDetails;
}

/**
 * Sets the contract details for the option with the given strike and right.
 *
 * @param contractDetails The contract details received from TWS.
 */
void OptionChainManager::setContractDetails(ContractDetails contractDetails) {
    this->optionChain[{contractDetails.contract.strike, contractDetails.contract.right}]->contractDetails = contractDetails;
}

/**
 * Sets the underlying contract for this option chain manager.
 *
 * @param underlyingSymbol The symbol of the underlying contract
 * @param futFopExchange The exchange on which the underlying contract is traded
 * @param underlyingSecurityType The type of underlying security
 * @param currency The currency in which the underlying contract is traded
 * @param contractDate The last trade date/contract month for the underlying contract
 */
void OptionChainManager::setUnderlyingContract(string underlyingSymbol, string futFopExchange, string underlyingSecurityType,
                                                      string currency, string contractDate) {

    this->underlyingContractDetails.contract.symbol = underlyingSymbol;
    this->underlyingContractDetails.contract.exchange = futFopExchange;
    this->underlyingContractDetails.contract.secType = underlyingSecurityType;
    this->underlyingContractDetails.contract.currency = currency;
    this->underlyingContractDetails.contract.lastTradeDateOrContractMonth = contractDate;
}

/**
 * Updates the bid price for the given ticker ID. If the ticker ID is 0, the method updates the
 * underlying contract's bid price. Otherwise, it updates the bid price of the corresponding option
 * contract. The method logs the update and updates the table with the new value.
 *
 * @param tickerId The Ticker ID of the contract for which to update the bid price
 * @param bid The new bid price to update
 */
void OptionChainManager::updateBid(TickerId tickerId, double bid) {

    pair<double, string> pair = this->tickerToPairMap[tickerId];

    if(tickerId == 0) {
        lock_guard<mutex> lock_underlying(this->underlyingMutex);
        this->underlyingBid = bid;

        string toLog = "'Bid' updated for underlying contract: " + this->underlyingContractDetails.contract.symbol + "\n";

        unique_lock<mutex> lockLogFile(logFileMutex);
        write(logFileFd, toLog.c_str(), toLog.length());
        lockLogFile.unlock();
        return;
    } else {
        lock_guard<mutex> lock_option(this->optionChain[pair]->dataMutex);
        this->optionChain[pair]->bid = bid;

        string toLog = "'Bid' updated for Ticker ID: " + to_string(tickerId) + " Symbol: " 
                        + this->optionChain[pair]->contractDetails.contract.symbol 
                        + " Strike: " + to_string(pair.first) + " Type: " + pair.second + "\n";
    
        unique_lock<mutex> lockLogFile(logFileMutex);
        write(logFileFd, toLog.c_str(), toLog.length());
        lockLogFile.unlock();

        string text = this->table.formatNumber2(bid); 
        int rowIndex = this->table.getRowIndex(pair.first); 

        if (pair.second == "C") {
            this->table.drawCell(rowIndex, CALL_BID_COLUMN, text);
        } else {
            this->table.drawCell(rowIndex, PUT_BID_COLUMN, text);
        }
    }
}

/**
 * Updates the ask price for the given ticker ID. If the ticker ID is 0, the method updates the
 * underlying contract's ask price. Otherwise, it updates the ask price of the corresponding option
 * contract. The method logs the update and updates the table with the new value.
 *
 * @param tickerId The Ticker ID of the contract for which to update the ask price.
 * @param ask The new ask price to update.
 */
void OptionChainManager::updateAsk(TickerId tickerId, double ask) {

    pair<double, string> pair = this->tickerToPairMap[tickerId];

    if(tickerId == 0) {
        lock_guard<mutex> lock_underlying(this->underlyingMutex);
        this->underlyingAsk = ask;

        string toLog = "'Ask' updated for underlying contract: " + this->underlyingContractDetails.contract.symbol + "\n";

        unique_lock<mutex> lockLogFile(logFileMutex);
        write(logFileFd, toLog.c_str(), toLog.length());
        lockLogFile.unlock();    
        return;
    } else {
        lock_guard<mutex> lock_option(this->optionChain[this->tickerToPairMap[tickerId]]->dataMutex);
        this->optionChain[this->tickerToPairMap[tickerId]]->ask = ask; 
    
        string toLog = "'Ask' updated for Ticker ID: " + to_string(tickerId) + " Symbol: " 
                        + this->optionChain[this->tickerToPairMap[tickerId]]->contractDetails.contract.symbol 
                        + " Strike: " + to_string(this->tickerToPairMap[tickerId].first)
                        + " Type: " + this->tickerToPairMap[tickerId].second + "\n";

        unique_lock<mutex> lockLogFile(logFileMutex);
        write(logFileFd, toLog.c_str(), toLog.length());
        lockLogFile.unlock();

        string text = this->table.formatNumber2(ask); 
        int rowIndex = this->table.getRowIndex(pair.first); 

        if (pair.second == "C") {
            this->table.drawCell(rowIndex, CALL_ASK_COLUMN, text);
        } else {
            this->table.drawCell(rowIndex, PUT_ASK_COLUMN, text);
        }
    }
}

/**
 * Updates the last price for the given ticker ID. If the ticker ID is 0, the method updates the
 * underlying contract's last price. Otherwise, it updates the last price of the corresponding option
 * contract. The method logs the update and updates the table with the new value.
 *
 * @param tickerId The Ticker ID of the contract for which to update the last price.
 * @param last The new last price to update.
 */
void OptionChainManager::updateLast(TickerId tickerId, double last) {

    pair<double, string> pair = this->tickerToPairMap[tickerId];

    if(tickerId == 0) {
        lock_guard<mutex> lock_underlying(this->underlyingMutex);
        this->underlyingLast = last;

        string toLog = "'Last' updated for underlying contract: " + this->underlyingContractDetails.contract.symbol + "\n";

        unique_lock<mutex> lockLogFile(logFileMutex);
        write(logFileFd, toLog.c_str(), toLog.length());
        lockLogFile.unlock();
        return;
    } else {
        lock_guard<mutex> lock_option(this->optionChain[this->tickerToPairMap[tickerId]]->dataMutex);
        this->optionChain[this->tickerToPairMap[tickerId]]->last = last;
    

        string toLog = "'Last' updated for Ticker ID: " + to_string(tickerId) + " Symbol: " 
                        + this->optionChain[this->tickerToPairMap[tickerId]]->contractDetails.contract.symbol 
                        + " Strike: " + to_string(this->tickerToPairMap[tickerId].first)
                        + " Type: " + this->tickerToPairMap[tickerId].second + "\n";


        unique_lock<mutex> lockLogFile(logFileMutex);
        write(logFileFd, toLog.c_str(), toLog.length());
        lockLogFile.unlock();

        string text = this->table.formatNumber2(last); 
        int rowIndex = this->table.getRowIndex(pair.first); 

        if (pair.second == "C") {
            this->table.drawCell(rowIndex, CALL_LAST_COLUMN, text);
        } else {
            this->table.drawCell(rowIndex, PUT_LAST_COLUMN, text);
        }
    }
}

/**
 * Returns the contract ID of the underlying contract.
 *
 * @return The contract ID of the underlying contract.
 */
int OptionChainManager::getUnderlyingContractId() {
    return this->underlyingContractDetails.contract.conId;
}

/**
 * Finds the strike closest to the given underlying price.
 *
 * This method searches for the closest strike by first finding the lower bound
 * of the underlying price in the set of strikes. If the underlying price is equal
 * to the lower bound, it returns the lower bound. If the lower bound is the first
 * element of the set or if the lower bound is equal to the underlying price, it
 * returns the lower bound. Otherwise, it checks if the lower bound is closer to
 * the underlying price than the next higher strike, and returns the one that is
 * closer.
 *
 * @param underlyingPrice The underlying price to find the closest strike for.
 * @return The strike closest to the given underlying price.
 */
double OptionChainManager::findClosestStrike(double underlyingPrice) {

    set<double>::iterator lowerBound = optionChainManager->strikes.lower_bound(underlyingPrice);

    if(underlyingPrice == *lowerBound) {    //check if equal to strike
        return *lowerBound;
    } else if (lowerBound == optionChainManager->strikes.end()) { //check if higher than all strikes
        return *prev(lowerBound);
    } else if (lowerBound == optionChainManager->strikes.begin()) { //check if lower than all strikes
        return *lowerBound;
    } else {
        double deltaLower = underlyingPrice - *prev(lowerBound); //check if closer to lower or higher
        double deltaHigher = *lowerBound - underlyingPrice;
        
        if(deltaHigher < deltaLower) {
            return *lowerBound;
        } else {
            return *prev(lowerBound);
        }
    }
}

/**
 * Retrieves the bid price for the specified ticker ID.
 *
 * If the ticker ID is 0, this function returns the bid price of the underlying contract.
 * Otherwise, it returns the bid price of the option contract associated with the given 
 * ticker ID. Access to the bid prices is thread-safe to ensure data consistency.
 *
 * @param tickerId The Ticker ID of the contract for which to retrieve the bid price.
 * @return The bid price of the specified contract.
 */
double OptionChainManager::getBid(TickerId tickerId) {

    if(tickerId == 0) {
        lock_guard<mutex> lock_underlying(this->underlyingMutex);
        return this->underlyingBid;
    } else {
        lock_guard<mutex> lock_option(this->optionChain[this->tickerToPairMap[tickerId]]->dataMutex);
        return this->optionChain[this->tickerToPairMap[tickerId]]->bid;
    }
}

/**
 * Retrieves the ask price for the specified ticker ID.
 *
 * If the ticker ID is 0, this function returns the ask price of the underlying contract.
 * Otherwise, it returns the ask price of the option contract associated with the given 
 * ticker ID. Access to the ask prices is thread-safe to ensure data consistency.
 *
 * @param tickerId The Ticker ID of the contract for which to retrieve the ask price.
 * @return The ask price of the specified contract.
 */
double OptionChainManager::getAsk(TickerId tickerId) {

    if(tickerId == 0) {
        lock_guard<mutex> lock_underlying(this->underlyingMutex);
        return this->underlyingAsk;
    } else {
        lock_guard<mutex> lock_option(this->optionChain[this->tickerToPairMap[tickerId]]->dataMutex);
        return this->optionChain[this->tickerToPairMap[tickerId]]->ask;
    }
}

/**
 * Retrieves the last price for the specified ticker ID.
 *
 * If the ticker ID is 0, this function returns the last price of the underlying contract.
 * Otherwise, it returns the last price of the option contract associated with the given
 * ticker ID. Access to the last prices is thread-safe to ensure data consistency.
 *
 * @param tickerId The Ticker ID of the contract for which to retrieve the last price.
 * @return The last price of the specified contract.
 */
double OptionChainManager::getLast(TickerId tickerId) {

    if(tickerId == 0) {
        lock_guard<mutex> lock_underlying(this->underlyingMutex);
        return this->underlyingLast;
    } else {
        lock_guard<mutex> lock_option(this->optionChain[this->tickerToPairMap[tickerId]]->dataMutex);
        return this->optionChain[this->tickerToPairMap[tickerId]]->last;
    }
}

/**
 * Retrieves the contract details for the option with the specified strike and type.
 *
 * @param strike The strike price of the option.
 * @param optionType The type of option, either "C" for call or "P" for put.
 * @return The contract details of the specified option.
 */
Contract OptionChainManager::getContract(double strike, string optionType) {
    return this->optionChain[{strike, optionType}]->contractDetails.contract;
}

/**
 * Retrieves the underlying contract.
 *
 * @return The underlying contract.
 */
Contract OptionChainManager::getUnderlyingContract() {
    return this->underlyingContractDetails.contract;
}

/**
 * Retrieves the option chain.
 *
 * @return The option chain.
 */
map<pair<double, string>, unique_ptr<OptionData>>& OptionChainManager::getOptionChain() {
    return this->optionChain;
}

/**
 * Retrieves a map of active strike prices to their respective row IDs.
 *
 * @return A map of active strike prices to their respective row IDs.
 */
map<double, int> OptionChainManager::getActiveStrikes() {
    return this->table.activeStrikes;
}

/**
 * Converts a pair of strike price and option type to a Ticker ID.
 *
 * This function retrieves the Ticker ID associated with the given pair of strike price 
 * and option type from the internal mapping. The option type is "C" for call 
 * or "P" for put.
 *
 * @param pair The pair consisting of a strike price and an option type.
 * @return The Ticker ID associated with the specified pair.
 */
TickerId OptionChainManager::pairToTicker(pair<double, string> pair) {
    return this->pairToTickerMap[pair];
}
