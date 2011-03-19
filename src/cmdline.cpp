/*
#
# Copyright 2010-2011 Jakob Leben (jakob.leben@gmail.com)
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

#include "cmdline.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>

using namespace Scate;

CmdLine::CmdLine( const QString &text, int maxHist )
  : curHistory( -1 ),
    maxHistory( qMax(1,maxHist) )
{
  QHBoxLayout *l = new QHBoxLayout;
  l->setContentsMargins(0,0,0,0);
  setLayout( l );

  QLabel *lbl = new QLabel(text);
  expr = new QLineEdit;

  l->addWidget(lbl);
  l->addWidget(expr);

  expr->installEventFilter( this );
}

bool CmdLine::eventFilter( QObject *, QEvent *e )
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
