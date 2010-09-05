/*
#
# Copyright 2010 Jakob Leben (jakob.leben@gmail.com)
#
# This file is part of Scate - a SuperCollider plugin for Kate
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
*/

#ifndef SCATE_CONFIG_H
#define SCATE_CONFIG_H

#include <kate/pluginconfigpageinterface.h>

#include <QLineEdit>
#include <QCheckBox>
#include <QListWidget>

class ScatePlugin;
class ScateDirListWidget;

class ScateConfigPage : public Kate::PluginConfigPage
{
  public:
    ScateConfigPage( ScatePlugin *, QWidget * );
    void apply();
    void defaults();
    void reset();
  private:
    QLineEdit *sclangExeEdit;
    QLineEdit *dataDirEdit;
    ScateDirListWidget *helpDirList;
    QLineEdit *swingOscDirEdit;
    QCheckBox *startLangCheck;
};

class ScateDirListWidget : public QWidget
{
  Q_OBJECT
  public:
    ScateDirListWidget( QWidget *parent = 0 );
    void setDirs( const QStringList & );
    QStringList dirs();
  public slots:
    void addDir();
    void removeDir();
  private:
    QListWidget *list;
};

#endif //SCATE_CONFIG_H
