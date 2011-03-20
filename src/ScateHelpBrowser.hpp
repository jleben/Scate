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

#ifndef SCATE_HELP_BROWSER_H
#define SCATE_HELP_BROWSER_H

#include <QWidget>
#include <QLineEdit>
#include <QWebView>
#include <QToolBar>

class QWebView;
class ScateFindBar;

class ScateHelpBrowser : public QWidget
{
  Q_OBJECT
  public:
    ScateHelpBrowser( QWidget *parent = 0 );
    inline bool webViewFocused() { return webView->hasFocus(); }
    inline QString selectedText() { return webView->selectedText(); }
  public slots:
    void goHome();
    void newTextSearch();
    // NOTE: findHelpFor( class ) is used as a fallback in case SCDoc's
    // search page (Search.html) is not found in help dirs
    bool findHelpFor( const QString & className );
    void searchHelp( const QString & searchTerm );
    void findText( const QString&, QWebPage::FindFlags );
  private:
    void warnSetHelpDir();
    void showEvent( QShowEvent *e );
    bool event( QEvent *e );
    QWebView *webView;
    ScateFindBar *findBar;
    bool virgin;
};

class ScateHelpSearchBar : public QWidget
{
  Q_OBJECT
  public:
    ScateHelpSearchBar();
  signals:
    void searchedFor( const QString &text );
  private slots:
    void search();
  private:
    QLineEdit *searchField;
};

class ScateFindBar : public QToolBar
{
  Q_OBJECT
  public:
    ScateFindBar();
  signals:
    void activated( const QString &text, QWebPage::FindFlags );
  public slots:
    void setText( const QString &text );
    inline void clear() { searchField->clear(); }
  private slots:
    void activate( bool backward = false );
    void previous();
    void next();
  private:
    QLineEdit *searchField;
};

#endif
