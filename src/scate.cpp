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


#include "scate.h"
//#include "build/katescate.moc" //TODO necessary?

#include <kate/application.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

#include <kaction.h>
#include <kactioncollection.h>
#include <klocale.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>
#include <KConfigGroup>
#include <kstandarddirs.h>

#include <signal.h>
#include <wait.h>

#include <QFormLayout>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QLabel>

#define DEFAULT_DATA_DIR ""

#define READ 0
#define WRITE 1

pid_t
popen2(const char *command, int *infp, int *outfp)
{
  int p_stdin[2], p_stdout[2];
  pid_t pid;

  if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0)
  return -1;

  pid = fork();
  if (pid < 0)
  return pid;
  else if (pid == 0)
  {
    setpgid( getpid(), getpid() );
    close(p_stdin[WRITE]);
    dup2(p_stdin[READ], READ);
    close(p_stdout[READ]);
    dup2(p_stdout[WRITE], WRITE);
    execl("/bin/sh", "sh", "-c", command, NULL);
    perror("execl");
    printf("child exiting\n");
    exit(1);
  }

  if (infp == NULL)
  {
    close(p_stdin[WRITE]);
    close(p_stdin[READ]);
  }
  else
  {
    close(p_stdin[READ]);
    *infp = p_stdin[WRITE];
  }
  if (outfp == NULL)
  {
    close(p_stdout[WRITE]);
    close(p_stdout[READ]);
  }
  else
  {
    close(p_stdout[WRITE]);
    *outfp = p_stdout[READ];
  }
  return pid;
}

#undef READ
#undef WRITE

/*************************** PLUGIN ***************************/

K_PLUGIN_FACTORY(ScatePluginFactory, registerPlugin<ScatePlugin>();)
K_EXPORT_PLUGIN( ScatePluginFactory( KAboutData("katescateplugin", 0,
                                                ki18n("Scate"),
                                                "0.9.2",
                                                ki18n("Kate SuperCollider plugin") ) ) )

ScatePlugin::ScatePlugin( QObject* parent, const QList<QVariant>& )
    : Kate::Plugin( (Kate::Application*)parent, "kate-scate-plugin" ),
    scThread( 0 ),
    scPid( 0 ),
    pipeW( 0 ),
    pipeR( 0 ),
    _iconPath( KStandardDirs::locate( "data", "kate/plugins/katescate/supercollider.png" ) )
{
  KConfigGroup config(KGlobal::config(), "Scate");
  bool b_startLang = config.readEntry( "StartLang", false );
  if( b_startLang )
    startLang();
}

ScatePlugin::~ScatePlugin()
{
  stopLang();
  cleanup();
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
  if( scThread )
  {
      printf("already running\n");
      return;
  }

  KConfigGroup config(KGlobal::config(), "Scate");
  QString exe = config.readEntry( "ScLangExecutable", QString() );
  QString rtDir = config.readEntry( "RuntimeDataDir", DEFAULT_DATA_DIR );

  QString cmd = exe.isEmpty() ? tr( "sclang" ) : exe;
  cmd += tr( " -i scate" );
  if( !rtDir.isEmpty() ) cmd.append( " -d " ).append( rtDir );

  printf("Trying to start with command:\n");
  printf( "%s\n", cmd.toStdString().c_str() );

  if ( ( scPid = popen2( cmd.toStdString().c_str(), &pipeW, &pipeR) ) <= 0)
  {
      printf("Unable to exec sc!\n");
      return;
  }
  scThread = new SCThread( pipeR );
  connect( scThread, SIGNAL( scSays( const QString& ) ),
           this, SIGNAL( scSaid( const QString& ) ), Qt::QueuedConnection );
  connect( scThread, SIGNAL( finished() ), this, SLOT( cleanup() ) );
  scThread->start();
  emit( langSwitched( true ) );
}

void ScatePlugin::stopLang()
{
  if( scThread && scThread->isRunning() )
  {
      if( scPid != 0 )
      {
        printf("killing SC\n");
        int pid = scPid;
        scPid = 0;
        if( killpg( pid, SIGINT ) == -1 )
          printf("could not kill SC!\n");
        else
          waitpid( pid, NULL, 0 );
      }
      printf("terminated sclang\n");
      scThread->wait();
      printf("SC thread finished\n");
      cleanup();
  }
  else
      printf("not running\n");
}

void ScatePlugin::cleanup()
{
  if( !scThread || scThread->isRunning() ) return;

  if( scPid != 0 )
  {
    printf("killing SC\n");
    if( killpg( scPid, SIGINT ) == -1 )
      printf("could not kill SC!\n");
    else
      waitpid( scPid, NULL, 0 );
  }

  close( pipeR );
  close( pipeW );
  delete scThread;
  scThread = 0;
  scPid = pipeR = pipeW = 0;
  emit( langSwitched( false ) );
  emit( serverSwitched( false ) );
  emit scSaid("\nInterpreter stopped.\n");
  printf("cleaned up\n");
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
  emit scSaid( "\nAll processing stopped\n" );
}

