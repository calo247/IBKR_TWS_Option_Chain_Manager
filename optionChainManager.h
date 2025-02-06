/*
Tyler Pelling
tpell114@mtroyal.ca
COMP3659
December 6, 2024
*/

#ifndef OPTION_CHAIN_MANAGER_H
#define OPTION_CHAIN_MANAGER_H

#include <map>
#include <mutex>
#include <set>
#include "Contract.h"
#include "table.h"

using namespace std;

typedef struct {
    double bid;         
    double ask;         
    double last;
    mutex dataMutex;
    ContractDetails contractDetails;
    TickerId tickerId;
} OptionData;

class OptionChainManager {

private:

    int contractCount;
    int initCallbackCount;
    double underlyingBid;
    double underlyingAsk;
    double underlyingLast;
    map<pair<double, string>, unique_ptr<OptionData>> optionChain;
    map<TickerId, pair<double, string>> tickerToPairMap;
    map<pair<double, string>, TickerId> pairToTickerMap;
    set<double> strikes;
    Table table;
    ContractDetails underlyingContractDetails;
    mutex underlyingMutex;
    TickerId underlyingTickerId;
    
public:

    bool isInitialized = false;

    void incrementInitCallbackCount();
    void initializeChain(const set<double>& strikes);
    void setUnderlyingContractDetails(ContractDetails contractDetails);
    void setContractDetails(ContractDetails contractDetails);
    void setUnderlyingContract(string underlyingSymbol, string futFopExchange, string underlyingSecurityType,
		 					   string currency, string contractDate);
    void updateBid(TickerId tickerId, double bid);
    void updateAsk(TickerId tickerId, double ask);
    void updateLast(TickerId tickerId, double last);
    int getUnderlyingContractId();
    double findClosestStrike(double underlyingPrice);
    double getBid(TickerId tickerId);
    double getAsk(TickerId tickerId);
    double getLast(TickerId tickerId);  
    Contract getContract(double strike, string optionType);
    Contract getUnderlyingContract();
    map<pair<double, string>, unique_ptr<OptionData>>& getOptionChain();
    map<double, int> getActiveStrikes();
    TickerId pairToTicker(pair<double, string> pair);
};

#endif