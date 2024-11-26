#include <td/telegram/Client.h>
#include <iostream>
#include <authentication/UserClientManager.cpp>


std::vector<td::td_api::int53> fetchMainChatsIds(const std::shared_ptr<UserClientManager>& client, td::td_api::int32 limit = 1000) {
    auto whatChatsToPull = td::td_api::make_object<td::td_api::chatListMain>();
    auto fetchChatsFunc = td::td_api::make_object<td::td_api::getChats>(std::move(whatChatsToPull), limit);
    auto response = client->send(std::move(fetchChatsFunc));
    return handleChatsIdsResponse(std::move(response));
}

std::vector<td::td_api::int53> fetchArchiveChatsIds(const std::shared_ptr<UserClientManager>& client, td::td_api::int32 limit = 1000) {
    auto whatChatsToPull = td::td_api::make_object<td::td_api::chatListArchive>();
    auto fetchChatsFunc = td::td_api::make_object<td::td_api::getChats>(std::move(whatChatsToPull), limit);
    auto response = client->send(std::move(fetchChatsFunc));
    return handleChatsIdsResponse(std::move(response));
}

std::vector<td::td_api::int53> fetchFolderChatsIds(const std::shared_ptr<UserClientManager>& client, td::td_api::int32 chatFolderId ,td::td_api::int32 limit = 1000) {
    auto whatChatsToPull = td::td_api::make_object<td::td_api::chatListFolder>(chatFolderId);
    auto fetchChatsFunc = td::td_api::make_object<td::td_api::getChats>(std::move(whatChatsToPull), limit);
    auto response = client->send(std::move(fetchChatsFunc));
    return handleChatsIdsResponse(std::move(response));
}
void proccesChat(td::td_api::object_ptr<td::td_api::Object> response) {
    if (response->get_id() == td::td_api::chat::ID) {
        auto chat = td::td_api::move_object_as<td::td_api::chat>(response);
        // Extract chat details
        std::cout << "Chat ID: " << chat->id_ << std::endl;
        std::cout << "Chat Name: " << chat->title_ << std::endl;

        // Extract chat profile picture
        if (chat->photo_) {
            std::cout << "Chat Photo: Small(" << chat->photo_->small_->local_->path_
                << "), Big(" << chat->photo_->big_->local_->path_ << ")" << std::endl;
        }
        else {
            std::cout << "No profile picture set for this chat." << std::endl;
        }

        // Extract description (if available)
        if (!chat->description_.empty()) {
            std::cout << "Chat Description: " << chat->description_ << std::endl;
        }
        else {
            std::cout << "No description set for this chat." << std::endl;
        }

        // Chat type (e.g., private, group, channel)
        if (chat->type_->get_id() == td::td_api::chatTypePrivate::ID) {
            auto private_chat = td::td_api::move_object_as<td::td_api::chatTypePrivate>(chat->type_);
            std::cout << "Chat Type: Private with User ID: " << private_chat->user_id_ << std::endl;
        }
        else if (chat->type_->get_id() == td::td_api::chatTypeSupergroup::ID) {
            auto supergroup_chat = td::td_api::move_object_as<td::td_api::chatTypeSupergroup>(chat->type_);
            std::cout << "Chat Type: Supergroup, Supergroup ID: " << supergroup_chat->supergroup_id_ << std::endl;
        }
        else if (chat->type_->get_id() == td::td_api::chatTypeBasicGroup::ID) {
            auto basic_group_chat = td::td_api::move_object_as<td::td_api::chatTypeBasicGroup>(chat->type_);
            std::cout << "Chat Type: Basic Group, Group ID: " << basic_group_chat->basic_group_id_ << std::endl;
        }
        else if (chat->type_->get_id() == td::td_api::chatTypeChannel::ID) {
            auto channel_chat = td::td_api::move_object_as<td::td_api::chatTypeChannel>(chat->type_);
            std::cout << "Chat Type: Channel, Channel ID: " << channel_chat->channel_id_ << std::endl;
        }
    }
    else {
        std::cerr << "Unexpected response type for chat ID: " << chat_id << std::endl;
    }
}

std::vector<td::td_api::int53> handleChatsIdsResponse(td::td_api::object_ptr<td::td_api::Object> response) {
    if (response->get_id() != td::td_api::chats::ID) {
        std::cerr << "Didn't recieve chats, instead got response: " << response->get_id() << std::endl;
        return;
    }
    auto chats = td::td_api::move_object_as<td::td_api::chats>(response);
    return chats->chat_ids_;
}

