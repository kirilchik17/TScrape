#include <iostream>
#include <scraping/TdMessage.cpp>
#include <authentication/UserClientManager.cpp>


std::shared_ptr<TdMessage> proccesMessage(std::shared_ptr<UserClientManager> client, td::td_api::object_ptr<td::td_api::message> msg) {
    if (!msg || !msg->content_)
        return nullptr;
    auto tdMsg = TdMessage();
    tdMsg.chatId = msg->chat_id_;
    tdMsg.dateTicks = msg->date_;
    tdMsg.senderId = msg->sender_id_->get_id();

    //TODO: Make sure content and other fields aren't null before accessing them
    switch (msg->content_->get_id()) {
    case td::td_api::messagePhoto::ID:
        tdMsg.textContent = td::move_tl_object_as<td::td_api::messagePhoto>(msg->content_)->caption_->text_;
        break;

    case td::td_api::messageAudio::ID:
        tdMsg.textContent = td::move_tl_object_as<td::td_api::messageAudio>(msg->content_)->caption_->text_;
        break;

    case td::td_api::messageVideo::ID:
        tdMsg.textContent = td::move_tl_object_as<td::td_api::messageVideo>(msg->content_)->caption_->text_;
        break;

    case td::td_api::messageText::ID:
        tdMsg.textContent = td::move_tl_object_as<td::td_api::messageText>(msg->content_)->text_->text_;
        tdMsg.hasFileContent = false;
        break;

    case td::td_api::messageDocument::ID:
        tdMsg.textContent = td::move_tl_object_as<td::td_api::messageDocument>(msg->content_)->caption_->text_;
        break;

    case td::td_api::messageAnimation::ID:
        tdMsg.textContent = td::move_tl_object_as<td::td_api::messageAnimation>(msg->content_)->caption_->text_;
        break;

    default:
        std::cout << "Currently unsupported message type" << std::endl;
        return nullptr;
    }
    return std::make_shared<TdMessage>(tdMsg);
}
