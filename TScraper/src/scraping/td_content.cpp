#include <td/telegram/td_api.h>
class TdContent {
public:
	std::string title;
	td::td_api::int53 size;
	int id;
};
class TdPhoto : public TdContent{

};
class TdVideo : public TdContent {

};
class TdAudio : public TdContent {
public:
	std::string performer;
	int duration;
};
class TdDocument : public TdContent {

};