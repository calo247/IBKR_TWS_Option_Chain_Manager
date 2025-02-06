/*
Tyler Pelling
tpell114@mtroyal.ca
COMP3659
December 6, 2024
*/

#include "my_wrapper.h"
#include "globals.h"

using namespace std;

//public methods

/**
 * Constructs a My_wrapper object.
 *
 * This constructor will create a new EClientSocket and link it to an
 * EReaderOSSignal. It will also initialize all required member variables.
 *
 * @see EClientSocket
 * @see EReaderOSSignal
 */
My_wrapper::My_wrapper() :
    m_pClientSocket(new EClientSocket(this, &m_osSignal)),
	m_pReader(nullptr),
	m_sleepDeadline(0),
	m_currentReqId(1),
	m_currentTickerId(1), 
	maxThreads(getMaxThreads())
{}

/**
 * Destroys a My_wrapper object.
 *
 * This destructor will delete the EClientSocket and EReaderOSSignal objects
 * that were created in the constructor. It will also set the pointers to
 * nullptr to prevent accidental reuse.
 *
 * @see EClientSocket
 * @see EReaderOSSignal
 */
My_wrapper::~My_wrapper() {
    if(m_pReader){
		delete m_pReader;
		m_pReader = nullptr;
	} 
	delete m_pClientSocket;
	m_pClientSocket = nullptr;
}

/**
 * Cancels all market data requests for both the underlying contract and all option contracts
 * managed by the optionChainManager. This function iterates over the option chain and cancels
 * market data using the ticker ID associated with each option.
 */
void My_wrapper::cancelMarketData() {

	const map<pair<double, string>, unique_ptr<OptionData>>& optionChain = optionChainManager->getOptionChain();

	m_pClientSocket->cancelMktData(0);

	for(const auto& pair : optionChain) {
		m_pClientSocket->cancelMktData(optionChainManager->pairToTicker(pair.first));
	}

	string toLog = "Cancelled all market data requests\n";
	unique_lock<mutex> lockLogFile(logFileMutex);
	write(logFileFd, toLog.c_str(), toLog.length());
	lockLogFile.unlock();
}

/**
 * Disconnects from the TWS server.
 *
 * This function will disconnect the current connection to the TWS server. If
 * the connection is successful, the function will return true. Otherwise, it
 * will return false.
 *
 * @return true if the disconnection was successful, false otherwise
 */
void My_wrapper::disconnect() {
	
	m_pClientSocket->eDisconnect();

	string toLog = "Disconnected\n";

	unique_lock<mutex> lockLogFile(logFileMutex);
	write(logFileFd, toLog.c_str(), toLog.length());
	lockLogFile.unlock();
}

/**
 * Processes incoming messages from the TWS server.
 *
 * This function waits for a signal indicating that there are messages to be 
 * processed, and then processes the messages using the EReader associated 
 * with the client socket. It ensures that all messages are handled 
 * appropriately, facilitating communication between the client and the 
 * TWS server.
 * 
 * @note This function processes messages using a single thread.
 */
void My_wrapper::processMessages() {
	m_osSignal.waitForSignal();
	m_pReader->processMsgs();
}

/**
 * Processes incoming messages from the TWS server using multiple threads.
 *
 * This function waits for a signal indicating that there are messages to be 
 * processed, and then processes the messages from the EReader message queue. 
 * Each thread processes messages until the queue is empty or the stopProcessingFlag 
 * is set to true.
 * 
 * @note This function processes messages using multiple threads, up to the
 * maximum number of threads available on the platform. 
 * @note The maximum number of threads are found in
 * the constructor for My_wrapper.
 */
