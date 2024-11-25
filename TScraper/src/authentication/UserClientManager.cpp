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
#include <iostream>
#include <mutex>
#include "AuthState.cpp"
#define DEFAULT_MAX_REQ 10


class UserClientManager {
private:
	std::atomic<bool> emptyRequests = true;
	bool isAuthorized = false;
	std::mutex registrationMutex;
	std::unique_ptr<td::Client> client;
	std::map<int, std::shared_ptr<td::Client::Request>> currentRequest;
	std::map<int, std::shared_ptr<td::Client::Response>> responses;
	std::queue<std::shared_ptr<td::Client::Request>> queuedRequests;
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
	std::shared_ptr<td::Client::Request> createRequest(td::td_api::object_ptr<td::td_api::Function> tdFunc) {
		int reqId = getRandomId();
		auto req = std::make_shared<td::Client::Request>();
		req->function = std::move(tdFunc);
		req->id = reqId;
		return req;
	}
	//TODO: might need to add mutex for queing and inserts
	auto registerRequest(std::shared_ptr<td::Client::Request> req) {
		emptyRequests = true;
		std::lock_guard<std::mutex> lock(registrationMutex);
		if (currentRequest.size() == maxConcurrentRequests) {
			queuedRequests.push(req);
			queuedRequestsIds.insert(req->id);
		} else {
			currentRequest[req->id] = req;
		}
	}
	auto waitForResponse(const int& respId) {
		while (true) {
			if (responses.count(respId)) {
				return responses[respId];
			}
		}
	}
	void addResponse(const td::Client::Response& resp) {
		if (resp.id) {
			responses[resp.id] = std::make_shared<td::Client::Response>(resp);
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
				//id specific so no need for mutex
				currentRequest.erase(resp.id);
				std::lock_guard<std::mutex> lock(registrationMutex);
				if (!queuedRequests.empty()) {
					auto req = queuedRequests.front();
					currentRequest.insert(make_pair(req->id, req));
					queuedRequestsIds.erase(req->id);
					queuedRequests.pop();
				}
				if (queuedRequests.empty() && currentRequest.empty())
					emptyRequests = true;
			}
		}
	}

	UserClientManager(std::unique_ptr<td::Client> client, const int& maxConcurrentRequests = DEFAULT_MAX_REQ) :
		client(move(client)), maxConcurrentRequests(maxConcurrentRequests)
	{
		currentRequest = std::map<int, std::shared_ptr<td::Client::Request>>();
		responses = std::map<int, std::shared_ptr<td::Client::Response>>();
		queuedRequests = std::queue<std::shared_ptr<td::Client::Request>>();
		queuedRequestsIds = std::set<int>();
		std::thread updateListener(listenToClient);
	}

	static td::td_api::object_ptr<td::td_api::setTdlibParameters> getConfigClientParameters(const std::string sessionDirectory = "") {
		//TODO: Use .env for the params 
		td::td_api::object_ptr<td::td_api::setTdlibParameters> parameters
			= td::td_api::make_object<td::td_api::setTdlibParameters>();
		parameters->database_directory_ = sessionDirectory == "" ? "sessions" : sessionDirectory; // TODO: Change to db dir path
		parameters->files_directory_ = (sessionDirectory == "" ? "sessions" : sessionDirectory) + "/tdlib_files";// TODO: Change to files path
		parameters->use_test_dc_ = false;
		parameters->api_id_ = 123456;  // TODO: Use API id
		parameters->api_hash_ = "your_api_hash"; // TODO: Use API hash
		parameters->system_language_code_ = "en";
		parameters->device_model_ = "Desktop";
		parameters->system_version_ = "1.0";//write our version
		parameters->application_version_ = "1.0";
		return parameters;
	}
	auto proccessAuthUpdate(td::td_api::object_ptr<td::td_api::Object> response) {
		if (response->get_id() == td::td_api::updateAuthorizationState::ID) {
			auto authStateObj = td::move_tl_object_as<td::td_api::updateAuthorizationState>(response).get();
			auto stateId = authStateObj->authorization_state_->get_id();
			switch (stateId) {
			case td::td_api::authorizationStateWaitTdlibParameters::ID:
				// Already sent TDLib parameters
				break;
			case td::td_api::authorizationStateWaitPhoneNumber::ID:
				std::cout << "Waiting for user {userId} phone number" << std::endl;
				return AuthState::PhoneRequired;

			case td::td_api::authorizationStateWaitCode::ID:
				std::cout << "Waiting for user {userId} auth code" << std::endl;
				return AuthState::AuthCodeRequired;

			case td::td_api::authorizationStateReady::ID:
				std::cout << "Authentication successful!" << std::endl;
				return AuthState::Successful;

			case td::td_api::authorizationStateClosed::ID:
				std::cout << "TDLib client closed." << std::endl;
				return AuthState::Closed;

			default:
				std::cerr << "Unexpected authorization state: " << stateId << std::endl;
				return AuthState::Unknown;
			}
		}
	}
