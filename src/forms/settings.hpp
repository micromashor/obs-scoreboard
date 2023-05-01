#ifndef Settings_H
#define Settings_H

#include <obs.hpp>

#include <QDialog>
#include <QString>

class Binding {
public:
	Binding();
	Binding(obs_data_t *json);
	obs_data_t *toJSON() const;

	bool enabled;
	QString name;
	uint32_t item_number;
	uint32_t field_length;
	QString source_id;
	QString parent_prop;
	int use_as;
	bool trim_str;
	bool invert_bool;
	QString value_if_true;
	QString value_if_false;
};

namespace Ui {
class Settings;
}

class Settings : public QDialog {
	Q_OBJECT

public:
	explicit Settings(QWidget *parent);
	~Settings();

	void toggleVisible(bool checked);

	bool enableReceiver;

	bool connectToUDS;
	QString udsAddr;
	uint16_t udsPort;
	QString listenAddr;
	uint16_t listenPort;
	bool validateChecksums;

	std::vector<Binding> bindings;

private slots:

	void connectToUDSChanged(int arg1);

	void okClicked();

	void cancelClicked();

private:
	void save();

	Ui::Settings *ui;
};

#endif // Settings_H
