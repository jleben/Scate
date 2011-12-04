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

#ifndef SCATE_PLUGIN_H
#define SCATE_PLUGIN_H

#include <kpluginfactory.h>
#include <ktexteditor/document.h>
#include <kate/plugin.h>
#include <kate/pluginconfigpageinterface.h>

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

    void applyConfig();

    bool langRunning();
    bool serverRunning();
    inline QString iconPath() { return _iconPath; }

  signals:
    void scSaid( const QString& );
    void langSwitched( bool );
    void serverSwitched( bool );
    void swingOscSwitched( bool );
    void configChanged();

  public slots:
    void onDocumentCreated(KTextEditor::Document *);
    void switchLang( bool );
    void switchServer( bool );
    void switchSwingOsc( bool );
    void restartLang();
    void recompileLibrary();
    void startServer();
    void stopServer();
    void startSwingOSC();
    void stopSwingOSC();
    void eval( const QString&, bool silent = false );
    void stopProcessing();
    void switchToQt();
    void switchToSwing();
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

K_PLUGIN_FACTORY_DECLARATION( ScatePluginFactory );

#endif //SCATE_PLUGIN_H
