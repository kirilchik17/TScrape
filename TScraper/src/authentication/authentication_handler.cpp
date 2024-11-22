#include <td/telegram/Client.h>
#include <authentication/UserClientManager.cpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <future>
#include <memory>

using namespace td::td_api;
using namespace std;
using Client = td::Client;

object_ptr<setTdlibParameters> getConfigClientParameters() {
    //TODO: Use .env for the params 
    object_ptr<setTdlibParameters> parameters = make_object<setTdlibParameters>();
    parameters->database_directory_ = "tdlib_database"; // TODO: Change to db dir path
    parameters->files_directory_ = "tdlib_files";// TODO: Change to files path
    parameters->use_test_dc_ = false;
    parameters->api_id_ = 123456;  // TODO: Use API id
    parameters->api_hash_ = "your_api_hash"; // TODO: Use API hash
    parameters->system_language_code_ = "en";
    parameters->device_model_ = "Desktop";
    parameters->system_version_ = "1.0";
    parameters->application_version_ = "1.0";
    return parameters;
}

auto getNewClient() {
    auto client = make_unique<Client>();
    cout << "TDLib client initialized." << endl;
    auto manager = make_shared<UserClientManager> client;
}
auto startInitClient(shared_ptr<Client> client) {
    auto params = getConfigClientParameters();
    auto init_req = Client::Request();
    init_req.function = move(params);
    client->send(move(init_req));
    auto update = async(launch::async, proccessInitUpdate, client);
    return update.get();
}
auto initSendPhone(shared_ptr<Client> client, string phoneNumber) {
    auto phoneReq = Client::Request();
    //TODO: add settings if needed  -> phoneNumberAuthenticationSettings()
    phoneReq.function = make_object<setAuthenticationPhoneNumber>(phoneNumber, nullptr);
    client->send(move(phoneReq));
    auto update = async(launch::async, proccessInitUpdate, client);
    return update.get();
}

auto initSendCode(shared_ptr<Client> client, string code){
    auto phoneReq = Client::Request();
    phoneReq.function = make_object<checkAuthenticationCode>(code);
    client->send(move(phoneReq));
    auto update = async(launch::async, proccessInitUpdate, client);
    return update.get();
}

auto proccessInitUpdate(shared_ptr<Client> client) {
    int restart = 3;
    while (true && restart != 0) {
        --restart;
        auto response = client->receive(10);// wait for 10 sec max
        if (response.object->get_id() == updateAuthorizationState::ID) {
            auto authStateObj = td::move_tl_object_as<updateAuthorizationState>(response.object).get();
            auto stateId = authStateObj->authorization_state_->get_id();
            switch (stateId) {
                case authorizationStateWaitTdlibParameters::ID:
                    // Already sent TDLib parameters
                    break;

                case authorizationStateWaitPhoneNumber::ID:
                    std::cout << "Waiting for user {userId} phone number" << endl;
                    return AuthState::PhoneRequired;

                case authorizationStateWaitCode::ID:
                    cout << "Waiting for user {userId} auth code" << endl;
                    return AuthState::AuthCodeRequired;

                case authorizationStateReady::ID:
                    cout << "Authentication successful!" << endl;
                    return AuthState::Successful;

                case authorizationStateClosed::ID:
                    cout << "TDLib client closed." << endl;
                    return AuthState::Closed;

                default:
                    cerr << "Unexpected authorization state: " << stateId << endl;
                    if (restart == 0) 
                        return AuthState::Unknown;
                    break;
            }
        }
    }
}