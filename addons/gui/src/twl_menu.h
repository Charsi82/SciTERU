// TWL_MENU.H
/////////////////////////

#pragma once
#include "twl.h"

class MessageHandler
{
	struct Item
	{
		UINT data;
		UINT id;
		Item(UINT _data);

	private:
		static UINT last_item_id;
	};
	std::list<Item> m_list;

public:
	MessageHandler();
	~MessageHandler();
	void remove(UINT id);
	bool dispatch(UINT id);
	void add_handler(MessageHandler*);
	UINT add_item(UINT data);
	void trigger(UINT data) const;
};
