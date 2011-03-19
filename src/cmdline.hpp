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

#ifndef SCATE_CMDLINE_H
#define SCATE_CMDLINE_H

#include <QString>
#include <QWidget>
#include <QLineEdit>

namespace Scate {

class CmdLine : public QWidget
{
  Q_OBJECT

  public:
    CmdLine( const QString &text, int maxHistory = 30 );
  signals:
    void invoked( const QString &, bool silent );
  private:
    bool eventFilter( QObject *, QEvent * );

    QLineEdit *expr;
    QStringList history;
    int curHistory;
    int maxHistory;
};

} // namespace Scate

#endif // SCATE_CMDLINE_H
