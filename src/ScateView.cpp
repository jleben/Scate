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

using namespace Scate;

ScateView::ScateView( ScatePlugin *plugin_, Kate::MainWindow *mainWin )
    : Kate::PluginView( mainWin ),
    plugin( plugin_ ),
    outputToolView(0),
    maxOutBlocks(500),
    helpToolView(0),
    helpWidget(0)
{
  setComponentData( ScatePluginFactory::componentData() );
  setXMLFile( "kate/plugins/katescate/ui.rc" );

  //TODO lazy creation
  outputToolView = createOutputView();
  helpToolView = createHelpView();

  KAction *a;

  a = actionCollection()->addAction( "scate_lang_switch" );
  a->setCheckable( true );
  a->setIcon( KIcon("system-shutdown") );
  a->setText( i18n("Boot") );
  connect( a, SIGNAL( triggered(bool) ), plugin, SLOT( switchLang(bool) ) );
  connect( plugin, SIGNAL( langSwitched(bool) ), this, SLOT( langStatusChanged(bool) ) );
  aLangSwitch = a;

  a = actionCollection()->addAction( "scate_lang_restart" );
  a->setIcon( KIcon("system-reboot") );
  a->setText( i18n("Reboot") );
  connect( a, SIGNAL( triggered(bool) ), plugin, SLOT( restartLang() ) );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_synth_start" );
  a->setText( i18n("Boot Synth") );
  connect( a, SIGNAL( triggered(bool) ), plugin, SLOT( startServer() ) );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_synth_stop" );
  a->setText( i18n("Shutdown Synth") );
  connect( a, SIGNAL( triggered(bool) ), plugin, SLOT( stopServer() ) );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_gui_qt", plugin, SLOT(switchToQt()) );
  a->setText( i18n("Qt") );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_gui_swing", plugin, SLOT(switchToSwing()) );
  a->setText( i18n("SwingOSC") );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_swing_start" );
  a->setText( i18n("Boot SwingOSC Server") );
  connect( a, SIGNAL( triggered(bool) ), plugin, SLOT( startSwingOSC() ) );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_swing_stop" );
  a->setText( i18n("Shutdown SwingOSC Server") );
  connect( a, SIGNAL( triggered(bool) ), plugin, SLOT( stopSwingOSC() ) );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_evaluate" );
  a->setIcon( KIcon("media-playback-start") );
  a->setText( i18n("Execute") );
  a->setShortcut( Qt::CTRL | Qt::Key_E );
  connect( a, SIGNAL( triggered(bool) ), this, SLOT( evaluateSelection() ) );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_stop_proc" );
  a->setIcon( KIcon("media-playback-stop") );
  a->setText( i18n("Stop") );
  a->setShortcut( Qt::Key_Escape );
  connect( a, SIGNAL( triggered(bool) ), plugin, SLOT( stopProcessing() ) );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_clear" );
  a->setIcon( KIcon("window-close") );
  a->setText( i18n("Clear Output") );
  connect( a, SIGNAL( triggered(bool) ), scOutView, SLOT( clear() ) );

  a = actionCollection()->addAction( "scate_browse_class" );
  a->setIcon( KIcon("help-browser") );
  a->setText( i18n("Class Reference for Selection") );
  a->setShortcut( Qt::CTRL | Qt::Key_B );
  connect( a, SIGNAL( triggered(bool) ), this, SLOT( browseSelectedClass() ) );
  langDepActions.append(a);

  a = actionCollection()->addAction( "scate_help" );
  a->setIcon( KIcon("system-help") );
  a->setText( i18n("Help for Selection") );
  a->setShortcut( Qt::CTRL | Qt::Key_H );
  connect( a, SIGNAL( triggered(bool) ), this, SLOT( helpForSelectedClass() ) );

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

QWidget * ScateView::createOutputView()
{
  QWidget *toolView = mainWindow()->createToolView(
    "SC Terminal",
    Kate::MainWindow::Right,
    QPixmap( plugin->iconPath() ),
    "SC Terminal"
  );

  scOutView = new QTextEdit;
  scOutView->setReadOnly( true );
  scOutView->document()->setMaximumBlockCount( maxOutBlocks );
  connect( plugin, SIGNAL( scSaid( const QString& ) ),
           this, SLOT( scSaid( const QString& ) ) );

  cmdLine = new CmdLine( "Code:", 30 );
  connect( cmdLine, SIGNAL( invoked( const QString&, bool ) ),
           plugin, SLOT( eval( const QString&, bool ) ) );

  QLayout *l = new QVBoxLayout;
  l->setContentsMargins(0,0,0,0);
  l->setSpacing(0);
  l->addWidget( scOutView );
  l->addWidget( cmdLine );

  QWidget *w = new QWidget (toolView);
  w->setLayout(l);

  return toolView;
}

QWidget * ScateView::createHelpView()
{
  QWidget *toolView = mainWindow()->createToolView (
    "SC Help",
    Kate::MainWindow::Right,
    QPixmap( plugin->iconPath() ),
    "SC Help"
  );

  helpWidget = new ScateHelpBrowser( toolView );

  return toolView;
}

void ScateView::langStatusChanged( bool b_switch )
{
  aLangSwitch->setChecked( b_switch );
  if( b_switch )
    aLangSwitch->setText( i18n("Shutdown") );
  else
    aLangSwitch->setText( i18n("Boot") );

  foreach ( QAction *a, langDepActions ) {
    a->setEnabled( b_switch );
  }
}

void ScateView::scSaid( const QString& str )
{
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
