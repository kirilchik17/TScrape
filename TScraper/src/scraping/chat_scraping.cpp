#include <td/telegram/Client.h>
#include <iostream>
#include <classes/TdMessage.cpp>

using namespace td::td_api;
using namespace std;
using Client = td::Client;

auto fetchChatMessages(shared_ptr<Client> client, int53 chatId,
	int53 fromMessageId = 0, int32 offset = 0,
	int32 limit = INT32_MAX, bool onlyLocal = true)
{
	auto req = Client::Request();
	auto reqFunction = make_object<getChatHistory>(chatId, fromMessageId, offset, limit, onlyLocal);
	req.function = move(reqFunction);
	client->send(move(req));

	//TODO: Maybe use a while loop to check if we got it
	auto response = client->receive(10);

    if (response.object) {
        if (response.object->get_id() == messages::ID) {
            auto messages = move_object_as<td::td_api::messages>(response.object);

            cout << "Received " << messages->messages_.size() << " messages from chat - " << chatId << endl;
            for (const auto& message : messages->messages_) {
                cout << "Message ID: " << message->id_
                    << ", Text: " << message->content_->get_id() << endl;
                
            }
        }
        else {
            cout << "Unexpected response type: " << response.object->get_id() << endl;
        }
    }
    else {
        cerr << "No response received or timeout." << endl;
    }
}

TdMessage proccesMessage(message msg) {
    auto tdMsg = TdMessage();
    
}