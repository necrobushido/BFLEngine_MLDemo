#pragma once

#include "DialogControl.h"

class ListBox : public DialogControl
{
public:
	enum
	{
		kSelectionChangedMsg = LBN_SELCHANGE
	};

public:
	int GetSelectionIdx();
	void DeleteSelection();
	void AddEntry(const char* name);
	void Clear();
	void ChangeName(int idx, const char* name);
	void SetSelection(int idx);
};