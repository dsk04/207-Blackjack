The code can be compiled by using make, and make clean can be used to remove the executables. Since compilation is so quick, I did not see the need for client/server targets only.

To run the server, merely type ./masterserver . Nothing will in the server window except error messages, most of which are not fatal. However, the server does log various statistics, albeit probably not in the most efficient way.

To run a client, type ./client 127.0.0.1 5000 . that will open a client, which immediately puts you into an interactive hand of blackjack with the server. The client will then be requested to type "1" or "2" to decide whether they hit or stand. If they type anything else, they will be returned to the same prompt. If they hit, they will receive a new card, while if they stand, they will stay with the cards they have. Once they finish, the dealer will do the same. Whichever player has the higher value is the victor.

Another thing that can be done with the client is to run multiple processes with randomized inputs as a form of stress testing. To do that, simply add a number after the port number 5000, and the client will attempt to create that number of processes. Note that, as of now, those processes do not necessarily elegantly.
