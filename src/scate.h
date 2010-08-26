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

#ifndef _PLUGIN_KATE_SCATE_H_
#define _PLUGIN_KATE_SCATE_H_

#include <kate/mainwindow.h>
#include <kate/plugin.h>
#include <kate/pluginconfigpageinterface.h>
#include <kxmlguiclient.h>

#include <QThread>
#include <QTextEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QAction>
#include <QStringList>

class SCThread;

class  ScatePlugin :
  public Kate::Plugin,
  public Kate::PluginConfigPageInterface
{
  Q_OBJECT
  Q_INTERFACES(Kate::PluginConfigPageInterface)

  public:
    explicit  ScatePlugin( QObject* parent = 0, const QList<QVariant>& = QList<QVariant>() );
    virtual ~ ScatePlugin();
    Kate::PluginView *createView( Kate::MainWindow *mainWindow );

    uint  configPages () const { return 1; }
    QString configPageFullName (uint number) const;
    KIcon configPageIcon (uint number) const;
    QString configPageName (uint number) const;
    Kate::PluginConfigPage * configPage (uint number, QWidget *parent, const char *name);

    bool langRunning();
    bool serverRunning();
    inline QString iconPath() { return _iconPath; }
  signals:
    void scSaid( const QString& );
    void langSwitched( bool );
    void serverSwitched( bool );
    void swingOscSwitched( bool );
  public slots:
    void switchLang( bool );
    void switchServer( bool );
    void switchSwingOsc( bool );
    void restartLang();
    void startServer();
    void stopServer();
    void startSwingOSC();
    void stopSwingOSC();
    void eval( const QString&, bool silent = false );
    void stopProcessing();
  private slots:
    void cleanup();
  private:
    void startLang();
    void stopLang();
    SCThread *scThread;
    int scPid;
    int pipeW;
    int pipeR;
    QString _iconPath;
};

class SCThread : public QThread
{
  Q_OBJECT;
  public:
    SCThread( int p );
  signals:
    void scSays( const QString& str );
  private:
    void run();
    int pipe;
};

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
  private slots:
    void langStatusChanged( bool );
    void scSaid( const QString& );
    void evaluateCmdLine();
  private:
    ScatePlugin *plugin;
    QWidget *toolView;
    QTextEdit *scOutView;
    QLineEdit *cmdLine;
    QAction *aLangSwitch;
    QList<QAction*> langDepActions;
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
    QLineEdit *swingOscDirEdit;
    QCheckBox *startLangCheck;
};

#endif
