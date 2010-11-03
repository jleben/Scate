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
#include "ScateHelpBrowser.hpp"

#include <kaction.h>
#include <kactioncollection.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <klocalizedstring.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QKeyEvent>

ScateView::ScateView( ScatePlugin *plugin_, Kate::MainWindow *mainWin )
    : Kate::PluginView( mainWin ),
    plugin( plugin_ ),
    outputToolView(0),
    helpToolView(0),
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
  delete outputToolView;
  delete helpToolView;
}

void ScateView::createOutputView()
{
  if( outputToolView ) return;

  outputToolView = mainWindow()->createToolView(
    "SC Output",
    Kate::MainWindow::Bottom,
    QPixmap( plugin->iconPath() ),
    "SC Output"
  );

  QWidget *w = new QWidget (outputToolView);
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

  l->addWidget( cmdLine );
}

void ScateView::createHelpView()
{
  if( helpToolView ) return;

  helpToolView = mainWindow()->createToolView (
    "SC Help",
    Kate::MainWindow::Bottom,
    QPixmap( plugin->iconPath() ),
    "SC Help"
  );

  helpWidget = new ScateHelpBrowser( helpToolView );
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
  QString text;

  if( helpWidget && helpWidget->webViewFocused() ) {
    text = helpWidget->selectedText();
  }
  else {
    KTextEditor::View *view = mainWindow()->activeView();
    if( view->selection() )
        text = view->selectionText();
    else
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
      if( helpWidget->findHelpFor( text ) ) {
        mainWindow()->showToolView( helpToolView );
      }
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

ScateCmdLine::ScateCmdLine()
  : curHistory( -1 )
{
  QHBoxLayout *l = new QHBoxLayout;
  l->setContentsMargins(5,0,0,0);
  setLayout( l );

  QLabel *lbl = new QLabel("Execute:");
  expr = new QLineEdit;

  l->addWidget(lbl);
  l->addWidget(expr);

  expr->installEventFilter( this );
}

bool ScateCmdLine::eventFilter( QObject *, QEvent *e )
{
  int type = e->type();
  if( type != QEvent::KeyPress ) return false;

  QKeyEvent *ke = static_cast<QKeyEvent*>(e);

  switch( ke->key() )
  {
    case Qt::Key_Return:
    case Qt::Key_Enter:

      if( expr->text().isEmpty() ) return true;

      emit invoked( expr->text(), false );
      if( history.count() == 0 || history[0] != expr->text() )
      {
        if( history.count() > 30 ) history.removeAt( history.count() - 1 );
        history.prepend( expr->text() );
      }
      curHistory = -1;
      expr->clear();
      return true;

    case Qt::Key_Up:
      if( curHistory < history.count() - 1 ) {
          expr->blockSignals(true);
          expr->setText( history[++curHistory] );
          expr->blockSignals(false);
      }
      return true;

    case Qt::Key_Down:
      if( curHistory > -1 ) {
          --curHistory;
          expr->blockSignals(true);
          if( curHistory == -1 ) expr->clear();
          else expr->setText( history[curHistory] );
          expr->blockSignals(false);
      }
      return true;

    default: return false;
  }
}
