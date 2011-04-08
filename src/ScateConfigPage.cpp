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
#include "ScatePlugin.hpp"

#include <klocale.h>
#include <kconfiggroup.h>
#include <kglobal.h>
#include <kicon.h>

#include <QTabWidget>
#include <QFormLayout>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QToolBar>
#include <QAction>

/*********************** CONFIG ******************************/

ScateConfigPage::ScateConfigPage( ScatePlugin *plugin_, QWidget *parent )
  : Kate::PluginConfigPage( parent ), plugin(plugin_)
{
  QTabWidget *tabs = new QTabWidget;

  // Programs tab

    // Sclang group

    sclangExeEdit = new QLineEdit();
    dataDirEdit = new QLineEdit();
    startLangCheck = new QCheckBox( "Start With Scate" );

    QFormLayout *sclangForm = new QFormLayout();
    sclangForm->setContentsMargins(0,0,0,0);
    sclangForm->addRow( new QLabel( i18n( "Command:" ) ),
                        sclangExeEdit );
    sclangForm->addRow( new QLabel( i18n( "Runtime Directory:" ) ),
                        dataDirEdit );

    QVBoxLayout *sclangVBox = new QVBoxLayout();
    sclangVBox->addLayout( sclangForm );
    sclangVBox->addWidget( startLangCheck );

    QGroupBox *sclangGrp = new QGroupBox( "Sclang" );
    sclangGrp->setLayout( sclangVBox );

    // GUI group

    swingOscDirEdit = new QLineEdit();

    QFormLayout *guiForm = new QFormLayout();
    guiForm->addRow( new QLabel( i18n( "SwingOSC Program:" ) ),
                      swingOscDirEdit );

    QGroupBox *guiGrp = new QGroupBox( "SuperCollider GUI" );
    guiGrp->setLayout( guiForm );


  QVBoxLayout * progVBox = new QVBoxLayout();
  progVBox->addWidget( sclangGrp );
  progVBox->addWidget( guiGrp );
  progVBox->addStretch(1);

  QWidget *progTab = new QWidget();
  progTab->setLayout( progVBox );

  // Terminal tab

  trmMaxRowSpin = new QSpinBox();
  trmMaxRowSpin->setRange( 10, 1000 );
  trmFontCombo = new QFontComboBox();
  trmFontSizeSpin = new QSpinBox();
  trmFontSizeSpin->setRange( 1, 30 );

  QFormLayout *trmForm = new QFormLayout();
  trmForm->addRow( new QLabel("Maximum Rows:"), trmMaxRowSpin );

  QHBoxLayout *trmFontHBox = new QHBoxLayout();
  trmFontHBox->setContentsMargins(0,0,0,0);
  trmFontHBox->addWidget( trmFontCombo );
  trmFontHBox->addWidget( new QLabel("Size:") );
  trmFontHBox->addWidget( trmFontSizeSpin );

  trmForm->addRow( new QLabel("Font:"), trmFontHBox );

  QWidget *trmTab = new QWidget();
  trmTab->setLayout( trmForm );

  // Help tab

  helpDirList = new ScateDirListWidget();
  helpFontScaleSpin = new QDoubleSpinBox();
  helpFontScaleSpin->setRange( 0.1, 10.0 );
  helpFontScaleSpin->setDecimals(1);
  helpFontScaleSpin->setSingleStep(0.1);

  QFormLayout *helpForm = new QFormLayout();
  helpForm->addRow( new QLabel("Locations:"),
                    helpDirList );
  helpForm->addRow( new QLabel("Text Scaling:"),
                    helpFontScaleSpin );

  QWidget *helpTab = new QWidget();
  helpTab->setLayout( helpForm );

  // Top layout

  tabs->addTab( progTab, "Programs" );
  tabs->addTab( trmTab, "Terminal" );
  tabs->addTab( helpTab, "Help && Documentation" );

  QVBoxLayout *layout = new QVBoxLayout( this );
  layout->addWidget( tabs );

  reset();

  connect( sclangExeEdit, SIGNAL(textChanged(QString)), this, SIGNAL(changed()) );
  connect( dataDirEdit, SIGNAL(textChanged(QString)), this, SIGNAL(changed()) );
  connect( startLangCheck, SIGNAL(stateChanged(int)), this, SIGNAL(changed()) );
  connect( swingOscDirEdit, SIGNAL(textChanged(QString)), this, SIGNAL(changed()) );
  connect( trmMaxRowSpin, SIGNAL(valueChanged(int)), this, SIGNAL(changed()) );
  connect( trmFontCombo, SIGNAL(currentFontChanged(QFont)), this, SIGNAL(changed()) );
  connect( trmFontSizeSpin, SIGNAL(valueChanged(int)), this, SIGNAL(changed()) );
  connect( helpDirList, SIGNAL(changed()), this, SIGNAL(changed()) );
  connect( helpFontScaleSpin, SIGNAL(valueChanged(double)), this, SIGNAL(changed()) );
}

