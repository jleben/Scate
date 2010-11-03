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

#include "ScateHelpBrowser.hpp"

#include <kaction.h>
#include <kactioncollection.h>
#include <kconfiggroup.h>

#include <QVBoxLayout>
#include <QToolBar>
#include <QPushButton>
#include <QLabel>
#include <QWebView>
#include <QDir>
#include <QDirIterator>
#include <QMessageBox>
#include <QDebug>
#include <QApplication>
#include <QShortcut>
#include <QKeyEvent>


ScateHelpBrowser::ScateHelpBrowser( QWidget *parent )
: QWidget( parent ), findBar(0), virgin( true )
{
  webView = new QWebView();

  ScateHelpSearchBar *searchBar = new ScateHelpSearchBar();

  QToolBar *toolBar = new QToolBar;

  QAction *a = KStandardAction::home( this, SLOT(goHome()), this );
  toolBar->addAction(a);
  toolBar->addAction( webView->pageAction( QWebPage::Back ) );
  toolBar->addAction( webView->pageAction( QWebPage::Forward ) );
  toolBar->addWidget( searchBar );

  QVBoxLayout *l = new QVBoxLayout();
  l->setContentsMargins(0,0,0,0);
  l->setSpacing(2);
  l->addWidget( toolBar );
  l->addWidget( webView );

  setLayout(l);

  QShortcut *copyShortcut = new QShortcut( QKeySequence::Copy, webView, 0, 0,
                                           Qt::WidgetWithChildrenShortcut );
  QShortcut *findShortcut = new QShortcut( QKeySequence::Find, this,
                                           SLOT(newTextSearch()), SLOT(newTextSearch()),
                                           Qt::WidgetWithChildrenShortcut );

  Q_UNUSED(findShortcut);

  connect( searchBar, SIGNAL(searchedFor(const QString&)),
           this, SLOT(findHelpFor(const QString&)) );
  connect( copyShortcut, SIGNAL(activated()),
           webView->pageAction( QWebPage::Copy ), SLOT(trigger()) );
}

void ScateHelpBrowser::findText( const QString& str, QWebPage::FindFlags flags )
{
  webView->findText( str, flags );
}

void ScateHelpBrowser::goHome()
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
      QUrl helpUrl( dir.absoluteFilePath("Help.html") );
      webView->load( helpUrl );
      return;
    }
  }

  QString msg("Could not find help homepage in any of given help directories.");
  QMessageBox::warning( this, "SuperCollider Help", msg );
}

void ScateHelpBrowser::newTextSearch()
{
  if( !findBar ) {
    findBar = new ScateFindBar();
    layout()->addWidget( findBar );

    QShortcut *escShortcut = new QShortcut( Qt::Key_Escape, this, 0, 0,
                                            Qt::WidgetWithChildrenShortcut );
    connect( findBar, SIGNAL(activated(const QString&,QWebPage::FindFlags)),
              this, SLOT(findText(const QString&,QWebPage::FindFlags)) );
    Q_UNUSED(escShortcut);
  }

  findBar->setText( webView->selectedText() );
  findBar->show();
}

bool ScateHelpBrowser::findHelpFor( const QString & className )
{

  KConfigGroup config(KGlobal::config(), "Scate");
  QStringList helpDirNames = config.readPathEntry( "HelpDirs", QStringList() );

  if( !helpDirNames.count() ) {
    warnSetHelpDir();
    return false;
  }

  QStringList filters;
  filters << ( className + ".html" );

  QUrl url;

  foreach( QString helpDirName, helpDirNames ) {
    qDebug() << tr("searching for help in: %1").arg(helpDirName);
    QDir helpDir( helpDirName );
    helpDir.setNameFilters( filters );

    QDirIterator iter( helpDir, QDirIterator::Subdirectories );
    if( iter.hasNext() ) {
      QString result = iter.next();
      qDebug() << tr("help search result: %1").arg( result );
      url = QUrl( iter.filePath() );
      break;
    }
    QApplication::processEvents();
  }

  if( url.isEmpty() ) {
    QString msg = tr("No help file for class '%1' found.").arg( className );
    QMessageBox::information( this, "SuperCollider Help", msg );
    return false;
  }
  else {
    webView->load( url );
    return true;
  }
}

void ScateHelpBrowser::warnSetHelpDir()
{
  QString msg( "Please set at least one SuperCollider Help directory"
               " on the Scate configuration panel." );
  QMessageBox::warning( this, "SuperCollider Help", msg );
}

void ScateHelpBrowser::showEvent( QShowEvent *e )
{
  Q_UNUSED(e);
  if( virgin && webView->url().isEmpty() ) {
    virgin = false;
    QMetaObject::invokeMethod( this, "goHome", Qt::QueuedConnection );
  }
}

bool ScateHelpBrowser::event( QEvent *e )
{
  if( e->type() == QEvent::ShortcutOverride ) {
    QKeyEvent *ke = static_cast<QKeyEvent*>(e);
    if( ke->matches( QKeySequence::Find ) ) {
      newTextSearch();
      e->accept();
      return true;
    }
    if( ke->key() == Qt::Key_Escape && findBar ) {
      webView->setFocus();
      findBar->hide();
      e->accept();
      return true;
    }
  }
  return QWidget::event( e );
}

ScateHelpSearchBar::ScateHelpSearchBar()
{
  searchField = new QLineEdit();
  QPushButton *findBtn = new QPushButton("Search");

  QHBoxLayout *l = new QHBoxLayout();
  l->addWidget( searchField );
  l->addWidget( findBtn );

  setLayout(l);

  connect( findBtn, SIGNAL(clicked()), this, SLOT(search()) );
  connect( searchField, SIGNAL(returnPressed()), this, SLOT(search()) );
}

void ScateHelpSearchBar::search()
{
  QString text = searchField->text();
  if( !text.isEmpty() ) emit searchedFor( text );
}

ScateFindBar::ScateFindBar()
{
  searchField = new QLineEdit();
  QPushButton *nextBtn = new QPushButton("Next");
  QPushButton *prevBtn = new QPushButton("Previous");
  QAction *nextAction = KStandardAction::findNext( this, SLOT(next()), this );
  QAction *prevAction = KStandardAction::findPrev( this, SLOT(previous()), this );

  addWidget( new QLabel("Find:") );
  addWidget( searchField );
  addAction( nextAction );
  addAction( prevAction );

  setIconSize( QSize(16,16) );
  setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Maximum);

  connect( searchField, SIGNAL(textChanged(const QString&)),
           this, SLOT(next()) );
  connect( searchField, SIGNAL(returnPressed()), this, SLOT(next()) );
  connect( nextBtn, SIGNAL(clicked()), this, SLOT(next()) );
  connect( prevBtn, SIGNAL(clicked()), this, SLOT(previous()) );
}

void ScateFindBar::setText( const QString &text )
{
  blockSignals(true);
  searchField->setText( text );
  searchField->selectAll();
  searchField->setFocus();
  blockSignals(false);
}

void ScateFindBar::activate( bool backward )
{
  QWebPage::FindFlags flags = QWebPage::FindWrapsAroundDocument;
  if( backward ) flags |= QWebPage::FindBackward;
  emit activated( searchField->text(), flags );
}

void ScateFindBar::next() {
  activate( false );
}

void ScateFindBar::previous() {
  activate( true );
}
