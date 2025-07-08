- advantages of fetch:
  i. https handling, dont require writing manual ssl code
  ii. handling redirects, benefits in saving time, as receiving rsponse then parsing it then again requesting for the given location is time consuming and overhead
  iii. handling compressed data
  iv. what it does: - http requesting/response parsing - ssl tunneling -

- advantages of dynamic buffer
  i. can receive as much data as given

- advantages of multi threading
  i. other clients wont need to wait for one to complete, server efficiently serves to clients

what i have yet:

- http request/response parser
- caching using doubly linked list + disk storage
- creating connection to remote server
- creating tcp server
- sending fixed size messages via http/https
- receiving fixed size messages via http/https
- getting blocked sites list from file
- getting cached entries from cached directory

where current code breaks:

- fixed size buffer not capable of getting full responses many times ✅(fixed)
- fails to get binary data from cache file
- fails to rewrite html according to our proxy
- code became messy , not clearly broke in terms of solving individual problems like redirection, handling https connection and data transfer, serving cached response, serving fetched response ✅(introduced new design)
- multithreaded code causes the application in hang state, currentl ssl code fails in multithreading environment,


now current scenario:
- can receive request from client
- can forward request to remote server
- can cache response for future use
- can respond from cache

what to build next
- rewrite html for proxy, injecting base tag and modifying links
- removing ads tags
- blocking website ✅

new challange:
- 1e10 is added at the html for no reason ✅ (it is the part of the data)
- trailing / in url causes cache not found ✅(solved by sanitizing the url properly)