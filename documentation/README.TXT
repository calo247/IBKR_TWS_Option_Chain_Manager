Trading times for CME E-Mini futures:

    Sunday to Friday: Trading begins at 4:00 p.m. MT and continues until 3:00 p.m. MT the following day.

    Daily Maintenance Break: There is a pause in trading each day from 3:00 p.m. to 4:00 p.m. MT.

    Broker Maintenance: Interactive Brokers performs intermittent maintenance on their servers. During this time,
        program behaviour may be unstable and unpredictable. This maintenance period is typically between 9:00 pm 
        and 1:00 am.

    Note: Market data for this application is delayed by up to 30 minutes, so adjust times above accordingly


Bugs and limitations:

- Only a subset of contracts and expirations are supported currently. The "20241220" expiration will not work after
  the date as named. 

- The next expiry that will be supported is "20250620". This can be attempted to be added to the supportedExpiries set 
  in the terminal.cpp file. However, this expiry is not currently supported because of the following bug.

- The program does not verify the number of strikes available for a contract. Because of this if the number of strikes 
  available is fewer than the number of rows in the table, the program will crash. This is because the table tries to 
  draw the next strike even if there are no more strikes available.

- There is currently no way to adjust which strikes are displayed. On startup, the program will display the closest strikes 
  to the current underlying price at the time.

- Each cell is limited to 5 digits to the left of the decimal and 2 digits to the right of the decimal. This is acceptable 
  for North American markets, but may not be for other markets.

- GLIBCXX_3.4.32 support is required for compilation.


Other notes:

- A price of 0.00 or -1.00 will be displayed if there is currently no data available for that data point. More specifically,
  a price of -1.00 indicates no data, and a price of 0.00 indicates that a combo is available. But for the scope of this 
  project, both values effectively indicate no data.

- Tested with Interactive Brokers TWS offline build 10.30.1r, Nov 19, 2024.