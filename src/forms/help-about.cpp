#include <obs.hpp>

#include "help-about.hpp"

#include "../receiver.hpp"

#include "ui_help-about.h"

#include "../plugin-macros.generated.h"

extern Receiver *receiver;

HelpAbout::HelpAbout(QWidget *parent) : QDialog(parent), ui(new Ui::HelpAbout)
{
	ui->setupUi(this);

	// Remove the ? button on dialogs on Windows
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	connect(receiver, &Receiver::counterChanged, this,
		&HelpAbout::counterChanged);
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

void HelpAbout::counterChanged(int which, unsigned long long newval)
{
	QLabel *counter = nullptr;

	switch (which) {
	case COUNTER_PACKETS:
		counter = ui->packetsReceived;
		break;
	case COUNTER_FRAMES:
		counter = ui->framesReceived;
		break;
	case COUNTER_ERRORS:
		counter = ui->framesDropped;
		break;
	}

	if (!counter)
		return;

	counter->setText(QString::number(newval));
}
