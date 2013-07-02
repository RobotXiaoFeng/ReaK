
/*
 *    Copyright 2013 Sven Mikael Persson
 *
 *    THIS SOFTWARE IS DISTRIBUTED UNDER THE TERMS OF THE GNU GENERAL PUBLIC LICENSE v3 (GPLv3).
 *
 *    This file is part of ReaK.
 *
 *    ReaK is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    ReaK is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with ReaK (as LICENSE in the root folder).  
 *    If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef REAK_VIEW3D_MENU_HPP
#define REAK_VIEW3D_MENU_HPP

#include <QMenu>

#include <string>

class SoSwitch;
class SoSeparator;

namespace ReaK {
  
namespace rkqt {

class View3DMenu : public QMenu {
    Q_OBJECT
  
  public:
    View3DMenu( QWidget * parent = 0, SoSeparator* aRoot = NULL );
    ~View3DMenu();
    
  private slots:
    
    void toggleDisplayGroup(bool isChecked);
    
  public:
    
    void setRoot(SoSeparator* aRoot);
    SoSwitch* getDisplayGroup(const std::string& aGroupName, bool initChecked = true);
    void removeDisplayGroup(const std::string& aGroupName);
    
  private:
    
    struct display_group {
      QAction* selector;
      SoSwitch* display_switch;
      
      display_group(QAction* aSelector = NULL, SoSwitch* aDisplaySwitch = NULL) : selector(aSelector), display_switch(aDisplaySwitch) { };
    };
    
    SoSeparator* root_sep;
    std::map< std::string, display_group > display_items;
    
};


};


};


#endif














