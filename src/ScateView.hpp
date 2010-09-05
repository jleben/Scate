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

#include <kxmlguiclient.h>
#include <khtml_part.h>
#include <kate/plugin.h>
#include <kate/mainwindow.h>

#include <QTextEdit>
#include <QLineEdit>
#include <QCheckBox>

class ScatePlugin;
class ScateHelpWidget;

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
    void evaluateCmdLine();
  private:
    void createOutputView();
    void createHelpView();

    ScatePlugin *plugin;

    QWidget *outputView;
    QTextEdit *scOutView;
    QLineEdit *cmdLine;

    QWidget *helpView;
    ScateHelpWidget *helpWidget;

    QAction *aLangSwitch;
    QList<QAction*> langDepActions;
};

class ScateUrlHistory : public QObject
{
  Q_OBJECT
  public:
    ScateUrlHistory( QObject *parent = 0 );
    QList<QAction*> actions();
  signals:
    void wentTo( const KUrl & );
  public slots:
    void goBack();
    void goForward();
    void goTo( const KUrl & );
  private:
    void print();

    QList<KUrl> history;
    int curIndex;
    QList<QAction*> _actions;
};

class ScateHelpWidget : public QWidget
{
  Q_OBJECT
  public:
    ScateHelpWidget( QWidget * parent = 0 );
    ScateUrlHistory *history() { return _history; }
  public slots:
    void goHome();
    void goToClass( const QString & className );
    void openUrl( const KUrl &url );
  private:
    void warnSetHelpDir();
    KHTMLPart *browser;
    ScateUrlHistory *_history;
};

class ScateCmdLine : public QLineEdit
{
  Q_OBJECT

  public:
    ScateCmdLine();
  signals:
    void invoked( const QString&, bool );
  private:
    void keyPressEvent( QKeyEvent * );
    QStringList history;
    int curHistory;
};

#endif //SCATE_VIEW_H
