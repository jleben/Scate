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

#include "ScatePlugin.hpp"
#include "ScateView.hpp"
#include "ScateConfigPage.hpp"

#include <kaboutdata.h>
#include <kstandarddirs.h>
#include <kconfiggroup.h>

#include <cstdio>

SCProcess::SCProcess( QObject *parent ) : QProcess( parent )
{
  connect( this, SIGNAL( readyRead() ), this, SLOT( onReadyRead() ) );
}

void SCProcess::onReadyRead()
{
  QByteArray bytes = readAll();
  if( bytes.isEmpty() ) return;
  QString text( bytes );
  emit scSays( text );
}

K_PLUGIN_FACTORY_DEFINITION(ScatePluginFactory, registerPlugin<ScatePlugin>();)
K_EXPORT_PLUGIN( ScatePluginFactory( KAboutData("katescateplugin", 0,
                                                ki18n("Scate"),
                                                "0.9.2",
                                                ki18n("Kate SuperCollider plugin") ) ) )

ScatePlugin::ScatePlugin( QObject* parent, const QList<QVariant>& )
    : Kate::Plugin( (Kate::Application*)parent, "kate-scate-plugin" ),
    scProcess( new SCProcess( this ) ),
    _iconPath( KStandardDirs::locate( "data", "kate/plugins/katescate/supercollider.png" ) ),
    restart( false )
{
  connect( scProcess, SIGNAL( started() ), this, SLOT( scStarted() ) );
  connect( scProcess, SIGNAL( finished( int, QProcess::ExitStatus ) ),
           this, SLOT( scFinished( int, QProcess::ExitStatus ) ) );
  connect( scProcess, SIGNAL( scSays( const QString& ) ),
           this, SIGNAL( scSaid( const QString& ) ) );
  KConfigGroup config(KGlobal::config(), "Scate");
  bool b_startLang = config.readEntry( "StartLang", false );
  if( b_startLang )
    startLang();
}

ScatePlugin::~ScatePlugin()
{
}

Kate::PluginView *ScatePlugin::createView( Kate::MainWindow *mainWindow )
{
  return new ScateView( this, mainWindow );
}

QString ScatePlugin::configPageFullName (uint number=0) const
{ return ( number == 0 ? i18n( "Scate Configuration" ) : QString() ); }

KIcon ScatePlugin::configPageIcon (uint number=0) const
{ return ( number == 0 ? KIcon( QIcon(_iconPath) ) : KIcon() ); }

QString ScatePlugin::configPageName (uint number=0) const
{ return ( number == 0 ? i18n( "Scate" ) : QString() ); }

Kate::PluginConfigPage * ScatePlugin::configPage (uint number=0, QWidget *parent=0, const char *name=0)
{
  Q_UNUSED( name );
  if( number != 0 ) return 0;
  return new ScateConfigPage( this, parent );
}

void ScatePlugin::startLang()
{
  QProcess::ProcessState state = scProcess->state();
  if( state == QProcess::Starting ) {
      printf("\nInterpreter already starting.\n\n");
      return;
  }
  else if( state == QProcess::Running ) {
      printf("\nInterpreter already running.\n\n");
      return;
  }
  else if( state != QProcess::NotRunning ) {
    return;
  }

  sysMsg( "Interpreter starting." );

  KConfigGroup config(KGlobal::config(), "Scate");
  QString exe = config.readEntry( "ScLangExecutable", QString() );
  QString rtDir = config.readEntry( "RuntimeDataDir", QString() );

  QString cmd = exe.isEmpty() ? tr( "sclang" ) : exe;
  cmd += tr( " -i scate" );
  if( !rtDir.isEmpty() ) cmd.append( " -d " ).append( rtDir );

  printf("Trying to start with command:\n");
  printf( "%s\n", cmd.toStdString().c_str() );

  scProcess->start( cmd );
}

void ScatePlugin::stopLang()
{
  scProcess->terminate();
}

void ScatePlugin::sysMsg( const QString &msg )
{
  emit scSaid(tr("\n") + msg + tr("\n\n"));
}

void ScatePlugin::scStarted()
{
  emit langSwitched( true );
}

void ScatePlugin::scFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
  Q_UNUSED( exitCode );
  QString msg;
  switch( exitStatus ) {
    /*case QProcess::CrashExit:
      msg = "ERROR: Interpreter crashed!"; break;*/
    case QProcess::NormalExit:
    default:
      msg = "Interpreter stopped.";
  }

  sysMsg( msg );
  emit( langSwitched( false ) );
  emit( serverSwitched( false ) );

  if( restart ) {
    restart = false;
    startLang();
  }
}

void ScatePlugin::startServer()
{
  eval( "Server.default.boot;", true );
  emit( serverSwitched( true ) );
}

void ScatePlugin::stopServer()
{
  eval( "Server.default.quit;", true );
  emit( serverSwitched( false ) );
}

void ScatePlugin::startSwingOSC()
{
  KConfigGroup config(KGlobal::config(), "Scate");
  QString swingosc = config.readEntry( "SwingOscProgram", QString() );
  eval( QString("SwingOSC.program=\"%1\";").arg( swingosc ), true );
  eval( "SwingOSC.default.boot;", true );
}

void ScatePlugin::stopSwingOSC()
{
  eval( "SwingOSC.default.quit;", true );
}

void ScatePlugin::switchLang( bool on )
{
  restart = false;
  if( on ) startLang();
  else stopLang();
}

void ScatePlugin::switchServer( bool on )
{
  if( on ) startServer();
  else stopServer();
}

void ScatePlugin::switchSwingOsc( bool on )
{
  if( on ) startSwingOSC();
  else stopSwingOSC();
}

void ScatePlugin::stopProcessing()
{
  eval( "thisProcess.stop;", true );
  sysMsg( "All processing stopped." );
}

void ScatePlugin::switchToQt() {
  eval( "GUI.qt" );
}

void ScatePlugin::switchToSwing() {
  eval( "GUI.swing" );
}

void ScatePlugin::eval( const QString& cmd, bool silent )
{
  if( !langRunning() ) {
    sysMsg( "Interpreter not running!" );
    return;
  }

  QString str = cmd + ( silent ? "\x1b" : "\x0c" );
  scProcess->write( str.toAscii() );
}

void ScatePlugin::restartLang()
{
  restart = true;
  stopLang();
}

bool ScatePlugin::langRunning()
{ return scProcess->state() != QProcess::NotRunning; }

bool ScatePlugin::serverRunning()
{ return false; }
