#ifndef Settings_H
#define Settings_H

#include <obs.hpp>

#include <QDialog>
#include <QString>

#include "../receiver.hpp"

namespace Ui {
class Settings;
}

class Settings : public QDialog {
	Q_OBJECT

public:
	explicit Settings(QWidget *parent);
	~Settings();

	void toggleVisible();

private slots:

	void connectToUDSChanged();

	void okClicked();

	void resetValues();

	void validate();

private:
	Ui::Settings *ui;
};

#endif // Settings_H
