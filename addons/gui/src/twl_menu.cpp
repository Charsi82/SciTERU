// TWL_MENU.CPP
// Steve Donovan, 2003
// This is GPL'd software, and the usual disclaimers apply.
// See LICENCE
/////////////////////

#include "twl_menu.h"

UINT Item::last_item_id = 100;

Item::Item(UINT _data)
	: data(_data), id(++last_item_id)
{
	//log_add("add item %d", id);
}

MessageHandler::MessageHandler() : m_list{}
{
	//log_add("init_mh");
}

MessageHandler::~MessageHandler()
{
	//log_add("destroy_mh [%p]", this);
	m_list.clear();
}

void MessageHandler::add_item(Item& item)
{
	m_list.push_back(item);
}

void MessageHandler::remove(UINT id)
{
	m_list.remove_if([&id](const Item& item) { return item.id == id; });
}

bool MessageHandler::dispatch(UINT id)
{
	auto it = std::find_if(m_list.begin(), m_list.end(), [&id](const Item& item) { return item.id == id; });
	if (it != m_list.end())
	{
		it->trigger();
		return true;// found, but no action
	}
	return false;
}

void MessageHandler::add_handler(MessageHandler* hndlr)
{
	if (hndlr) m_list.splice(m_list.end(), hndlr->m_list);
}
