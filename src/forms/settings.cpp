#include <obs-frontend-api.h>
#include <util/dstr.h>
#include <util/config-file.h>
#include <obs-module.h>
#include <obs.hpp>

#include <QPushButton>

#include <string>

#include "settings.hpp"
#include "ui_settings.h"

#include "../receiver.hpp"

extern Receiver *receiver;

Settings::Settings(QWidget *parent) : QDialog(parent), ui(new Ui::Settings)
{
	ui->setupUi(this);

	// Remove the ? button on dialogs on Windows
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	// Set up slots
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
		&Settings::okClicked);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
		&Settings::resetValues);
	connect(ui->udsAddr, &QLineEdit::textChanged, this,
		&Settings::validate);
	connect(ui->localAddr, &QLineEdit::textChanged, this,
		&Settings::validate);
	connect(ui->connectToUDS, &QCheckBox::stateChanged, this,
		&Settings::connectToUDSChanged);
}

Settings::~Settings()
{
	delete ui;
}

void Settings::toggleVisible()
{
	resetValues();

	setVisible(!isVisible());
}

void Settings::connectToUDSChanged()
{
	bool enabled = ui->connectToUDS->isChecked();
	ui->udsAddr->setEnabled(enabled);
	ui->udsPort->setEnabled(enabled);
}

void Settings::validate()
{
	bool ok = true;

	if (ui->connectToUDS->isChecked()) {
		if (!QHostAddress().setAddress(ui->udsAddr->text()))
			ok = false;
	}

	if (!QHostAddress().setAddress(ui->localAddr->text()))
		ok = false;

	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ok);
}

void Settings::okClicked()
{
	bool enableReceiver = ui->enableReceiver->isChecked();

	if (ui->connectToUDS->isChecked())
		receiver->udsAddr.setAddress(ui->localAddr->text());
	else
		receiver->udsAddr.clear();
	receiver->udsPort = ui->udsPort->value();
	receiver->listenAddr = QHostAddress(ui->localAddr->text());
	receiver->listenPort = ui->localPort->value();
	receiver->validateChecksums = ui->validateChecksums->isChecked();

	receiver->updateReceiver(enableReceiver);
}

void Settings::resetValues()
{
	// put everything back
	ui->enableReceiver->setChecked(receiver->isEnabled());
	ui->udsAddr->setText(receiver->udsAddr.toString());
	ui->udsPort->setValue(receiver->udsPort);
	ui->localAddr->setText(receiver->listenAddr.toString());
	ui->localPort->setValue(receiver->listenPort);
	ui->validateChecksums->setChecked(receiver->validateChecksums);
}
