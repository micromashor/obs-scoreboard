#include <stack>

#include <obs.hpp>

#include "configure-binding.hpp"

#include "ui_configure-binding.h"

#include "../plugin-macros.generated.h"

#include "../receiver.hpp"

extern Receiver *receiver;

ConfigureBinding::ConfigureBinding(QWidget *parent)
	: QDialog(parent), ui(new Ui::ConfigureBinding)
{
	ui->setupUi(this);

	// Remove the ? button on dialogs on Windows
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	connect(ui->refreshSourceListButton, &QPushButton::clicked, this,
		&ConfigureBinding::refreshSourceList);
	connect(ui->sourceComboBox, &QComboBox::currentIndexChanged, this,
		&ConfigureBinding::sourceChanged);
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
		&ConfigureBinding::saved);
}

ConfigureBinding::~ConfigureBinding()
{
	delete ui;
}

typedef std::tuple<QComboBox *, ConfigureBinding *> add_to_source_combobox_arg;

extern "C" bool add_source_to_combobox(void *arg, obs_source_t *source)
{
	auto data = (add_to_source_combobox_arg *)arg;

	auto box = std::get<QComboBox *>(*data);
	auto config = std::get<ConfigureBinding *>(*data);

	const char *name = obs_source_get_name(source);
	const char *uuid = obs_source_get_uuid(source);

	box->addItem(name, uuid);

	if (config->active->source_id == uuid) {
		box->setCurrentIndex(box->count() - 1);
		config->sourceChanged();
	}

	return true;
}

void ConfigureBinding::refreshSourceList()
{
	ui->sourceComboBox->clear();

	add_to_source_combobox_arg arg{ui->sourceComboBox, this};

	obs_enum_sources(&add_source_to_combobox, &arg);
}

void ConfigureBinding::addPropertiesObjectRecursive(
	std::map<std::string, std::string> &map, obs_properties_t *props,
	const std::string &setting_prefix,
	const std::string &description_prefix)
{
	obs_property_t *prop = obs_properties_first(props);
	do {
		std::string setting = setting_prefix + obs_property_name(prop);
		std::string description =
			description_prefix + obs_property_description(prop);

		obs_property_type type = obs_property_get_type(prop);

		if (type == OBS_PROPERTY_GROUP) {
			obs_properties_t *c_props =
				obs_property_group_content(prop);
			addPropertiesObjectRecursive(map, c_props,
						     setting + " > ",
						     description + " > ");
		} else if (type == OBS_PROPERTY_BOOL ||
			   type == OBS_PROPERTY_TEXT) {
			map[setting] = description;
		} else if (type == OBS_PROPERTY_FONT) {
			map[setting + "#" +
			    QString::number(OBS_FONT_BOLD).toStdString()] =
				description + " > Bold";
			map[setting + "#" +
			    QString::number(OBS_FONT_ITALIC).toStdString()] =
				description + " > Italic";
			map[setting + "#" +
			    QString::number(OBS_FONT_UNDERLINE).toStdString()] =
				description + " > Underline";
			map[setting + "#" +
			    QString::number(OBS_FONT_STRIKEOUT).toStdString()] =
				description + " > Strikeout";
		}
	} while (obs_property_next(&prop));
}

void ConfigureBinding::sourceChanged()
{
	auto uuid = ui->sourceComboBox->currentData().toString().toUtf8();

	ui->propComboBox->clear();
	ui->propComboBox->setEnabled(false);

	OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.constData());

	if (!source)
		return;

	obs_properties_t *props = obs_source_properties(source);

	std::map<std::string, std::string> map;

	addPropertiesObjectRecursive(map, props, "", "");

	for (auto &pair : map) {
		const char *dataPath = pair.first.c_str();
		const char *name = pair.second.c_str();

		ui->propComboBox->addItem(name, dataPath);
	}

	QStringList keylist;
	for (auto &elem : active->parent_prop)
		keylist.append(elem.c_str());

	QString activeKey = keylist.join('.');

	if (active->flag_value != 0) {
		activeKey += '#';
		activeKey += QString::number(active->flag_value);
	}

	int itemCount = ui->propComboBox->count();
	for (int i = 0; i < itemCount; i++) {
		QString str = ui->propComboBox->itemData(i).toString();

		if (str == activeKey) {
			ui->propComboBox->setCurrentIndex(i);

			break;
		}
	}

	ui->propComboBox->setEnabled(true);
}

void ConfigureBinding::openForBinding(Binding *binding)
{
	active = binding;

	ui->enableCheckbox->setChecked(active->enabled);
	ui->itemNoBox->setValue(active->item_number);
	ui->lengthBox->setValue(active->field_length);
	refreshSourceList();
	ui->trimStrCheckbox->setChecked(active->trim_str);
	ui->invertBoolCheckbox->setChecked(active->invert_bool);

	open();
}

void ConfigureBinding::saved()
{
	active->enabled = ui->enableCheckbox->isChecked();
	active->item_number = ui->itemNoBox->value();
	active->field_length = ui->lengthBox->value();
	active->source_id =
		ui->sourceComboBox->currentData().toString().toStdString();

	active->parent_prop.clear();
	auto flagsplit_arr =
		ui->propComboBox->currentData().toString().split('#');
	if (flagsplit_arr.length() == 2)
		active->flag_value = flagsplit_arr[1].toInt();
	else
		active->flag_value = 0;

	auto proplist = flagsplit_arr[0].split('.');
	for (auto &str : proplist)
		active->parent_prop.emplace_back(str.toUtf8().toStdString());

	active->trim_str = ui->trimStrCheckbox->isChecked();
	active->invert_bool = ui->invertBoolCheckbox->isChecked();

	receiver->saveConfig();
}
