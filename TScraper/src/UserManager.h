#pragma once
#include <stdio.h>
#include <authentication/UserClientManager.cpp>
#include <unordered_map>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <memory>
#include <filesystem>
//TODO: Move implementation to .cpp
class UserManager {
private:
	std::unordered_map<std::string, std::shared_ptr<UserClientManager>> clients;

    void loadSessions(const std::string& baseDirectory) {
        if (!std::filesystem::exists(baseDirectory) || !std::filesystem::is_directory(baseDirectory)) {
            std::cerr << "Invalid base directory: " << baseDirectory << std::endl;
            return;
        }
        for (const auto& entry : std::filesystem::directory_iterator(baseDirectory)) {
            if (std::filesystem::is_directory(entry)) {
                std::string sessionPath = entry.path().string();
                std::string sessionName = entry.path().filename().string();
                std::cout << "Found session directory: " << sessionPath << std::endl;
                auto client = createClient(sessionPath);
                if (client) {
                    clients[sessionName] = client;
                }
            }
        }
    }

    std::shared_ptr<UserClientManager> createClient(const std::string& sessionDirectory) {
        auto client = UserClientManager::initiateClientFromSession(sessionDirectory);
        return client;
    }

    void stopAllClients() {
        for (auto& [sessionName, client] : clients) {
            std::cout << "Stopping client for session: " << sessionName << std::endl;
            client->send(td::td_api::make_object<td::td_api::close>());
        }
    }
	UserManager() {
        this->clients = std::unordered_map<std::string, std::shared_ptr<UserClientManager>>();
        loadSessions("sessions");
	}
    ~UserManager() {
        stopAllClients();
    }
public:
	UserManager(const UserManager&) = delete;
	UserManager& operator=(const UserManager&) = delete;

	static UserManager& getInstance() {
        //TODO: Insert a string to be the directory of the clients sessions
		static UserManager instance; 
		return instance;
	}

};
