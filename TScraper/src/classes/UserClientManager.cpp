#include <td/telegram/Client.h>
//#include <random>
#include <ctime>
#include <stdio.h>
#include <map>
#include <queue>
#include <set>
using namespace td;
using namespace td::td_api;
using namespace std;
#define DEFAULT_MAX_REQ 10


//TODO: One thread will constantly run to check if there are ressponses;
class UserClientManager {
private:
	std::shared_ptr<Client> client;
	
	//need to know the: id - the callback function -- maybe the send will return the response object
	std::map<int, shared_ptr<Client::Request>> currentRequest;
	//add a set for call back functions
	std::queue< shared_ptr<Client::Request>> queuedRequests;
	std::set<int> queuedRequestsIds;
	int maxConcurrentRequests;

	int getRandomId() {
		int id = 0;
		do {
			std::srand(time(0));
			int id = rand() % 1000000;
		} while (currentRequest.count(id) > 0 || queuedRequestsIds.count(id) > 0 || !id);
		return id;
	}
	shared_ptr<Client::Request> createRequest(object_ptr<Function> tdFunc) {
		int reqId = getRandomId();
		auto req = make_shared<Client::Request>();
		req->function = std::move(tdFunc);
		req->id = reqId;
		return req;
	}
	//might need to add mutex for queing and inserts
	//TODO: Add the ability to add callback functions
	auto registerRequest(std::shared_ptr<Client::Request> req) {
		if (currentRequest.size() == maxConcurrentRequests) {
			queuedRequests.push(req);
			queuedRequestsIds.insert(req->id);
		} else {
			currentRequest[req->id] = req;
		}
		//Add insert to callback functions
	}

	auto listenToClient() {
		//TODO: Check the emptieness outside to not waste calculations
		bool empty = true;
		while (true) {
			while (!empty) {
				//Find a way to make it without timeout, or check if there is actually a timeout
				auto resp = client->receive(10);
				// add response to different function
				if (resp.id == 0) {
					//id = 0 is reserved for updates
					//TODO: Add update handlers
				}
				//Check in which cases it will occur but ussually shouldn't happen
				if (currentRequest[resp.id] != nullptr) {
					currentRequest.erase(resp.id);
					//Check if request has callback 

				}
				
			}
			empty = currentRequest.empty() && queuedRequests.empty();
		}
	}

public:
	UserClientManager(std::shared_ptr<Client> client, int maxConcurrentRequests = DEFAULT_MAX_REQ):
		client(move(client)), maxConcurrentRequests(maxConcurrentRequests) 
	{
		currentRequest = std::map<int, shared_ptr<Client::Request>>();
		queuedRequests = std::queue<shared_ptr<Client::Request>>();
		queuedRequestsIds = std::set<int>();
	}
	//Maybe should have a return type so it will be a object
	//Or maybe have send and sendAsync in order to prevent confusion
	bool send(object_ptr<Function> tdFunc) {
		auto req = createRequest(std::move(tdFunc));
		if (currentRequest.size() == maxConcurrentRequests) {
			queuedRequests.push(req);
			queuedRequestsIds.insert(req->id);
			//wait for response
		}
		concu
	}

	
};