void My_wrapper::processMessagesMultithreaded() {

    stopProcessingFlag = false;
    deque<shared_ptr<EMessage>>& m_msgQueue = m_pReader->getMsgQueue();
    EDecoder& processMsgsDecoder = m_pReader->getProcessMsgsDecoder();
    EMutex& messageQueueMutex = m_pReader->getMsgQueueMutex();

    auto worker = [&]() {	//worker lambda function
        while (true) {
			
            shared_ptr<EMessage> message;

            m_osSignal.waitForSignal();

            if (stopProcessingFlag) break;

            {
                EMutexGuard lock(messageQueueMutex);
                if (!m_msgQueue.empty()) {
                    message = m_msgQueue.front();
                    m_msgQueue.pop_front();
                }
            }

            if (message) {
                const char* pBegin = message->begin();
                while (processMsgsDecoder.parseAndProcessMsg(pBegin, message->end()) > 0) {
                    {
                        EMutexGuard lock(messageQueueMutex);
                        if (m_msgQueue.empty()) break;
                        message = m_msgQueue.front();
                        m_msgQueue.pop_front();
                    }
                    pBegin = message->begin();
                }
            }
        }
    };

    vector<thread> threads;		//make thread pool
    for (int i = 0; i < maxThreads; ++i) {
        threads.emplace_back(worker);
    }

	while(getch() != 'q');

	stopProcessingFlag = true;
    m_osSignal.issueSignalAllThreads(); // wake up all threads

    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

/**
 * Requests contract details for a given contract.
 *
 * This function generates a unique request ID and logs the details of the request.
 * It then sends a request to retrieve contract details via the client socket.
 * 
 * The callback for this request is handled in the `contractDetails` function.
 *
 * @param contract The contract for which details are being requested. It includes information
 * such as the symbol, strike, and right.
 */
void My_wrapper::requestContractDetails(const Contract& contract) {

	int reqId = getNextReqId();

	string toLog = "ReqID: " + to_string(reqId) + " - Requesting contract details for " + contract.symbol + " Strike " + to_string(contract.strike) + " Right: " + contract.right + "\n";
	unique_lock<mutex> lockLogFile(logFileMutex);
	write(logFileFd, toLog.c_str(), toLog.length());
	lockLogFile.unlock();

    m_pClientSocket->reqContractDetails(reqId, contract);
}

/**
 * Requests an option chain for the given underlying contract.
 *
 * This function first requests the details of the underlying contract and then waits for the callback
 * to be processed. Once the underlying contract details are known, it requests the option chain associated
 * with that contract.
 *
 * @param underlyingSymbol The symbol of the underlying contract, e.g. "ES"
 * @param futFopExchange The exchange on which the underlying contract is traded, e.g. "GLOBEX"
 * @param underlyingSecurityType The type of underlying security, e.g. "FUT"
 * @param currency The currency in which the underlying contract is traded, e.g. "USD"
 * @param contractDate The last trade date/contract month for the underlying contract, e.g. "202003"
 */
void My_wrapper::requestOptionChain(string underlyingSymbol, string futFopExchange, string underlyingSecurityType,
		 							string currency, string contractDate) {
    
	
	optionChainManager->setUnderlyingContract(underlyingSymbol, futFopExchange, underlyingSecurityType,
											  currency, contractDate);

	requestContractDetails(optionChainManager->getUnderlyingContract());
	processMessages();
	
	int reqId = getNextReqId();
	string toLog = "ReqID: " + to_string(reqId) + " - Requesting option chain for: " + to_string(optionChainManager->getUnderlyingContractId()) + "\n";
	unique_lock<mutex> lockLogFile(logFileMutex);
	write(logFileFd, toLog.c_str(), toLog.length());
	lockLogFile.unlock();
	
    m_pClientSocket->reqSecDefOptParams(reqId, underlyingSymbol, futFopExchange, underlyingSecurityType, 
										optionChainManager->getUnderlyingContractId());
}

/**
 * Requests that market data type be switched to delayed data type.
 *
 * This function sends a request to TWS to switch the market data type to delayed data type.
 * It logs a message to the log file indicating that such a request has been made.
 * 
 * @note The callback for this function is handled in the `marketDataType` function.
 */
void My_wrapper::requestDelayedDataType() {

	string toLog = "Requesting delayed data type\n";
	unique_lock<mutex> lockLogFile(logFileMutex);
	write(logFileFd, toLog.c_str(), toLog.length());
	lockLogFile.unlock();

	m_pClientSocket->reqMarketDataType(DELAYED_DATA_TYPE);
}

/**
 * Requests market data for the underlying contract.
 *
 * This function sends a request to TWS for market data on the underlying contract.
 * The market data type is set to delayed data type. The request is logged to the
 * log file.
 */
void My_wrapper::requestUnderlyingMarketData() {
	m_pClientSocket->reqMarketDataType(DELAYED_DATA_TYPE);
	m_pClientSocket->reqMktData(0, optionChainManager->getUnderlyingContract(), "", false, false, TagValueListSPtr());
}

/**
 * Requests market data for all active strikes in the option chain.
 *
 * This function iterates over the active strikes in the option chain and requests
 * market data for both calls and puts associated with each strike. The market data
 * type is set to delayed data type. The requests are logged to the log file.
 */
void My_wrapper::requestMarketData() {

	const map<pair<double, string>, unique_ptr<OptionData>>& optionChain = optionChainManager->getOptionChain();
	const map<double, int> activeStrikes = optionChainManager->getActiveStrikes();
	
	for(const auto& pair : activeStrikes) {
		
		//Request calls
		TickerId callTickerId = optionChainManager->pairToTicker(make_pair(pair.first, "C"));

		m_pClientSocket->reqMktData(callTickerId, optionChain.find(make_pair(pair.first, "C"))->second->contractDetails.contract,
									"", false, false, TagValueListSPtr());
		
		string toLog = "ReqID: " + to_string(callTickerId) + " - Requesting market data for " 
					   + to_string(optionChain.find(make_pair(pair.first, "C"))->second->contractDetails.contract.conId) + "\n";

		unique_lock<mutex> lockLogFile(logFileMutex);
		write(logFileFd, toLog.c_str(), toLog.length());
		lockLogFile.unlock();


		//Request puts
		TickerId putTickerId = optionChainManager->pairToTicker(make_pair(pair.first, "P"));
		
		m_pClientSocket->reqMktData(putTickerId, optionChain.find(make_pair(pair.first, "P"))->second->contractDetails.contract,
									"", false, false, TagValueListSPtr());
	
		toLog = "ReqID: " + to_string(putTickerId) + " - Requesting market data for " 
				+ to_string(optionChain.find(make_pair(pair.first, "P"))->second->contractDetails.contract.conId) + "\n";
		
		lockLogFile.lock();
		write(logFileFd, toLog.c_str(), toLog.length());
		lockLogFile.unlock();
	}
}

/**
 * Establishes a connection to TWS using the provided host, port, and client ID.
 * If the connection is successful, an EReader object is created and started to
 * manage incoming messages from the server. The function returns a boolean value
 * indicating the success of the connection.
 *
 * @param host The hostname or IP address of the TWS server
 * @param port The port number to connect to
 * @param clientId The client ID to use for the connection
 * @return true if the connection was successful, false otherwise
 */
bool My_wrapper::connect(const char *host, int port, int clientId) {

	string toLog = "Connecting to " + string(host) + ":" + to_string(port) + " clientId:" + to_string(clientId) + "\n";
	unique_lock<mutex> lockLogFile(logFileMutex);
	write(logFileFd, toLog.c_str(), toLog.length());
	lockLogFile.unlock();
	
	bool connection_status = m_pClientSocket->eConnect(host, port, clientId);
	
	if (connection_status) {
		toLog = "Connected to " + string(host) + ":" + to_string(port) + " clientId:" + to_string(clientId) + "\n";
		lockLogFile.lock();
		write(logFileFd, toLog.c_str(), toLog.length());
		lockLogFile.unlock();

		m_pReader = new EReader(m_pClientSocket, &m_osSignal);
		m_pReader->start();
	}
	else
		toLog = "Cannot connect to " + string(host) + ":" + to_string(port) + " clientId:" + to_string(clientId) + "\n";
		lockLogFile.lock();
		write(logFileFd, toLog.c_str(), toLog.length());
		lockLogFile.unlock();

	return connection_status;
}

/**
 * Generates a unique request ID that can be used for requests to the TWS server.
 *
 * This function is thread-safe and uses a mutex to ensure that only one request ID
 * is generated at a time. The request ID is incremented after each call to this
 * function.
 *
 * @return a unique request ID.
 */
int My_wrapper::getNextReqId() {
	lock_guard<mutex> lock(m_reqIdMutex);
	int reqId = m_currentReqId;
	m_currentReqId++;
	return reqId;
}

/**
 * Generates a unique ticker ID for market data requests.
 *
 * This function is thread-safe and uses a mutex to ensure that only one ticker ID
 * is generated at a time. The ticker ID is incremented after each call to this
 * function to maintain uniqueness.
 *
 * @return a unique ticker ID.
 */
TickerId My_wrapper::getNextTickerId() {
	lock_guard<mutex> lock(m_tickerIdMutex);
	int tickerId = m_currentTickerId;
	m_currentTickerId++;
	return tickerId;
}

//interface overrides

/**
 * Processes the contract details received from the server.
 *
 * This function is invoked as a callback to a request made by the `requestContractDetails` function. 
 * It updates the optionChainMangager with the contract details.
 *
 * @param reqId The unique request identifier associated with the contract details.
 * @param contractDetails The full details of the contract received, including contract ID, symbol, 
 * trading class, strike price, right, etc.
 */
void My_wrapper::contractDetails(int reqId, const ContractDetails& contractDetails) {

	if(contractDetails.contract.tradingClass == selectedSymbol) {

		string toLog = "ReqID: " + to_string(reqId) + " - Received contract details for " + contractDetails.contract.symbol 
						+ ", Contract ID: " + to_string(contractDetails.contract.conId) + " Trading Class: " + contractDetails.contract.tradingClass 
						+ " Strike: " + to_string(contractDetails.contract.strike) + " Right: " + contractDetails.contract.right
						+ " Last Trade Date: " + contractDetails.contract.lastTradeDateOrContractMonth + "\n";

		unique_lock<mutex> lockLogFile(logFileMutex);
		write(logFileFd, toLog.c_str(), toLog.length());
		lockLogFile.unlock();

		if(contractDetails.contract.secType == FUTURES_CODE){
			optionChainManager->setUnderlyingContractDetails(contractDetails);

		} else if(contractDetails.contract.secType == FUTURES_OPTION_CODE){
			optionChainManager->setContractDetails(contractDetails);
			optionChainManager->incrementInitCallbackCount();
		}	
	}
}

/**
 * Handles error messages by logging them to a file.
 *
 * This callback function is invoked when an error occurs. It logs the error details, 
 * including the identifier, error code, error message to a log file.
 * 
 * Also used to return startup information (e.g. TWS version, datafarm connections, etc.)
 *
 * @param id The identifier associated with the error.
 * @param errorCode The code representing the specific error.
 * @param errorString A descriptive message of the error.
 * @param advancedOrderRejectJson Not used for this implementation
 */
void My_wrapper::error(int id, int errorCode, const string& errorString, const string& advancedOrderRejectJson) {
	
	string toLog = "Error. Id: " + to_string(id) + ", Code: " + to_string(errorCode) + ", Msg: " + errorString + "\n";
	
	unique_lock<mutex> lockLogFile(logFileMutex);
	write(logFileFd, toLog.c_str(), toLog.length());
	lockLogFile.unlock();    
}

/**
 * Handles the market data type response from the server.
 *
 * This function is a callback invoked when the server responds to a request made by the
 * `requestDelayedDataType` function. It logs a message indicating the market data type
 * associated with the request ID.
 *
 * @param reqId The unique request identifier associated with the market data type.
 * @param marketDataType The market data type associated with the request ID.
 */
void My_wrapper::marketDataType(TickerId reqId, int marketDataType) {

	string toLog = "MarketDataType. ReqId: " + to_string(reqId) + ", Type: " + to_string(marketDataType) + "\n";
	unique_lock<mutex> lockLogFile(logFileMutex);
	write(logFileFd, toLog.c_str(), toLog.length());
	lockLogFile.unlock();
}

/**
 * Handles the security definition optional parameter response from the server.
 *
 * This function is a callback invoked when the server responds to a request made by the
 * `requestOptionChain` function. It logs a message indicating the receipt of the option chain
 * for the underlying contract and initializes the option chain manager with the strikes
 * received.
 *
 * @param reqId The unique request identifier associated with the option chain.
 * @param exchange The exchange on which the underlying contract is traded.
 * @param underlyingConId The contract ID of the underlying contract.
 * @param tradingClass The trading class of the underlying contract.
 * @param multiplier The multiplier for the underlying contract.
 * @param expirations The set of expiration dates for the option chain.
 * @param strikes The set of strikes for the option chain.
 */
void My_wrapper::securityDefinitionOptionalParameter(int reqId, const string& exchange, int underlyingConId, 
													const string& tradingClass, const string& multiplier, 
													const set<string>& expirations, const set<double>& strikes) {

	if(tradingClass == selectedSymbol) {

		string toLog = "ReqID: " + to_string(reqId) + " - Received option chain for " + to_string(underlyingConId) + "\n";
		unique_lock<mutex> lockLogFile(logFileMutex);
		write(logFileFd, toLog.c_str(), toLog.length());
		lockLogFile.unlock();

		optionChainManager->initializeChain(strikes);
	}
}

/**
 * Handles real-time price updates for a given option contract.
 *
 * This function is a callback invoked by TWS when it receives a price update
 * for an option contract. It logs the received price and updates the
 * prices of the bid, ask, and last fields in the option chain manager.
 *
 * @param tickerId The unique identifier associated with the option contract.
 * @param field The type of price update (bid, ask, or last).
 * @param price The updated price.
 * @param attrib The set of attributes associated with the price update.
 */
void My_wrapper::tickPrice(TickerId tickerId, TickType field, double price, const TickAttrib& attrib) {

	string toLog = "Tick Price. Ticker Id: " + to_string(tickerId) + ", Field: " + to_string(field)
					+ ", Price: " + to_string(price) + "\n";

	unique_lock<mutex> lockLogFile(logFileMutex);
	write(logFileFd, toLog.c_str(), toLog.length());
	lockLogFile.unlock();
	
	switch (field){

	case DELAYED_BID:
		optionChainManager->updateBid(tickerId, price);
		break;
	
	case DELAYED_ASK:
		optionChainManager->updateAsk(tickerId, price);
		break;

	case DELAYED_LAST:
		optionChainManager->updateLast(tickerId, price);
		break;
	}
}

//private methods

/**
 * Retrieves the maximum number of threads that can be supported by the hardware.
 *
 * This function utilizes the `thread::hardware_concurrency()` method to determine
 * the number of concurrent threads supported by the hardware. It logs the number
 * of threads to the standard output and returns it.
 *
 * @return The maximum number of threads that can be supported by the hardware.
 */
unsigned int My_wrapper::getMaxThreads() {

	unsigned int maxThreads = thread::hardware_concurrency();

	string toLog = "Using max threads: " + to_string(maxThreads) + "\n";
	write(STDOUT_FILENO, toLog.c_str(), toLog.length());

	return maxThreads;
}
