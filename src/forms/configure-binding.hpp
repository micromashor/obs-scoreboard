#ifndef ConfigureBinding_H
#define ConfigureBinding_H

#include <QDialog>
#include <QLabel>

#include "../receiver.hpp"

namespace Ui {
class ConfigureBinding;
}

class ConfigureBinding : public QDialog {
	Q_OBJECT

public:
	explicit ConfigureBinding(QWidget *parent);
	~ConfigureBinding();

	Binding *active;

public slots:

	void sourceChanged();

	void refreshSourceList();

	void openForBinding(Binding *binding);

	void saved();

private:
	void
	addPropertiesObjectRecursive(std::map<std::string, std::string> &map,
				     obs_properties_t *props,
				     const std::string &setting_prefix,
				     const std::string &description_prefixs);

	Ui::ConfigureBinding *ui;
};

#endif // ConfigureBinding_H