void ScatePlugin::eval( const QString& cmd, bool silent )
{
  if( !langRunning() ) {
    emit scSaid( "\nInterpreter not running!\n" );
    return;
  }

  QString str = cmd + ( silent ? "\x1b" : "\x0c" );
  int written = write( pipeW, str.toStdString().c_str(), str.length() );
}

void ScatePlugin::restartLang()
{
  stopLang();
  emit scSaid( "\n" );
  startLang();
}

bool ScatePlugin::langRunning()
{ return scThread && scThread->isRunning(); }

bool ScatePlugin::serverRunning()
{ return false; }

SCThread::SCThread( int p ) : pipe(p) { pipe = p;}

void SCThread::run()
{
  printf("starting to read SC pipe\n");
  char buf[128];
  memset (buf, 0x0, sizeof(buf));
  int bytesRead;
  while( ( bytesRead = read(pipe, buf, 127) ) > 0 ) {
    buf[bytesRead] = 0;
    emit scSays( buf );
  }
  printf("finished reading SC pipe\n");
}

/*********************** VIEW ******************************/

ScateView::ScateView( ScatePlugin *plugin_, Kate::MainWindow *mainWin )
    : Kate::PluginView( mainWin ),
    plugin( plugin_ )
{
  setComponentData( ScatePluginFactory::componentData() );
  setXMLFile( "kate/plugins/katescate/ui.rc" );

  toolView = mainWindow()->createToolView(
    "Scate",
    Kate::MainWindow::Bottom,
    QPixmap( plugin->iconPath() ),
    "SuperCollider" );

  QWidget *w = new QWidget (toolView);
  QLayout *l = new QVBoxLayout;
  l->setContentsMargins(0,0,0,0);
  l->setSpacing(0);
  w->setLayout(l);

  scOutView = new QTextEdit;
  scOutView->setReadOnly( true );
  connect( plugin, SIGNAL( scSaid( const QString& ) ),
           this, SLOT( scSaid( const QString& ) ) );
  l->addWidget( scOutView );

  cmdLine = new ScateCmdLine;
  connect( cmdLine, SIGNAL( invoked( const QString&, bool ) ),
           plugin, SLOT( eval( const QString&, bool ) ) );
  /*cmdLine = new QLineEdit;
  connect( cmdLine, SIGNAL( returnPressed() ),
           this, SLOT( evaluateCmdLine() ) );*/
  l->addWidget( cmdLine );

  KAction *a;

  a = actionCollection()->addAction( "scate_switch_lang" );
  a->setCheckable( true );
  a->setText( i18n("Interpreter Running") );
  connect( a, SIGNAL( triggered(bool) ), plugin, SLOT( switchLang(bool) ) );
  connect( plugin, SIGNAL( langSwitched(bool) ), this, SLOT( langStatusChanged(bool) ) );
  aLangSwitch = a;

  a = actionCollection()->addAction( "scate_restart_lang" );
  a->setText( i18n("Restart Interpreter") );
  connect( a, SIGNAL( triggered(bool) ), plugin, SLOT( restartLang() ) );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_start_server" );
  a->setText( i18n("Start Synth Server") );
  connect( a, SIGNAL( triggered(bool) ), plugin, SLOT( startServer() ) );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_stop_server" );
  a->setText( i18n("Stop Synth Server") );
  connect( a, SIGNAL( triggered(bool) ), plugin, SLOT( stopServer() ) );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_start_swingosc" );
  a->setText( i18n("Start SwingOSC GUI Server") );
  connect( a, SIGNAL( triggered(bool) ), plugin, SLOT( startSwingOSC() ) );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_stop_swingosc" );
  a->setText( i18n("Stop SwingOSC GUI Server") );
  connect( a, SIGNAL( triggered(bool) ), plugin, SLOT( stopSwingOSC() ) );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_evaluate" );
  a->setText( i18n("Evaluate") );
  a->setShortcut( Qt::CTRL | Qt::Key_E );
  connect( a, SIGNAL( triggered(bool) ), this, SLOT( evaluateSelection() ) );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_browse_class" );
  a->setText( i18n("Show in Class Browser") );
  a->setShortcut( Qt::CTRL | Qt::Key_B );
  connect( a, SIGNAL( triggered(bool) ), this, SLOT( browseSelectedClass() ) );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_stop_proc" );
  a->setText( i18n("Stop All Processing") );
  a->setShortcut( Qt::Key_Escape );
  connect( a, SIGNAL( triggered(bool) ), plugin, SLOT( stopProcessing() ) );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_clear" );
  a->setText( i18n("Clear Output") );
  connect( a, SIGNAL( triggered(bool) ), scOutView, SLOT( clear() ) );

  mainWindow()->guiFactory()->addClient( this );

  //check and enable actions according to interpreter status
  langStatusChanged( plugin->langRunning() );
}

ScateView::~ScateView()
{
  mainWindow()->guiFactory()->removeClient( this );
  delete toolView;
}

void ScateView::langStatusChanged( bool b_switch )
{
  aLangSwitch->setChecked( b_switch );
  foreach ( QAction *a, langDepActions ) {
    a->setEnabled( b_switch );
  }
}

