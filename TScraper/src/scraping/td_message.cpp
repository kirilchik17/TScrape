#include <string>
#include <scraping/td_content.cpp>
#include <memory>
using string = std::string;

class TdMessage {
public:
	string textContent;
	string chatId;
	string senderId;
	std::shared_ptr<TdContent> content;
	int dateTicks;
	bool hasFileContent = true;
};
