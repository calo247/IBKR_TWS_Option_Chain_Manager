/*
Tyler Pelling
tpell114@mtroyal.ca
COMP3659
December 6, 2024
*/

#ifndef MY_WRAPPER_H
#define MY_WRAPPER_H

#include "DefaultEWrapper.h"
#include "EReaderOSSignal.h"
#include "EReader.h"
#include "EClientSocket.h"
#include "Contract.h"
#include "EMessage.h"
#include "terminal.h"
#include <thread>

using namespace std;

class My_wrapper : public DefaultEWrapper {
private:

	EReaderOSSignal m_osSignal;
	EClientSocket* m_pClientSocket;
	time_t m_sleepDeadline;
	EReader* m_pReader;
	string m_bboExchange;
	mutex m_reqIdMutex;
	int m_currentReqId;
	mutex m_tickerIdMutex;
	TickerId m_currentTickerId;
	atomic<bool> stopProcessingFlag;
	unsigned int maxThreads;

	unsigned int getMaxThreads();

public:

    My_wrapper();
    ~My_wrapper();

	void cancelMarketData();
	void disconnect();
	void processMessages();
	void processMessagesMultithreaded();
	void requestContractDetails(const Contract& contract);
	void requestOptionChain(string underlyingSymbol, string futFopExchange, string underlyingSecurityType,
		 					string currency, string contractDate);
	void requestDelayedDataType();
	void requestUnderlyingMarketData();
	void requestMarketData();
	bool connect(const char * host, int port, int clientId = 0);
	int getNextReqId();
	TickerId getNextTickerId();

	//overrides
	void contractDetails(int reqId, const ContractDetails& contractDetails) override;
	void error(int id, int errorCode, const string& errorString, const string& advancedOrderRejectJson) override;
	void marketDataType(TickerId reqId, int marketDataType) override;	
	void securityDefinitionOptionalParameter(int reqId, const string& exchange, int underlyingConId, 
											const string& tradingClass, const string& multiplier, 
											const set<string>& expirations, const set<double>& strikes) override;
	void tickPrice(TickerId tickerId, TickType field, double price, const TickAttrib& attrib) override;

};

#endif
