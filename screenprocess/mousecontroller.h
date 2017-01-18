#pragma once
#include "Header.h"

class mousecontroller
{
public:
	mousecontroller();
	~mousecontroller();

	void LeftClick();
	void MouseMove(int x, int y);
	void generateclick(int x, int y, int width, int height);
};

