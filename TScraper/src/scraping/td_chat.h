#include <string>
#include "td/telegram/td_api.h"
#include "scraping/td_chat_type.cpp"
class TdChat {
public:
	std::string title;
	std::string description;
	std::string title;
	td::td_api::int53 id;
	TdChatType chatType;
};