void ScateConfigPage::apply()
{
  KConfigGroup config(KGlobal::config(), "Scate");
  config.writeEntry( "ScLangExecutable", sclangExeEdit->text() );
  config.writeEntry( "RuntimeDataDir", dataDirEdit->text() );
  config.writeEntry( "StartLang", startLangCheck->isChecked() );
  config.writeEntry( "SwingOscProgram", swingOscDirEdit->text() );

  config.writeEntry( "TerminalMaxRows", trmMaxRowSpin->value() );
  QFont trmFont = trmFontCombo->currentFont();
  trmFont.setPointSize( trmFontSizeSpin->value() );
  config.writeEntry( "TerminalFont", trmFont );

  config.writePathEntry( "HelpDirs", helpDirList->dirs() );
  config.writeEntry( "HelpFontScale", helpFontScaleSpin->value() );

  config.sync();

  plugin->applyConfig();
}

void ScateConfigPage::reset()
{
  KConfigGroup config(KGlobal::config(), "Scate");
  sclangExeEdit->setText( config.readEntry( "ScLangExecutable", QString() ) );
  dataDirEdit->setText( config.readEntry( "RuntimeDataDir", QString() ) );
  startLangCheck->setChecked( config.readEntry( "StartLang", false ) );
  swingOscDirEdit->setText( config.readEntry( "SwingOscProgram", QString() ) );

  trmMaxRowSpin->setValue( config.readEntry( "TerminalMaxRows", 500 ) );
  QFont trmFont = config.readEntry( "TerminalFont", QFont() );
  trmFontCombo->setCurrentFont( trmFont );
  trmFontSizeSpin->setValue( trmFont.pointSize() );

  helpDirList->setDirs( config.readEntry( "HelpDirs", QStringList() ) );
  helpFontScaleSpin->setValue( config.readEntry( "HelpFontScale", 1.0 ) );
}

void ScateConfigPage::defaults()
{
  sclangExeEdit->clear();
  dataDirEdit->setText( QString() );
  startLangCheck->setChecked( false );
  swingOscDirEdit->clear();

  trmMaxRowSpin->setValue(500);
  QFont defFont;
  trmFontCombo->setCurrentFont( defFont );
  trmFontSizeSpin->setValue( defFont.pointSize() );

  helpDirList->setDirs( QStringList() );
  helpFontScaleSpin->setValue(1.0);

  KConfigGroup config(KGlobal::config(), "scate");
  config.writeEntry( "ScLangExecutable", QString() );
  config.writeEntry( "RuntimeDataDir", QString() );
  config.writeEntry( "SwingOscProgram", QString() );
  config.writeEntry( "StartLang", false );

  config.writeEntry( "TerminalMaxRows", 500 );
  config.writeEntry( "TerminalFont", defFont );

  config.writePathEntry( "HelpDirs", QStringList() );
  config.writeEntry( "HelpFontScale", 1.0 );
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

  connect( list, SIGNAL(itemChanged(QListWidgetItem*)), this, SIGNAL(changed()) );
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
  emit changed();
}

void ScateDirListWidget::removeDir()
{
  QListWidgetItem *item = list->currentItem();
  if( item ) {
    delete item;
    emit changed();
  }
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

