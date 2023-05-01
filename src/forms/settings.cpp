#include <obs-frontend-api.h>
#include <util/dstr.h>
#include <util/config-file.h>
#include <obs-module.h>
#include <obs.hpp>

#include <string>

#include "settings.hpp"
#include "ui_settings.h"

#include "../plugin-macros.generated.h"

#define CFG_SECTION "OBSScoreboard"

#define CFG_RECEIVER_RUNNING "ReceiverRunning"
#define CFG_CONNECT_TO_UDS "ConnectToUDS"
#define CFG_UDS_ADDR "udsAddr"
#define CFG_UDS_PORT "udsPort"
#define CFG_LISTEN_ADDR "ListenAddr"
#define CFG_LISTEN_PORT "ListenPort"
#define CFG_VALIDATE_CHECKSUMS "ValidateChecksums"
#define CFG_BINDINGS_JSON "BindingsJSON"

#define BINDINGS_JSON_KEY "bindings"

#define BINDING_ENABLED "enabled"
#define BINDING_NAME "name"
#define BINDING_ITEMNO "item_number"
#define BINDING_LENGTH "field_length"
#define BINDING_SOURCE "source_id"
#define BINDING_PARENT_PROP "parent_prop"
#define BINDING_USE_AS "use_as"
#define BINDING_USE_AS_STR "STR"
#define BINDING_USE_AS_BOOL "BOOL"
#define BINDING_USE_AS_SWITCH "SWITCH"
#define BINDING_TRIM_STR "trim_str"
#define BINDING_INVERT_BOOL "invert_bool"
#define BINDING_VALUE_IF_TRUE "value_if_true"
#define BINDING_VALUE_IF_FALSE "value_if_false"

#define USE_AS_STR 0
#define USE_AS_BOOL 1
#define USE_AS_SWITCH 2

Binding::Binding()
{
	enabled = false;
	name = obs_module_text("OBSScoreboard.Bindings.NewName");
	item_number = 1;
	field_length = 1;
	source_id = "";
	parent_prop = "";
	use_as = USE_AS_STR;
	trim_str = false;
	invert_bool = false;
	value_if_true = "";
	value_if_false = "";
}

Binding::Binding(obs_data_t *json)
{
	enabled = obs_data_get_bool(json, BINDING_ENABLED);
	name = obs_data_get_string(json, BINDING_NAME);
	item_number = obs_data_get_int(json, BINDING_ITEMNO);
	field_length = obs_data_get_int(json, BINDING_LENGTH);
	source_id = obs_data_get_string(json, BINDING_SOURCE);
	parent_prop = obs_data_get_string(json, BINDING_PARENT_PROP);

	// yes, this is probably overkill, but it allows correct handling if we add more ways to use values in the future
	use_as = 0;
	const char *use_as_str = obs_data_get_string(json, BINDING_USE_AS);
	if (astrcmpi(use_as_str, BINDING_USE_AS_STR) == 0)
		use_as = USE_AS_STR;
	else if (astrcmpi(use_as_str, BINDING_USE_AS_BOOL) == 0)
		use_as = USE_AS_BOOL;
	else if (astrcmpi(use_as_str, BINDING_USE_AS_SWITCH) == 0)
		use_as = USE_AS_SWITCH;

	trim_str = obs_data_get_bool(json, BINDING_TRIM_STR);
	invert_bool = obs_data_get_bool(json, BINDING_INVERT_BOOL);
	value_if_true = obs_data_get_string(json, BINDING_VALUE_IF_TRUE);
	value_if_false = obs_data_get_string(json, BINDING_VALUE_IF_FALSE);
}

obs_data_t *Binding::toJSON() const
{
	obs_data_t *json = obs_data_create();

	obs_data_set_bool(json, BINDING_ENABLED, enabled);
	obs_data_set_string(json, BINDING_NAME, name.toUtf8().constData());
	obs_data_set_int(json, BINDING_ITEMNO, item_number);
	obs_data_set_int(json, BINDING_LENGTH, field_length);
	obs_data_set_string(json, BINDING_SOURCE,
			    source_id.toUtf8().constData());
	obs_data_set_string(json, BINDING_PARENT_PROP,
			    parent_prop.toUtf8().constData());

	// see note on this above
	const char *use_as_strval = BINDING_USE_AS_STR;
	switch (use_as) {
	case USE_AS_STR:
		use_as_strval = BINDING_USE_AS_STR;
		break;
	case USE_AS_BOOL:
		use_as_strval = BINDING_USE_AS_BOOL;
		break;
	case USE_AS_SWITCH:
		use_as_strval = BINDING_USE_AS_SWITCH;
		break;
	}
	obs_data_set_string(json, BINDING_USE_AS, use_as_strval);

	obs_data_set_bool(json, BINDING_TRIM_STR, trim_str);
	obs_data_set_bool(json, BINDING_INVERT_BOOL, invert_bool);
	obs_data_set_string(json, BINDING_VALUE_IF_TRUE,
			    value_if_true.toUtf8().constData());
	obs_data_set_string(json, BINDING_VALUE_IF_FALSE,
			    value_if_false.toUtf8().constData());

	return json;
}

