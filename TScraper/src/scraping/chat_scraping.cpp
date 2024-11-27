#include <td/telegram/Client.h>
#include <iostream>
#include <authentication/UserClientManager.cpp>
#include <scraping/td_chat.h>


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

//Unfinished
//TODO: write cache handler to see if the images of chats are already there
std::shared_ptr<TdChat> proccesChatResponse(td::td_api::object_ptr<td::td_api::Object> response, std::shared_ptr<UserClientManager> client) {
    auto chat = td::td_api::move_object_as<td::td_api::chat>(response);
    auto tdChat = std::make_shared<TdChat>();
    tdChat->id = chat->id_;
    tdChat->title = chat->title_;
    //Possible to pull chat->unread_count_ but seems unnecessery
    
    if (chat->photo_) {
        //TODO: Check if photo path exists
        std::cout << "Chat Photo: Small(" << chat->photo_->small_->local_->path_ << std::endl;
        auto download_request = td::td_api::make_object<td::td_api::downloadFile>(
            chat->photo_->big_->id_,
            10,    // Priority of the download (higher is prioritized more), ranges between 1-32
            0, // offset
            0, // limit
            false // synchronus
        );
        //Start downloading proccess
        auto resp = client->send(std::move(download_request));
    }
    else {
        std::cout << "No profile picture set for this chat." << std::endl;
    }
    // Chat type (e.g., private, group, channel)
    //Description is available on Super and Basic group but another reqeust is needed for that
    if (chat->type_->get_id() == td::td_api::chatTypePrivate::ID) {
        auto private_chat = td::td_api::move_object_as<td::td_api::chatTypePrivate>(chat->type_);
        std::cout << "Chat Type: Private with User ID: " << private_chat->user_id_ << std::endl;
        tdChat->chatType = PrivateChat;
    }
    else if (chat->type_->get_id() == td::td_api::chatTypeSupergroup::ID) {
        auto supergroup_chat = td::td_api::move_object_as<td::td_api::chatTypeSupergroup>(chat->type_);
        std::cout << "Chat Type: Supergroup, Supergroup ID: " << supergroup_chat->supergroup_id_ << std::endl;
        tdChat->chatType = Supergroup;

    }
    else if (chat->type_->get_id() == td::td_api::chatTypeBasicGroup::ID) {
        auto basic_group_chat = td::td_api::move_object_as<td::td_api::chatTypeBasicGroup>(chat->type_);
        std::cout << "Chat Type: Basic Group, Group ID: " << basic_group_chat->basic_group_id_ << std::endl;
        tdChat->chatType = BasicGroup;
    }
    return tdChat;
}

std::vector<td::td_api::int53> handleChatsIdsResponse(td::td_api::object_ptr<td::td_api::Object> response) {
    if (response->get_id() != td::td_api::chats::ID) {
        std::cerr << "Didn't recieve chats, instead got response: " << response->get_id() << std::endl;
        return;
    }
    auto chats = td::td_api::move_object_as<td::td_api::chats>(response);
    return chats->chat_ids_;
}

