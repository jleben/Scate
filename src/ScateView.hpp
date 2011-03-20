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

#ifndef SCATE_VIEW_H
#define SCATE_VIEW_H

#include "cmdline.hpp"

#include <kxmlguiclient.h>
#include <kate/plugin.h>
#include <kate/mainwindow.h>

#include <QTextEdit>
#include <QLineEdit>

class ScatePlugin;
class ScateHelpWidget;
class ScateCmdLine;
class ScateHelpBrowser;

class ScateView : public Kate::PluginView, public KXMLGUIClient
{
    Q_OBJECT

  public:
     ScateView(  ScatePlugin *plugin, Kate::MainWindow *mainWindow );
    ~ ScateView();

    virtual void readSessionConfig( KConfigBase* config, const QString& groupPrefix );
    virtual void writeSessionConfig( KConfigBase* config, const QString& groupPrefix );
  public slots:
    void evaluateSelection();
    void browseSelectedClass();
    void helpForSelectedClass();
  private slots:
    void langStatusChanged( bool );
    void scSaid( const QString& );
  private:
    QWidget * createOutputView();
    QWidget * createHelpView();

    ScatePlugin *plugin;

    QWidget *outputToolView;
    QTextEdit *scOutView;
    int maxOutBlocks;
    Scate::CmdLine *cmdLine;

    QWidget *helpToolView;
    ScateHelpBrowser *helpWidget;

    QAction *aLangSwitch;
    QList<QAction*> langDepActions;
};

#endif //SCATE_VIEW_H
