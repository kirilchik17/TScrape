#include <td/telegram/Client.h>
//#include <random>
#include <ctime>
#include <stdio.h>
#include <map>
#include <queue>
#include <thread>
#include <atomic>
#include <set>
#include <type_traits>
using namespace td;
using namespace td::td_api;
using namespace std;
#define DEFAULT_MAX_REQ 10


//TODO: One thread will constantly run to check if there are responses;
class UserClientManager {
private:
	std::atomic<bool> emptyRequests = true;
	std::shared_ptr<Client> client;
	std::map<int, shared_ptr<Client::Request>> currentRequest;
	std::map<int, shared_ptr<Client::Response>> responses;
	std::queue<shared_ptr<Client::Request>> queuedRequests;
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
	//TODO: might need to add mutex for queing and inserts
	auto registerRequest(std::shared_ptr<Client::Request> req) {
		emptyRequests = true;
		if (currentRequest.size() == maxConcurrentRequests) {
			queuedRequests.push(req);
			queuedRequestsIds.insert(req->id);
		} else {
			currentRequest[req->id] = req;
		}
	}
	auto waitForResponse(int respId) {
		while (true) {
			if (responses.count(respId)) {
				return responses[respId];
			}
		}
	}
	void addResponse(const Client::Response& resp) {
		if (resp.id) {
			responses[resp.id] = make_shared<Client::Response>(resp);
		}
		else {
			//TODO: Handle update
		}
	}
	auto listenToClient() {
		while (true) {
			while (!emptyRequests) {
				//Find a way to make it without timeout, or check if there is actually a timeout
				auto resp = client->receive(10);
				addResponse(resp);
				currentRequest.erase(resp.id);
				//Adding next request
				if (!queuedRequests.empty()) {
					auto req = queuedRequests.front();
					currentRequest.insert(make_pair(req->id, req));
					//Erasing from queue
					queuedRequestsIds.erase(req->id);
					queuedRequests.pop();
				}
				if (queuedRequests.empty() && currentRequest.empty())
					emptyRequests = true;
			}
		}
	}

public:
	//Write a function that will initialize a client
	UserClientManager(std::shared_ptr<Client> client, int maxConcurrentRequests = DEFAULT_MAX_REQ):
		client(move(client)), maxConcurrentRequests(maxConcurrentRequests) 
	{
		currentRequest = std::map<int, shared_ptr<Client::Request>>();
		responses = std::map<int, shared_ptr<Client::Response>>();
		queuedRequests = std::queue<shared_ptr<Client::Request>>();
		queuedRequestsIds = std::set<int>();
		std::thread updateListener(listenToClient);
	}

	object_ptr<Object> send(object_ptr<Function> tdFunc) {
		auto req = createRequest(std::move(tdFunc));
		registerRequest(req);
		auto response = waitForResponse(req->id);
		responses.erase(response->id);
		return move(response->object);
	}

	template <typename Callback, typename... Args>
	auto sendWithCallback(object_ptr<Function> tdFunc, Callback&& callback, Args&&... args) {
		auto req = createRequest(std::move(tdFunc));
		registerRequest(req);
		auto response = waitForResponse(req->id);
		responses.erase(response->id);
		return std::invoke(std::forward<Callback>(callback),
			move(response->object),
			std::forward<Args>(args)...);
	}
	
};