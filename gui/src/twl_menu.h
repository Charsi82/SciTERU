// TWL_MENU.H
/////////////////////////

#pragma once
#include "twl.h"

struct Item
{
	UINT data;
	UINT id;
	Item(UINT _data);
	void trigger();

private:
	static UINT last_item_id;
};

class MessageHandler
{
	std::list<Item> m_list;

public:
	MessageHandler();
	~MessageHandler();
	void remove(UINT id);
	bool dispatch(UINT id);
	void add_handler(MessageHandler*);
	void add_item(Item&);
};
