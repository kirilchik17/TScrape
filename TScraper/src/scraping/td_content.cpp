#include <td/telegram/td_api.h>
class TdContent {
public:
	std::string title;
	td::td_api::int53 size;
	int id;
};
class TdPhoto : public TdContent{
public:
	int width;
	int height;
};
class TdVideo : public TdContent {
public: 
	int duration;
	int width;
	int height;
};
class TdAudio : public TdContent {
public:
	std::string performer;
	int duration;
};
class TdDocument : public TdContent {
public:
	std::string mimetype;
};
class TdAnimation : public TdContent {
public:
	int width;
	int height;
	int duration;
};