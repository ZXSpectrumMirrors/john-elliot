/* PCW Explorer - access Amstrad PCW discs on Linux or Windows
    Copyright (C) 2000  John Elliott

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "proppage.hxx"

class PropPageDrive : public PcwPropPage
{
public:
	PropPageDrive(struct cpmStatFS *buf, wxIcon &icon,
                      wxWindow *parent, wxWindowID id, 
                       const wxPoint& pos = wxDefaultPosition, 
                       const wxSize& size = wxDefaultSize, 
                       long style = wxDEFAULT_DIALOG_STYLE | wxCAPTION, 
                       const wxString& name = "dialog");

	void OnPaint(wxPaintEvent &e);
protected:
	struct cpmStatFS *m_statfs;
	DECLARE_EVENT_TABLE()
};

