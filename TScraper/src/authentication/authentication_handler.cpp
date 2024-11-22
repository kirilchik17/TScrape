#include <td/telegram/Client.h>
#include <authentication/UserClientManager.cpp>
#include <authentication/AuthState.cpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <future>
#include <memory>

using Client = td::Client;

td::td_api::object_ptr<td::td_api::setTdlibParameters> getConfigClientParameters() {
    //TODO: Use .env for the params 
    td::td_api::object_ptr<td::td_api::setTdlibParameters> parameters 
        = td::td_api::make_object<td::td_api::setTdlibParameters>();

    parameters->database_directory_ = "tdlib_database"; // TODO: Change to db dir path
    parameters->files_directory_ = "tdlib_files";// TODO: Change to files path
    parameters->use_test_dc_ = false;
    parameters->api_id_ = 123456;  // TODO: Use API id
    parameters->api_hash_ = "your_api_hash"; // TODO: Use API hash
    parameters->system_language_code_ = "en";
    parameters->device_model_ = "Desktop";
    parameters->system_version_ = "1.0";//write our version
    parameters->application_version_ = "1.0";
    return parameters;
}

auto getNewClient() {
    auto client = std::make_unique<Client>();
    std::cout << "TDLib client initialized." << std::endl;
    auto manager =std::make_shared<UserClientManager>(move(client));
}
auto startAuthClient(std::shared_ptr<UserClientManager> client) {
    auto params = getConfigClientParameters();
    auto response = client->send(std::move(params));
    return proccessInitUpdate(std::move(response));
}

auto authSendPhone(std::shared_ptr<UserClientManager> client, std::string phoneNumber) {
    auto sendPhoneFunc = td::td_api::make_object<td::td_api::setAuthenticationPhoneNumber>(phoneNumber, nullptr);
    auto resp = client->send(std::move(sendPhoneFunc));
    return proccessInitUpdate(std::move(resp));
}

auto proccessInitUpdate(td::td_api::object_ptr<td::td_api::Object> response) {
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


auto initSendCode(shared_ptr<Client> client, string code){
    auto phoneReq = Client::Request();
    phoneReq.function = make_object<checkAuthenticationCode>(code);
    client->send(move(phoneReq));
    auto update = async(launch::async, proccessInitUpdate, client);
    return update.get();
}