Settings::Settings(QWidget *parent) : QDialog(parent), ui(new Ui::Settings)
{
	ui->setupUi(this);

	// Remove the ? button on dialogs on Windows
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	// Set up slots
	connect(ui->connectToUDS, &QCheckBox::stateChanged, this,
		&Settings::connectToUDSChanged);
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this,
		&Settings::okClicked);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this,
		&Settings::cancelClicked);

	// set up defaults - if a config is found, these will be overwritten later
	enableReceiver = false;
	connectToUDS = false;
	udsAddr = "";
	udsPort = 0;
	listenAddr = "0.0.0.0";
	listenPort = 21000;
	validateChecksums = true;

	config_t *config = obs_frontend_get_global_config();

	// check that the scoreboard section exists
	bool scoreboardSectionExists = false;
	for (size_t i = 0; i < config_num_sections(config); i++) {
		const char *section_name = config_get_section(config, i);
		if (astrcmpi(section_name, CFG_SECTION) == 0) {
			scoreboardSectionExists = true;
			break;
		}
	}
	if (!scoreboardSectionExists) {
		blog(LOG_WARNING, "No configuration for " PLUGIN_NAME
				  " found. Falling back to defaults.");
		return;
	}

	// load from config
	enableReceiver =
		config_get_bool(config, CFG_SECTION, CFG_RECEIVER_RUNNING);
	connectToUDS = config_get_bool(config, CFG_SECTION, CFG_CONNECT_TO_UDS);
	udsAddr = config_get_string(config, CFG_SECTION, CFG_UDS_ADDR);
	udsPort = config_get_uint(config, CFG_SECTION, CFG_UDS_PORT);
	listenAddr = config_get_string(config, CFG_SECTION, CFG_LISTEN_ADDR);
	listenPort = config_get_uint(config, CFG_SECTION, CFG_LISTEN_PORT);
	validateChecksums =
		config_get_bool(config, CFG_SECTION, CFG_VALIDATE_CHECKSUMS);

	// load binding list
	QByteArray bindingsJSON =
		config_get_string(config, CFG_SECTION, CFG_BINDINGS_JSON);
	OBSDataAutoRelease bindingsObj =
		obs_data_create_from_json(bindingsJSON.constData());
	OBSDataArrayAutoRelease bindingsArr =
		obs_data_get_array(bindingsObj, BINDINGS_JSON_KEY);

	size_t bindingsCount = obs_data_array_count(bindingsArr);
	for (size_t i = 0; i < bindingsCount; i++) {
		OBSDataAutoRelease item = obs_data_array_item(bindingsArr, i);
		bindings.emplace_back(item);
	}
}

Settings::~Settings()
{
	delete ui;
}

void Settings::save()
{
	config_t *config = obs_frontend_get_global_config();

	config_set_bool(config, CFG_SECTION, CFG_RECEIVER_RUNNING,
			enableReceiver);
	config_set_bool(config, CFG_SECTION, CFG_CONNECT_TO_UDS, connectToUDS);
	config_set_string(config, CFG_SECTION, CFG_UDS_ADDR,
			  udsAddr.toUtf8().constData());
	config_set_uint(config, CFG_SECTION, CFG_UDS_PORT, udsPort);
	config_set_string(config, CFG_SECTION, CFG_LISTEN_ADDR,
			  listenAddr.toUtf8().constData());
	config_set_uint(config, CFG_SECTION, CFG_LISTEN_PORT, listenPort);
	config_set_bool(config, CFG_SECTION, CFG_VALIDATE_CHECKSUMS,
			validateChecksums);

	OBSDataArrayAutoRelease bindingsArr = obs_data_array_create();
	for (auto binding : bindings) {
		OBSDataAutoRelease bindingObj = binding.toJSON();
		obs_data_array_push_back(bindingsArr, bindingObj);
	}

	OBSDataAutoRelease bindingsObj = obs_data_create();

	// add the array and release - the library has it from here
	obs_data_set_array(bindingsObj, BINDINGS_JSON_KEY, bindingsArr);

	// serialize and save
	QByteArray json = obs_data_get_json(bindingsObj);
	std::string b64 = json.toBase64().toStdString();
	config_set_string(config, CFG_SECTION, CFG_BINDINGS_JSON, b64.c_str());

	config_save(config);
}

void Settings::toggleVisible(bool checked)
{
	UNUSED_PARAMETER(checked);
	setVisible(!isVisible());
}

void Settings::connectToUDSChanged(int newValue)
{
	bool checked = newValue;
	ui->udsAddr->setEnabled(checked);
	ui->udsPort->setEnabled(checked);
}

void Settings::okClicked()
{
	enableReceiver = ui->enableReceiver->isChecked();
	connectToUDS = ui->connectToUDS->isChecked();
	udsAddr = ui->udsAddr->text();
	udsPort = ui->udsPort->value();
	listenAddr = ui->localAddr->text();
	listenPort = ui->localPort->value();
	validateChecksums = ui->validateChecksums->isChecked();
	save();
}

void Settings::cancelClicked()
{
	// put everything back
	ui->enableReceiver->setChecked(enableReceiver);
	ui->connectToUDS->setChecked(connectToUDS);
	connectToUDSChanged(ui->connectToUDS->isChecked());
	ui->udsAddr->setText(udsAddr);
	ui->udsPort->setValue(udsPort);
	ui->localAddr->setText(listenAddr);
	ui->localPort->setValue(listenPort);
	ui->validateChecksums->setChecked(validateChecksums);
}
