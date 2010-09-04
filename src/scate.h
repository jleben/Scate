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
#include <khtml_part.h>

#include <QThread>
#include <QTextEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QAction>
#include <QStringList>
#include <QProcess>

class SCProcess;

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
    void scStarted();
    void scFinished( int, QProcess::ExitStatus );
  private:
    void startLang();
    void stopLang();
    void sysMsg( const QString & );
    SCProcess *scProcess;
    QString _iconPath;
    bool restart;
};

class SCProcess : public QProcess
{
  Q_OBJECT
  public:
    SCProcess( QObject *parent = 0 );
  signals:
    void scSays( const QString& str );
  private slots:
    void onReadyRead();
};

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
    QLineEdit *helpDirEdit;
    QLineEdit *swingOscDirEdit;
    QCheckBox *startLangCheck;
};

#endif
