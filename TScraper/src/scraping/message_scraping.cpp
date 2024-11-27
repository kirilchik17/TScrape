#include <iostream>
#include <scraping/td_message.cpp>
#include <scraping/td_content.cpp>
#include <authentication/UserClientManager.cpp>


std::shared_ptr<TdMessage> proccesMessage(std::shared_ptr<UserClientManager> client, td::td_api::object_ptr<td::td_api::message> msg) {
    if (!msg || !msg->content_)
        return nullptr;
    auto tdMsg = std::make_shared<TdMessage>();
    tdMsg->chatId = msg->chat_id_;
    tdMsg->dateTicks = msg->date_;
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
        tdMsg->textContent = td::move_tl_object_as<td::td_api::messageVideo>(msg->content_)->caption_->text_;
        break;
    }
    case td::td_api::messageText::ID: {
        tdMsg->textContent = td::move_tl_object_as<td::td_api::messageText>(msg->content_)->text_->text_;
        tdMsg->hasFileContent = false;
        break;
    }
    case td::td_api::messageDocument::ID: {
        tdMsg->textContent = td::move_tl_object_as<td::td_api::messageDocument>(msg->content_)->caption_->text_;
        break;
    }

    case td::td_api::messageAnimation::ID: {
        tdMsg->textContent = td::move_tl_object_as<td::td_api::messageAnimation>(msg->content_)->caption_->text_;
        break;
    }
    default:
        std::cerr << "Currently unsupported message type" << std::endl;
        return nullptr;
    }
    return std::make_shared<TdMessage>(tdMsg);
}
