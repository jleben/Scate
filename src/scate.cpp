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
#include <khtmlview.h>

#include <QFormLayout>
#include <QVBoxLayout>
#include <QToolBar>
#include <QToolButton>
#include <QKeyEvent>
#include <QLabel>
#include <QDir>
#include <QDirIterator>
#include <QMessageBox>

#define DEFAULT_DATA_DIR ""

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


/*************************** PLUGIN ***************************/

K_PLUGIN_FACTORY(ScatePluginFactory, registerPlugin<ScatePlugin>();)
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
  QString rtDir = config.readEntry( "RuntimeDataDir", DEFAULT_DATA_DIR );

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

/*********************** VIEW ******************************/

ScateView::ScateView( ScatePlugin *plugin_, Kate::MainWindow *mainWin )
    : Kate::PluginView( mainWin ),
    plugin( plugin_ ),
    outputView(0),
    helpView(0),
    helpWidget(0)
{
  setComponentData( ScatePluginFactory::componentData() );
  setXMLFile( "kate/plugins/katescate/ui.rc" );

  createOutputView();
  createHelpView();

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

  a = actionCollection()->addAction( "scate_class_help" );
  a->setText( i18n("Find Help for Class") );
  a->setShortcut( Qt::CTRL | Qt::Key_H );
  connect( a, SIGNAL( triggered(bool) ), this, SLOT( helpForSelectedClass() ) );

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
  delete outputView;
  delete helpView;
}

void ScateView::createOutputView()
{
  if( outputView ) return;

  outputView = mainWindow()->createToolView(
    "SC Output",
    Kate::MainWindow::Bottom,
    QPixmap( plugin->iconPath() ),
    "SC Output"
  );

  QWidget *w = new QWidget (outputView);
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
}

