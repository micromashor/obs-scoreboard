#ifndef Help_H
#define Help_H

#include <obs.hpp>

#include "help-about.hpp"

#include "ui_help-about.h"

#include "../plugin-macros.generated.h"

HelpAbout::HelpAbout(QWidget *parent) : QDialog(parent), ui(new Ui::HelpAbout)
{
	ui->setupUi(this);

	// Remove the ? button on dialogs on Windows
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

HelpAbout::~HelpAbout()
{
	delete ui;
}

void HelpAbout::toggleVisible(bool checked)
{
	UNUSED_PARAMETER(checked);
	setVisible(!isVisible());
}

void HelpAbout::incrementCounter(int which)
{
	QLabel *label = nullptr;

	switch (which) {
	case HelpAbout::counters::packetsReceived:
		label = ui->packetsReceived;
		break;
	case HelpAbout::counters::framesReceived:
		label = ui->framesReceived;
		break;
	case HelpAbout::counters::framesDropped:
		label = ui->framesDropped;
		break;
	}

	if (!label) {
		blog(LOG_ERROR, "unknown counter (id = %d)", which);
		return;
	}

	auto currentVal = label->text().toULongLong();
	currentVal++;
	label->setText(QString::number(currentVal));
}

#endif // Help_H
