#pragma once
#include "Twl.h"

class TCustomPaintWin :public TWin
{
public:
	TCustomPaintWin(TEventWindow* form);
	virtual void handle_paint(TDC*) = 0;
};