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


#include "ScateConfigPage.hpp"

#include <klocale.h>
#include <kconfiggroup.h>
#include <kglobal.h>
#include <kicon.h>

#include <QFormLayout>
#include <QLabel>
#include <QToolBar>
#include <QAction>

/*********************** CONFIG ******************************/

ScateConfigPage::ScateConfigPage( ScatePlugin *, QWidget *parent )
  : Kate::PluginConfigPage( parent )
{
  QFormLayout *layout = new QFormLayout( this );

  sclangExeEdit = new QLineEdit();
  layout->addRow( new QLabel( i18n( "Sclang Command" ) ),
                  sclangExeEdit );

  dataDirEdit = new QLineEdit();
  layout->addRow( new QLabel( i18n( "Runtime Data Directory" ) ),
                  dataDirEdit );

  helpDirList = new ScateDirListWidget();
  layout->addRow( new QLabel( i18n( "Help File Directories" ) ),
                  helpDirList );

  swingOscDirEdit = new QLineEdit();
  layout->addRow( new QLabel( i18n( "SwingOSC Java Program" ) ),
                  swingOscDirEdit );

  startLangCheck = new QCheckBox( );
  layout->addRow( new QLabel( i18n( "Start Interpreter With Plugin" ) ),
                  startLangCheck );
  reset();
}

void ScateConfigPage::apply()
{
  KConfigGroup config(KGlobal::config(), "Scate");
  config.writeEntry( "ScLangExecutable", sclangExeEdit->text() );
  config.writeEntry( "RuntimeDataDir", dataDirEdit->text() );
  config.writePathEntry( "HelpDirs", helpDirList->dirs() );
  config.writeEntry( "SwingOscProgram", swingOscDirEdit->text() );
  config.writeEntry( "StartLang", startLangCheck->isChecked() );
  config.sync();
}

void ScateConfigPage::reset()
{
  KConfigGroup config(KGlobal::config(), "Scate");
  sclangExeEdit->setText( config.readEntry( "ScLangExecutable", QString() ) );
  dataDirEdit->setText( config.readEntry( "RuntimeDataDir", QString() ) );
  helpDirList->setDirs( config.readEntry( "HelpDirs", QStringList() ) );
  swingOscDirEdit->setText( config.readEntry( "SwingOscProgram", QString() ) );
  startLangCheck->setChecked( config.readEntry( "StartLang", false ) );
}

void ScateConfigPage::defaults()
{
  sclangExeEdit->clear();
  dataDirEdit->setText( QString() );
  helpDirList->setDirs( QStringList() );
  swingOscDirEdit->clear();
  startLangCheck->setChecked( false );

  KConfigGroup config(KGlobal::config(), "scate");
  config.writeEntry( "ScLangExecutable", QString() );
  config.writeEntry( "RuntimeDataDir", QString() );
  config.writePathEntry( "HelpDirs", QStringList() );
  config.writeEntry( "SwingOscProgram", QString() );
  config.writeEntry( "StartLang", false );
}

ScateDirListWidget::ScateDirListWidget( QWidget *parent ) :
  QWidget( parent )
{
  QHBoxLayout *l = new QHBoxLayout;
  l->setContentsMargins(0,0,0,0);
  l->setSpacing(0);

  list = new QListWidget;
  list->setMaximumHeight( 70 );
  l->addWidget( list );

  QToolBar *tools = new QToolBar;
  tools->setOrientation( Qt::Vertical );
  tools->setIconSize( QSize(16,16) );
  QAction *a;
  a = tools->addAction( KIcon("list-add"), "Add" );
  QObject::connect( a, SIGNAL(triggered()), this, SLOT(addDir()) );
  a = tools->addAction( KIcon("list-remove"), "Remove" );
  QObject::connect( a, SIGNAL(triggered()), this, SLOT(removeDir()) );
  l->addWidget( tools );

  setLayout( l );
}

void ScateDirListWidget::setDirs( const QStringList &dirs )
{
  list->clear();
  foreach( QString dir, dirs ) {
    QListWidgetItem *item = new QListWidgetItem( dir );
    item->setFlags( item->flags() | Qt::ItemIsEditable );
    list->addItem( item );
  }
}

void ScateDirListWidget::addDir()
{
  QListWidgetItem *item = new QListWidgetItem( "A Help Directory" );
  item->setFlags( item->flags() | Qt::ItemIsEditable );
  list->addItem( item );
  list->setCurrentItem( item );
  list->editItem( item );
}

void ScateDirListWidget::removeDir()
{
  delete list->currentItem();
}

QStringList ScateDirListWidget::dirs()
{
  QStringList dirs;
  int c = list->count();
  int i;
  for( i = 0; i < c; ++i ) {
    dirs << list->item(i)->text();
  }
  return dirs;
}
