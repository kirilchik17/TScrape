//TODO: Instead make this class a file so it will include a list of callback functions when the client recieves the response. 
//Because the request and response will include the same ids we can connect them even tho the client can recieve a different response at the same time
//Thus there will be a thread that will always listen to the client's response and will start a function that deals with the response dependant on id


//Talk with Amir - we will need a single thread that will listen to the client updates but it will be a global thread as there are more 
//functions that chat scraping so we will need a TGClientManager that will always run the client and will have a list of tasks with their corresponding 
//request/response ids and a function to callback to that will be written by us
#include <td/telegram/Client.h>
#include <iostream>
#include <classes/TdMessage.cpp>
#include <queue>
#include <set>
#include <map>
#include <mutex>
using namespace td::td_api;
using namespace std;
using Client = td::Client;


std::mutex download_mutex;

auto fetchChatMessages(shared_ptr<Client> client, int53 chatId,
	int53 fromMessageId = 0, int32 offset = 0,
	int32 limit = INT32_MAX, bool onlyLocal = true)
{
    shared_ptr<map<object_ptr<message>, TdMessage>> activeDownloads;   
    shared_ptr<queue<object_ptr<message>>> pendingDownloads;

	auto reqFunction = make_object<getChatHistory>(chatId, fromMessageId, offset, limit, onlyLocal);
	client->send({0, move(reqFunction)});

	//TODO: Maybe use a while loop to check if we got it
	auto response = client->receive(10);
    //Taking the messages to queue
    if (response.object) {
        if (response.object->get_id() == messages::ID) {
            auto messages = move_object_as<td::td_api::messages>(response.object);
            cout << "Received " << messages->messages_.size() << " messages from chat - " << chatId << endl;
            pendingDownloads = make_shared<queue<object_ptr<message>>>(messages->messages_);
        }
        else {
            cout << "Unexpected response type: " << response.object->get_id() << endl;
        }
    }
    else {
        cerr << "No response received or timeout." << endl;
    }

    while (!pendingDownloads->empty()) {
        if (activeDownloads->size() == 10)
            continue;
        auto nextDownload = move(pendingDownloads->front());
        pendingDownloads->pop();

        activeDownloads->insert({ move(nextDownload), TdMessage()});

    }
}


shared_ptr<TdMessage> proccesMessage(shared_ptr<Client> client, object_ptr<message> msg) {
    if (!msg || !msg->content_)
        return nullptr;
    auto tdMsg = TdMessage();
    tdMsg.chatId = msg->chat_id_;
    tdMsg.dateTicks = msg->date_;
    tdMsg.senderId = msg->sender_id_->get_id();

    //TODO: Make sure content and other fields aren't null before accessing them
    switch (msg->content_->get_id()) {
    case messagePhoto::ID:
        tdMsg.textContent = td::move_tl_object_as<messagePhoto>(msg->content_)->caption_->text_;
        break;

    case messageAudio::ID:
        tdMsg.textContent = td::move_tl_object_as<messageAudio>(msg->content_)->caption_->text_;
        break;

    case messageVideo::ID:
        tdMsg.textContent = td::move_tl_object_as<messageVideo>(msg->content_)->caption_->text_;
        break;

    case messageText::ID:
        tdMsg.textContent = td::move_tl_object_as<messageText>(msg->content_)->text_->text_;
        tdMsg.hasFileContent = false;
        break;

    case messageDocument::ID:
        tdMsg.textContent = td::move_tl_object_as<messageDocument>(msg->content_)->caption_->text_;
        break;

    case messageAnimation::ID:
        tdMsg.textContent = td::move_tl_object_as<messageAnimation>(msg->content_)->caption_->text_;
        break;

    default:
        cout << "Currently unsupported message type" << endl;
        return nullptr;
    }
    return make_shared<TdMessage>(tdMsg);
}

auto downloadMessageFile(shared_ptr<Client> client ,td::td_api::int32 fileId,
    int32 priority = 1, int53 offset = 0, int53 limit = 0, bool runAsync) 
{
    Client::Request::
    auto downloadFunc = make_object<downloadFile>(fileId, priority, offset, limit, !runAsync);
    client->send({ 0 , move(downloadFunc) });

    //This shit might not even work as there may be multiple files downloaded at the same time
    //A set can be used to store files ids and connect them to the recieved data

    //sizes_ includes includes all the different photo sizes - the last one is the largets 
    //sizes_.back()->photo_ includes the exact photo_ and photo_->id_ includes it's id
    while (true) {

        std::unique_lock<std::mutex> lock(download_mutex);
        auto response = client->receive(10.0); // Wait up to 10 seconds
        lock.unlock();
        if (!response.object) continue;

        if (response.object->get_id() == td::td_api::file::ID) {
            auto file = td::td_api::move_object_as<td::td_api::file>(response.object);
            if (file->local_->is_downloading_completed_) {
                std::cout << "Photo downloaded to: " << file->local_->path_ << std::endl;
                return;
            }
            else {
                std::cout << "Downloading photo... " << std::endl;
            }
        }
        else {
            std::cout << "Unexpected response ID: " << response.object->get_id() << std::endl;
        }
    }
}