void ScateView::scSaid( const QString& str )
{
  /*QCursor cursor( scOutView->document()->end() );
  cursor->insertText( str );*/
  scOutView->moveCursor( QTextCursor::End );
  scOutView->insertPlainText( str );
  scOutView->ensureCursorVisible();
}

void ScateView::evaluateSelection()
{
  KTextEditor::View *view = mainWindow()->activeView();
  QString text;
  if( view->selection() )
      text = view->selectionText();
  else
  {
      //text = view->document()->line( view->cursorPosition().line() );
  }
  if( !text.isEmpty() ) plugin->eval( text );
}

void ScateView::browseSelectedClass()
{
  KTextEditor::View *view = mainWindow()->activeView();
  if( view->selection() )
  {
      QString text = view->selectionText();
      plugin->eval( text + QString(".browse;"), true );
  }
}

void ScateView::readSessionConfig( KConfigBase* config, const QString& groupPrefix )
{
  // If you have session-dependant settings, load them here.
  // If you have application wide settings, you have to read your own KConfig,
  // see the Kate::Plugin docs for more information.
  Q_UNUSED( config );
  Q_UNUSED( groupPrefix );
}

void ScateView::writeSessionConfig( KConfigBase* config, const QString& groupPrefix )
{
  // If you have session-dependant settings, save them here.
  // If you have application wide settings, you have to create your own KConfig,
  // see the Kate::Plugin docs for more information.
  Q_UNUSED( config );
  Q_UNUSED( groupPrefix );
}

void ScateView::evaluateCmdLine()
{
  plugin->eval( cmdLine->text(), false );
  cmdLine->clear();
}


ScateCmdLine::ScateCmdLine()
  : curHistory( -1 )
{}

void ScateCmdLine::keyPressEvent( QKeyEvent *e )
{
  switch( e->key() )
  {
    case Qt::Key_Return:
    case Qt::Key_Enter:

      if( text().isEmpty() ) break;

      emit invoked( text(), false );
      if( history.count() == 0 || history[0] != text() )
      {
        if( history.count() > 30 ) history.removeAt( history.count() - 1 );
        history.prepend( text() );
      }
      curHistory = -1;
      clear();
      break;

    case Qt::Key_Up:
      if( curHistory < history.count() - 1 ) {
          setText( history[++curHistory] );
      }
      break;

    case Qt::Key_Down:
      if( curHistory > -1 ) {
          --curHistory;
          if( curHistory == -1 ) clear();
          else setText( history[curHistory] );
      }
      break;

    default:
      QLineEdit::keyPressEvent( e );
  }
}

/*********************** CONFIG ******************************/

ScateConfigPage::ScateConfigPage( ScatePlugin *, QWidget *parent )
  : Kate::PluginConfigPage( parent )
{
  QFormLayout *layout = new QFormLayout( this );

  sclangExeEdit = new QLineEdit();
  layout->addRow( new QLabel( i18n( "SC Lang program (full path)" ) ),
                  sclangExeEdit );

  dataDirEdit = new QLineEdit();
  layout->addRow( new QLabel( i18n( "Runtime data directory" ) ),
                  dataDirEdit );

  swingOscDirEdit = new QLineEdit();
  layout->addRow( new QLabel( i18n( "SwingOSC program filename" ) ),
                  swingOscDirEdit );

  startLangCheck = new QCheckBox( );
  layout->addRow( new QLabel( i18n( "Start interpreter with plugin" ) ),
                  startLangCheck );
  reset();
}

void ScateConfigPage::apply()
{
  KConfigGroup config(KGlobal::config(), "Scate");
  config.writeEntry( "ScLangExecutable", sclangExeEdit->text() );
  config.writeEntry( "RuntimeDataDir", dataDirEdit->text() );
  config.writeEntry( "SwingOscProgram", swingOscDirEdit->text() );
  config.writeEntry( "StartLang", startLangCheck->isChecked() );
  config.sync();
}

void ScateConfigPage::reset()
{
  KConfigGroup config(KGlobal::config(), "Scate");
  sclangExeEdit->setText( config.readEntry( "ScLangExecutable", QString() ) );
  dataDirEdit->setText( config.readEntry( "RuntimeDataDir", DEFAULT_DATA_DIR ) );
  swingOscDirEdit->setText( config.readEntry( "SwingOscProgram", QString() ) );
  startLangCheck->setChecked( config.readEntry( "StartLang", false ) );
}

void ScateConfigPage::defaults()
{
  sclangExeEdit->clear();
  dataDirEdit->setText( DEFAULT_DATA_DIR );
  swingOscDirEdit->clear();
  startLangCheck->setChecked( false );

  KConfigGroup config(KGlobal::config(), "scate");
  config.writeEntry( "ScLangExecutable", QString() );
  config.writeEntry( "RuntimeDataDir", DEFAULT_DATA_DIR );
  config.writeEntry( "SwingOscProgram", QString() );
  config.writeEntry( "StartLang", false );
}