public:
	static std::shared_ptr<UserClientManager> initiateClientFromSession(const std::string& sessionPath) {
		auto client = std::make_unique<td::Client>();
		std::cout << "TDLib client initialized." << std::endl;
		auto manager = std::make_shared<UserClientManager>(move(client));
		auto params = getConfigClientParameters(sessionPath);
		auto paramsResponse = manager->send(std::move(params));
		auto clientState = manager->proccessAuthUpdate(std::move(paramsResponse));
		if (clientState == Successful) {
			std::cout << "Successful client initialization from session." << std::endl;
			return manager;
		}
		else {
			std::cerr << "Failed to initialize client form session" << std::endl;
			return nullptr;
		}
	}

	//TODO: the docs say that the session continuity is automatic so there is no need to do anything
	static std::shared_ptr<UserClientManager> initiateClient(const std::string& phoneNumber) {
		auto client = std::make_unique<td::Client>();
		std::cout << "TDLib client initialized." << std::endl;
		auto manager = std::make_shared<UserClientManager>(move(client));
		auto params = getConfigClientParameters();
		auto paramsResponse = manager->send(std::move(params));
		auto afterParamsAuthState = manager->proccessAuthUpdate(std::move(paramsResponse));
		if (afterParamsAuthState == PhoneRequired) {
			//TODO: add td::td_api::phoneNumberAuthenticationSettings()
			auto sendPhoneFunc = td::td_api::make_object<td::td_api::setAuthenticationPhoneNumber>(phoneNumber, nullptr);
			auto phoneResp = manager->send(std::move(sendPhoneFunc));
			auto phoneAuthState = manager->proccessAuthUpdate(std::move(phoneResp));
			if(phoneAuthState != AuthCodeRequired)
				//TODO: Deal with error nib
				std::cerr << "Failed to send phone for authorization" << std::endl;
			return manager;
		} else {
			std::cerr << "Failed to create client" << std::endl;
			std::cout << "you pussy" << std::endl;
			return nullptr;
		}
	}

	bool authorizeClient(const std::string& authCode) {
		auto checkAuthFunc = td::td_api::make_object<td::td_api::checkAuthenticationCode>(authCode);
		auto resp = send(std::move(checkAuthFunc));
		auto afterCodeAuthState = proccessAuthUpdate(std::move(resp));
		auto success = afterCodeAuthState == Successful;
		isAuthorized = success;
		if (!success)
			std::cerr << "Failed to authorize td_lib client" << std::endl;
		return success;
	}
	//If the client is not authorized return false
	td::td_api::object_ptr<td::td_api::Object> send(td::td_api::object_ptr<td::td_api::Function> tdFunc) {
		auto req = createRequest(std::move(tdFunc));
		registerRequest(req);
		auto response = waitForResponse(req->id);
		responses.erase(response->id);
		return std::move(response->object);
	}

	//If the client is not authorized return false
	template <typename Callback, typename... Args>
	auto sendWithCallback(td::td_api::object_ptr<td::td_api::Function> tdFunc, Callback&& callback, Args&&... args) {
		auto req = createRequest(std::move(tdFunc));
		registerRequest(req);
		auto response = waitForResponse(req->id);
		responses.erase(response->id);
		return std::invoke(std::forward<Callback>(callback),
			move(response->object),
			std::forward<Args>(args)...);
	}
};