#pragma once

#include "DialogControl.h"

class EditControl : public DialogControl
{
public:
	enum
	{
		kTextChangedMsg = EN_CHANGE
	};

public:
	void GetText(char* buffer, int bufferSize);
	void SetText(const char* buffer);
};