#include <iostream>
#include <scraping/td_message.cpp>
#include <scraping/td_content.cpp>
#include <authentication/UserClientManager.cpp>

std::shared_ptr<std::vector<std::shared_ptr<TdMessage>>> fetchMessagesFromChat(std::shared_ptr<UserClientManager> client, td::td_api::int53 chatId,
    td::td_api::int53 fromMessage = 0, td::td_api::int53 offset = 0, td::td_api::int53 limit = 100, bool onlyLocal = false) {
    //max limit of messages is 100
    //Limit 0 will fetch the most recent message
    auto pullMessagesFunc = td::td_api::make_object<td::td_api::getChatHistory>(chatId, fromMessage, offset, limit, true);
    auto response = client->send(std::move(pullMessagesFunc));
    if (response->get_id() != td::td_api::messages::ID) {
        return nullptr;
    }
    else {
        auto chatMessages = std::make_shared<std::vector<std::shared_ptr<TdMessage>>>();
        auto messages = td::td_api::move_object_as<td::td_api::messages>(response);
        for (auto& message : messages->messages_) {
            auto tdMsg = proccesMessage(client, message);
            chatMessages->push_back(tdMsg);
        }
    }
}


std::shared_ptr<TdMessage> proccesMessage(std::shared_ptr<UserClientManager> client, td::td_api::object_ptr<td::td_api::message>& msg) {
    if (!msg || !msg->content_)
        return nullptr;
    auto tdMsg = std::make_shared<TdMessage>();
    tdMsg->chatId = msg->chat_id_;
    tdMsg->dateTicks = msg->date_;
    tdMsg->messageId = msg->id_;
    // msg->sender_id_ is not and does not include the ID of the sender

    //TODO: Make sure content and other fields aren't null before accessing them
    switch (msg->content_->get_id()) {
    case td::td_api::messagePhoto::ID: {
        auto photoMsg = td::move_tl_object_as<td::td_api::messagePhoto>(msg->content_);
        tdMsg->textContent = photoMsg->caption_->text_;

        auto content = std::shared_ptr<TdPhoto>();
        auto bigPhoto = std::move(photoMsg->photo_->sizes_.front());
        content->id = bigPhoto->photo_->id_;
        content->size = bigPhoto->photo_->expected_size_;
        content->width = bigPhoto->width_;
        content->height = bigPhoto->height_;
        tdMsg->content = content;
        break;
    }
    case td::td_api::messageAudio::ID: {
        auto audioMsg = td::move_tl_object_as<td::td_api::messageAudio>(msg->content_);
        tdMsg->textContent = audioMsg->caption_->text_;
        auto content = std::make_shared<TdAudio>();
        content->title = audioMsg->audio_->title_;
        content->size = audioMsg->audio_->audio_->expected_size_;
        content->duration = audioMsg->audio_->duration_;
        content->id = audioMsg->audio_->audio_->id_;
        content->performer = audioMsg->audio_->performer_;
        tdMsg->content = content;
        break;
    }
    case td::td_api::messageVideo::ID: {
        auto videoMsg = td::move_tl_object_as<td::td_api::messageVideo>(msg->content_);
        auto content = std::make_shared<TdVideo>();
        tdMsg->textContent = videoMsg->caption_->text_;
        content->size = videoMsg->video_->video_->expected_size_;
        content->duration = videoMsg->video_->duration_;
        content->title = videoMsg->video_->file_name_;
        content->id = videoMsg->video_->video_->id_;
        content->height = videoMsg->video_->height_;
        content->width = videoMsg->video_->width_;
        break;
    }
    case td::td_api::messageText::ID: {
        tdMsg->textContent = td::move_tl_object_as<td::td_api::messageText>(msg->content_)->text_->text_;
        tdMsg->content = nullptr;
        break;
    }
    case td::td_api::messageDocument::ID: {
        auto docMsg = td::move_tl_object_as<td::td_api::messageDocument>(msg->content_);
        tdMsg->textContent = docMsg->caption_->text_;
        auto content = std::make_shared<TdDocument>();
        content->id = docMsg->document_->document_->id_;
        content->size = docMsg->document_->document_->expected_size_;
        content->title = docMsg->document_->file_name_;
        content->mimetype = docMsg->document_->mime_type_;
        break;
    }
    case td::td_api::messageAnimation::ID: {
        auto animMsg = td::move_tl_object_as<td::td_api::messageAnimation>(msg->content_);
        tdMsg->textContent = animMsg->caption_->text_;
        auto content = std::make_shared<TdAnimation>();
        content->id = animMsg->animation_->animation_->id_;
        content->size = animMsg->animation_->animation_->expected_size_;
        content->title = animMsg->animation_->file_name_;
        content->height = animMsg->animation_->height_;
        content->width = animMsg->animation_->width_;
        content->duration = animMsg->animation_->duration_;
        break;
    }
    default:
        std::cerr << "Currently unsupported message type" << std::endl;
        return nullptr;
    }
    return std::make_shared<TdMessage>(tdMsg);
}
