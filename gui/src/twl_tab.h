#pragma once
#include <vector>

class TTabControl : public TNotifyWin, public THasIconWin
{
	int m_index;
	int m_last_selected_idx;
	std::vector<TWin*> panels;

public:
	TTabControl(TEventWindow* form);
	~TTabControl();
	void add(wchar_t* caption, TWin* data, int image_idx = -1);
	void remove(int idx = -1);
	void* get_data(int idx = -1);
	void selected(int idx);
	int selected();
	int fixedwidth();
	void fixedwidth(int flag);
	void set_item_size(int w, int h);
	void set_image_list(bool small_size) override;
	int getRowCount() const;
	int handle_notify(void* p) override;

private:
	virtual void handle_select(int id) = 0;
};