void ScateView::createHelpView()
{
  if( helpView ) return;

  QWidget *helpView = mainWindow()->createToolView (
    "SC Help",
    Kate::MainWindow::Bottom,
    QPixmap( plugin->iconPath() ),
    "SC Help"
  );

  helpWidget = new ScateHelpWidget( helpView );
  helpWidget->goHome();
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
      text = view->document()->line( view->cursorPosition().line() );
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

void ScateView::helpForSelectedClass()
{
  KTextEditor::View *view = mainWindow()->activeView();
  if( view->selection() )
  {
      QString text = view->selectionText();
      helpWidget->goToClass( text );
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

ScateUrlHistory::ScateUrlHistory( QObject *parent ) :
  QObject( parent ),
  curIndex(0)
{
  QAction *a;

  a = KStandardAction::back( this, SLOT(goBack()), this );
  _actions.append( a );

  a = KStandardAction::forward( this, SLOT(goForward()), this );
  _actions.append( a );
}

QList<QAction*> ScateUrlHistory::actions()
{
  return _actions;
}

void ScateUrlHistory::goBack()
{
  if( history.count() && curIndex > 0 ) {
    curIndex--;
    emit wentTo( history[curIndex] );
  }
}

void ScateUrlHistory::goForward()
{
  if( curIndex < history.count() - 1 ) {
    curIndex++;
    emit wentTo( history[curIndex] );
  }
}

void ScateUrlHistory::goTo( const KUrl &url  )
{
  int c = history.count();
  if( c && history[curIndex] == url ) return;

  c--;
  while( c > curIndex ) {
    history.removeLast();
    c--;
  }

  history.append( url );

  if( history.count() >= 25 ) {
    history.removeFirst();
  }

  curIndex = history.count() - 1;

  emit wentTo( history[curIndex] );
}

void ScateUrlHistory::print()
{
  printf("URLS are:\n");
  Q_FOREACH( KUrl url, history ) {
    printf("%s\n", url.url().toStdString().c_str());
  }
}


ScateHelpWidget::ScateHelpWidget( QWidget * parent ) :
  QWidget( parent ),
  _history( new ScateUrlHistory( this ) )
{
  QVBoxLayout *box = new QVBoxLayout;
  box->setContentsMargins(0,0,0,0);
  box->setSpacing(0);
  setLayout(box);

  QToolBar *toolBar = new QToolBar;

  QAction *a = KStandardAction::home( this, SLOT(goHome()), this );
  toolBar->addAction(a);

  QList<QAction*> historyActions = _history->actions();
  foreach( QAction *a, historyActions ) {
    toolBar->addAction( a );
  }

  box->addWidget( toolBar );

  browser = new KHTMLPart();
  browser->view()->setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
  box->addWidget( browser->view() );

  connect( browser->browserExtension(),
            SIGNAL( openUrlRequest(const KUrl &,
                                    const KParts::OpenUrlArguments &,
                                    const KParts::BrowserArguments & ) ),
            _history,
            SLOT( goTo(const KUrl &) )
          );
  connect( _history, SIGNAL(wentTo(const KUrl &)),
           this, SLOT(openUrl(const KUrl &)) );
}

void ScateHelpWidget::openUrl( const KUrl &url )
{
  browser->openUrl( url );
}

void ScateHelpWidget::goHome()
{
  KConfigGroup config(KGlobal::config(), "Scate");
  QString helpDir = config.readEntry( "HelpDir", QString() );

  if( helpDir.isEmpty() ) return;

  KUrl helpUrl( helpDir );
  helpUrl.addPath( "Help.html" );
  _history->goTo( helpUrl );
}

void ScateHelpWidget::goToClass( const QString & className )
{
  KConfigGroup config(KGlobal::config(), "Scate");
  QString helpDirName = config.readEntry( "HelpDir", QString() );

  if( helpDirName.isEmpty() ) return;

  QStringList filters;
  filters << ( className + ".html" );

  QDir helpDir( helpDirName );
  helpDir.setNameFilters( filters );

  QDirIterator iter( helpDir, QDirIterator::Subdirectories );
  if( iter.hasNext() ) {
    _history->goTo( KUrl( iter.filePath() ) );
  }
  else {
    QString msg = tr("No help file for class '%1' found.").arg( className );
    QMessageBox::warning( this, "Class help search", msg );
  }
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
          blockSignals(true);
          setText( history[++curHistory] );
          blockSignals(false);
      }
      break;

    case Qt::Key_Down:
      if( curHistory > -1 ) {
          --curHistory;
          blockSignals(true);
          if( curHistory == -1 ) clear();
          else setText( history[curHistory] );
          blockSignals(false);
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
  layout->addRow( new QLabel( i18n( "Sclang command" ) ),
                  sclangExeEdit );

  dataDirEdit = new QLineEdit();
  layout->addRow( new QLabel( i18n( "Runtime data directory" ) ),
                  dataDirEdit );

  helpDirEdit = new QLineEdit();
  layout->addRow( new QLabel( i18n( "SuperCollider Help directory" ) ),
                  helpDirEdit );

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
  config.writeEntry( "HelpDir", helpDirEdit->text() );
  config.writeEntry( "SwingOscProgram", swingOscDirEdit->text() );
  config.writeEntry( "StartLang", startLangCheck->isChecked() );
  config.sync();
}

void ScateConfigPage::reset()
{
  KConfigGroup config(KGlobal::config(), "Scate");
  sclangExeEdit->setText( config.readEntry( "ScLangExecutable", QString() ) );
  dataDirEdit->setText( config.readEntry( "RuntimeDataDir", DEFAULT_DATA_DIR ) );
  helpDirEdit->setText( config.readEntry( "HelpDir", QString() ) );
  swingOscDirEdit->setText( config.readEntry( "SwingOscProgram", QString() ) );
  startLangCheck->setChecked( config.readEntry( "StartLang", false ) );
}

void ScateConfigPage::defaults()
{
  sclangExeEdit->clear();
  dataDirEdit->setText( DEFAULT_DATA_DIR );
  helpDirEdit->clear();
  swingOscDirEdit->clear();
  startLangCheck->setChecked( false );

  KConfigGroup config(KGlobal::config(), "scate");
  config.writeEntry( "ScLangExecutable", QString() );
  config.writeEntry( "RuntimeDataDir", DEFAULT_DATA_DIR );
  config.writeEntry( "HelpDir", QString() );
  config.writeEntry( "SwingOscProgram", QString() );
  config.writeEntry( "StartLang", false );
}
