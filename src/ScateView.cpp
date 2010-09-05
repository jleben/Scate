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

#include "ScateView.hpp"
#include "ScatePlugin.hpp"

#include <kaction.h>
#include <kactioncollection.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <khtmlview.h>

#include <QVBoxLayout>
#include <QToolBar>
#include <QDir>
#include <QDirIterator>
#include <QMessageBox>
#include <QApplication>

ScateView::ScateView( ScatePlugin *plugin_, Kate::MainWindow *mainWin )
    : Kate::PluginView( mainWin ),
    plugin( plugin_ ),
    outputView(0),
    helpView(0),
    helpWidget(0)
{
  setComponentData( ScatePluginFactory::componentData() );
  setXMLFile( "kate/plugins/katescate/ui.rc" );

  //TODO lazy creation
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
  QStringList helpDirs = config.readPathEntry( "HelpDirs", QStringList() );

  if( !helpDirs.count() ) {
    warnSetHelpDir();
    return;
  }

  foreach( QString dirName, helpDirs ) {
    QDir dir(dirName);
    if( dir.exists("Help.html") ) {
      KUrl helpUrl( dirName );
      helpUrl.addPath( "Help.html" );
      _history->goTo( helpUrl );
      return;
    }
  }

  QString msg("Could not find help homepage in any of given help directories.");
  QMessageBox::warning( this, "SuperCollider Help", msg );
}

void ScateHelpWidget::goToClass( const QString & className )
{
  KConfigGroup config(KGlobal::config(), "Scate");
  QStringList helpDirNames = config.readPathEntry( "HelpDirs", QStringList() );

  if( !helpDirNames.count() ) {
    warnSetHelpDir();
    return;
  }

  QStringList filters;
  filters << ( className + ".html" );

  KUrl url;

  foreach( QString helpDirName, helpDirNames ) {
    qDebug() << tr("searching for help in: %1").arg(helpDirName);
    QDir helpDir( helpDirName );
    helpDir.setNameFilters( filters );

    QDirIterator iter( helpDir, QDirIterator::Subdirectories );
    if( iter.hasNext() ) {
      QString result = iter.next();
      qDebug() << tr("help search result: %1").arg( result );
      url = KUrl( iter.filePath() );
      break;
    }
    QApplication::processEvents();
  }

  if( !url.isValid() ) {
    QString msg = tr("No help file for class '%1' found.").arg( className );
    QMessageBox::information( this, "SuperCollider Help", msg );
  }
  else {
    _history->goTo( url );
  }
}

void ScateHelpWidget::warnSetHelpDir()
{
  QString msg( "Please set at least one SuperCollider Help directory"
               " on the Scate configuration panel." );
  QMessageBox::warning( this, "SuperCollider Help", msg );
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
