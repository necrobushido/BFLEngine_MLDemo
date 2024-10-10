#include "ListBox.h"

int ListBox::GetSelectionIdx()
{
	return (int)SendDlgItemMessage(m_parentDialogHandle, m_controlID, (UINT)LB_GETCURSEL, 0, 0);
}

void ListBox::DeleteSelection()
{
	LRESULT	selectionIndex = SendDlgItemMessage(m_parentDialogHandle, m_controlID, (UINT)LB_GETCURSEL, 0, 0);
	if( selectionIndex != LB_ERR )
	{
		SendDlgItemMessage(m_parentDialogHandle, m_controlID, LB_DELETESTRING, selectionIndex, 0);
	}
}

void ListBox::AddEntry(const char* name)
{
	SendDlgItemMessage(m_parentDialogHandle, m_controlID, LB_INSERTSTRING, -1, (LPARAM)name);
}

void ListBox::Clear()
{
	SendDlgItemMessage(m_parentDialogHandle, m_controlID, LB_RESETCONTENT, 0, 0);
}

void ListBox::ChangeName(int idx, const char* name)
{
	if( idx >= 0 )
	{
		SendDlgItemMessage(m_parentDialogHandle, m_controlID, LB_DELETESTRING, idx, 0);
		SendDlgItemMessage(m_parentDialogHandle, m_controlID, LB_INSERTSTRING, idx, (LPARAM)name);
	}
}

void ListBox::SetSelection(int idx)
{
	SendDlgItemMessage(m_parentDialogHandle, m_controlID, LB_SETCURSEL, idx, 0);
}
