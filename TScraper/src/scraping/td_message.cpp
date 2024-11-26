#include <string>
using string = std::string;

class TdMessage {
public:
	string textContent;
	string base64Content;
	string chatId;
	string senderId;
	int dateTicks;
	bool hasFileContent = true;
};