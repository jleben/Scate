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

#include <QFormLayout>
#include <QLabel>

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
  dataDirEdit->setText( config.readEntry( "RuntimeDataDir", QString() ) );
  helpDirEdit->setText( config.readEntry( "HelpDir", QString() ) );
  swingOscDirEdit->setText( config.readEntry( "SwingOscProgram", QString() ) );
  startLangCheck->setChecked( config.readEntry( "StartLang", false ) );
}

void ScateConfigPage::defaults()
{
  sclangExeEdit->clear();
  dataDirEdit->setText( QString() );
  helpDirEdit->clear();
  swingOscDirEdit->clear();
  startLangCheck->setChecked( false );

  KConfigGroup config(KGlobal::config(), "scate");
  config.writeEntry( "ScLangExecutable", QString() );
  config.writeEntry( "RuntimeDataDir", QString() );
  config.writeEntry( "HelpDir", QString() );
  config.writeEntry( "SwingOscProgram", QString() );
  config.writeEntry( "StartLang", false );
}
