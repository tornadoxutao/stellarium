/*
 * Stellarium TelescopeControl Plug-in
 * 
 * Copyright (C) 2009-2010 Bogdan Marinov (this file)
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "Dialog.hpp"
#include "ui_telescopeConfigurationDialog.h"
#include "TelescopeConfigurationDialog.hpp"
#include "TelescopeControl.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelStyle.hpp"

#include <QDebug>
#include <QCompleter>
#include <QFrame>
#include <QTimer>

TelescopeConfigurationDialog::TelescopeConfigurationDialog()
{
	ui = new Ui_telescopeConfigurationDialog;
	
	telescopeManager = GETSTELMODULE(TelescopeControl);

	telescopeNameValidator = new QRegExpValidator (QRegExp("[^:\"]+"), this);//Test the update for JSON
	hostNameValidator = new QRegExpValidator (QRegExp("[a-zA-Z0-9\\-\\.]+"), this);//TODO: Write a proper host/IP regexp?
	circleListValidator = new QRegExpValidator (QRegExp("[0-9,\\.\\s]+"), this);
	#ifdef Q_OS_WIN32
	serialPortValidator = new QRegExpValidator (QRegExp("COM[0-9]+"), this);
	#else
	serialPortValidator = new QRegExpValidator (QRegExp("/dev/*"), this);
	#endif
}

TelescopeConfigurationDialog::~TelescopeConfigurationDialog()
{	
	delete ui;
	
	delete telescopeNameValidator;
	delete hostNameValidator;
	delete circleListValidator;
	delete serialPortValidator;
}

void TelescopeConfigurationDialog::languageChanged()
{
	if (dialog)
		ui->retranslateUi(dialog);
}

// Initialize the dialog widgets and connect the signals/slots
void TelescopeConfigurationDialog::createDialogContent()
{
	ui->setupUi(dialog);
	
	//Inherited connect
	connect(ui->closeStelWindow, SIGNAL(clicked()), this, SLOT(buttonDiscardPressed()));
	connect(dialog, SIGNAL(rejected()), this, SLOT(buttonDiscardPressed()));
	
	//Connect: sender, signal, receiver, member
	connect(ui->radioButtonTelescopeLocal, SIGNAL(toggled(bool)), this, SLOT(toggleTypeLocal(bool)));
	connect(ui->radioButtonTelescopeConnection, SIGNAL(toggled(bool)), this, SLOT(toggleTypeConnection(bool)));
	connect(ui->radioButtonTelescopeVirtual, SIGNAL(toggled(bool)), this, SLOT(toggleTypeVirtual(bool)));
	
	connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(buttonSavePressed()));
	connect(ui->pushButtonDiscard, SIGNAL(clicked()), this, SLOT(buttonDiscardPressed()));
	
	connect(ui->comboBoxDeviceModel, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(deviceModelSelected(const QString&)));
	
	//Setting validators
	ui->lineEditTelescopeName->setValidator(telescopeNameValidator);
	ui->lineEditHostName->setValidator(hostNameValidator);
	ui->lineEditCircleList->setValidator(circleListValidator);
	ui->lineEditSerialPort->setValidator(serialPortValidator);
	
	//Initialize the style
	updateStyle();
}

//Set the configuration panel in a predictable state
void TelescopeConfigurationDialog::initConfigurationDialog()
{
	//Reusing code used in both methods that call this one
	deviceModelNames = telescopeManager->getDeviceModels().keys();
	
	//Type
	ui->radioButtonTelescopeLocal->setEnabled(true);
	
	//Name
	ui->lineEditTelescopeName->clear();

	//Connect at startup
	ui->checkBoxConnectAtStartup->setChecked(false);
	
	//Serial port
	ui->lineEditSerialPort->clear();
	ui->lineEditSerialPort->setCompleter(new QCompleter(SERIAL_PORT_NAMES, this));
	ui->lineEditSerialPort->setText(SERIAL_PORT_NAMES.value(0));
	
	//Populating the list of available devices
	ui->comboBoxDeviceModel->clear();
	if (!deviceModelNames.isEmpty())
	{
		deviceModelNames.sort();
		ui->comboBoxDeviceModel->addItems(deviceModelNames);
	}
	ui->comboBoxDeviceModel->setCurrentIndex(0);
	
	//FOV circles
	ui->checkBoxCircles->setChecked(false);
	ui->lineEditCircleList->clear();
}

void TelescopeConfigurationDialog::initNewTelescopeConfiguration(int slot)
{
	configuredSlot = slot;
	initConfigurationDialog();
	ui->stelWindowTitle->setText("Add New Telescope");
	ui->lineEditTelescopeName->setText(QString("New Telescope %1").arg(QString::number(configuredSlot)));
	
	if(deviceModelNames.isEmpty())
	{
		ui->radioButtonTelescopeLocal->setEnabled(false);
		ui->radioButtonTelescopeConnection->setChecked(true);
		toggleTypeConnection(true);//Not called if the button is already checked
	}
	else
	{
		ui->radioButtonTelescopeLocal->setEnabled(true);
		ui->radioButtonTelescopeLocal->setChecked(true);
		toggleTypeLocal(true);//Not called if the button is already checked
	}
	
	ui->doubleSpinBoxTelescopeDelay->setValue(SECONDS_FROM_MICROSECONDS(DEFAULT_DELAY));
}

void TelescopeConfigurationDialog::initExistingTelescopeConfiguration(int slot)
{
	configuredSlot = slot;
	initConfigurationDialog();
	ui->stelWindowTitle->setText("Configure Telescope");
	
	//Read the telescope properties
	QString name;
	ConnectionType connectionType;
	QString host;
	int portTCP;
	int delay;
	bool connectAtStartup;
	QList<double> circles;
	QString deviceModelName;
	QString serialPortName;
	if(!telescopeManager->getTelescopeAtSlot(slot, connectionType, name, host, portTCP, delay, connectAtStartup, circles, deviceModelName, serialPortName))
	{
		//TODO: Add debug
		return;
	}
	
	ui->lineEditTelescopeName->setText(name);
	
	if(!deviceModelName.isEmpty())
	{
		ui->radioButtonTelescopeLocal->setChecked(true);
		
		ui->lineEditHostName->setText("localhost");//TODO: Remove magic word!
		
		//Make the current device model selected in the list
		int index = ui->comboBoxDeviceModel->findText(deviceModelName);
		if(index < 0)
		{
			qDebug() << "TelescopeConfigurationDialog: Current device model is not in the list?";
			emit changesDiscarded();
			return;
		}
		else
		{
			ui->comboBoxDeviceModel->setCurrentIndex(index);
		}
		//Initialize the serial port value
		ui->lineEditSerialPort->setText(serialPortName);
	}
	else //Local or remote connection
	{
		ui->radioButtonTelescopeConnection->setChecked(true);//Calls toggleTypeConnection(true)
		if (connectionType == ConnectionRemote)
			ui->lineEditHostName->setText(host);
	}
	
	//Circles
	if(!circles.isEmpty())
	{
		ui->checkBoxCircles->setChecked(true);
		
		QStringList circleList;
		for(int i = 0; i < circles.size(); i++)
			circleList.append(QString::number(circles[i]));
		ui->lineEditCircleList->setText(circleList.join(", "));
	}
	
	//TCP port
	ui->spinBoxTCPPort->setValue(portTCP);
	
	//Delay
	ui->doubleSpinBoxTelescopeDelay->setValue(SECONDS_FROM_MICROSECONDS(delay));//Microseconds to seconds
	
	//Connect at startup
	ui->checkBoxConnectAtStartup->setChecked(connectAtStartup);
}

void TelescopeConfigurationDialog::toggleTypeLocal(bool isChecked)
{
	if(isChecked)
	{
		//Re-initialize values that may have been changed
		ui->comboBoxDeviceModel->setCurrentIndex(0);
		ui->lineEditSerialPort->setText(SERIAL_PORT_NAMES.value(0));
		ui->lineEditHostName->setText("localhost");
		ui->spinBoxTCPPort->setValue(DEFAULT_TCP_PORT_FOR_SLOT(configuredSlot));

		//Enable/disable controls
		ui->labelHost->setEnabled(false);
		ui->lineEditHostName->setEnabled(false);

		ui->toolBoxSettings->setCurrentIndex(ui->toolBoxSettings->indexOf(ui->pageTelescopeProperties));
	}
	else
	{
		ui->labelHost->setEnabled(true);
		ui->lineEditHostName->setEnabled(true);
	}
}

void TelescopeConfigurationDialog::toggleTypeConnection(bool isChecked)
{
	if(isChecked)
	{
		//Re-initialize values that may have been changed
		ui->lineEditHostName->setText("localhost");
		ui->spinBoxTCPPort->setValue(DEFAULT_TCP_PORT_FOR_SLOT(configuredSlot));

		ui->toolBoxSettings->setItemEnabled(ui->toolBoxSettings->indexOf(ui->pageDeviceSettings), false);

		ui->toolBoxSettings->setCurrentIndex(ui->toolBoxSettings->indexOf(ui->pageTelescopeProperties));
	}
	else
	{
		ui->toolBoxSettings->setItemEnabled(ui->toolBoxSettings->indexOf(ui->pageDeviceSettings), true);
	}
}

void TelescopeConfigurationDialog::toggleTypeVirtual(bool isChecked)
{
	if(isChecked)
	{
		ui->toolBoxSettings->setItemEnabled(ui->toolBoxSettings->indexOf(ui->pageDeviceSettings), false);
		ui->toolBoxSettings->setItemEnabled(ui->toolBoxSettings->indexOf(ui->pageConnectionSettings), false);

		ui->toolBoxSettings->setCurrentIndex(ui->toolBoxSettings->indexOf(ui->pageTelescopeProperties));
	}
	else
	{
		ui->toolBoxSettings->setItemEnabled(ui->toolBoxSettings->indexOf(ui->pageDeviceSettings), true);
		ui->toolBoxSettings->setItemEnabled(ui->toolBoxSettings->indexOf(ui->pageConnectionSettings), true);
	}
}

void TelescopeConfigurationDialog::buttonSavePressed()
{
	//Main telescope properties
	QString name = ui->lineEditTelescopeName->text().trimmed();
	if(name.isEmpty())
		return;
	QString host = ui->lineEditHostName->text();
	if(host.isEmpty())//TODO: Validate host
		return;
	
	int delay = MICROSECONDS_FROM_SECONDS(ui->doubleSpinBoxTelescopeDelay->value());
	int portTCP = ui->spinBoxTCPPort->value();
	bool connectAtStartup = ui->checkBoxConnectAtStartup->isChecked();
	
	//Circles
	//TODO: This will change if there is a validator for that field
	QList<double> circles;
	QString rawCircles = ui->lineEditCircleList->text().trimmed();
	QStringList circleStrings;
	if(ui->checkBoxCircles->isChecked() && !(rawCircles.isEmpty()))
	{
		circleStrings = rawCircles.simplified().remove(' ').split(',', QString::SkipEmptyParts);
		circleStrings.removeDuplicates();
		circleStrings.sort();
		
		for(int i = 0; i < circleStrings.size(); i++)
		{
			if(i >= MAX_CIRCLE_COUNT)
				break;
			double circle = circleStrings.at(i).toDouble();
			if(circle > 0.0)
				circles.append(circle);
		}
	}
	
	//Type and server properties
	//TODO: When adding, check for success!
	ConnectionType type = ConnectionNA;
	if(ui->radioButtonTelescopeLocal->isChecked())
	{
		//Read the serial port
		QString serialPortName = ui->lineEditSerialPort->text();
		if(!serialPortName.startsWith(SERIAL_PORT_PREFIX))
			return;//TODO: Add more validation!
		
		type = ConnectionInternal;
		telescopeManager->addTelescopeAtSlot(configuredSlot, type, name, host, portTCP, delay, connectAtStartup, circles, ui->comboBoxDeviceModel->currentText(), serialPortName);
	}
	else if (ui->radioButtonTelescopeConnection->isChecked())
	{
		if(host == "localhost")
			type = ConnectionLocal;
		else
			type = ConnectionRemote;
		telescopeManager->addTelescopeAtSlot(configuredSlot, type, name, host, portTCP, delay, connectAtStartup, circles);
	}
	else if (ui->radioButtonTelescopeVirtual->isChecked())
	{
		type = ConnectionVirtual;
		telescopeManager->addTelescopeAtSlot(configuredSlot, type, name, QString(), portTCP, delay, connectAtStartup, circles);
	}
	
	emit changesSaved(name, type);
}

void TelescopeConfigurationDialog::buttonDiscardPressed()
{
	emit changesDiscarded();
}

void TelescopeConfigurationDialog::deviceModelSelected(const QString& deviceModelName)
{
	ui->labelDeviceModelDescription->setText(telescopeManager->getDeviceModels().value(deviceModelName).description);
	ui->doubleSpinBoxTelescopeDelay->setValue(SECONDS_FROM_MICROSECONDS(telescopeManager->getDeviceModels().value(deviceModelName).defaultDelay));
}

void TelescopeConfigurationDialog::updateStyle()
{
	if (dialog)
	{
		StelGui* gui = dynamic_cast<StelGui*>(StelApp::getInstance().getGui());
		Q_ASSERT(gui);
		dialog->setStyleSheet(telescopeManager->getModuleStyleSheet(gui->getStelStyle()).qtStyleSheet);
	}
}
