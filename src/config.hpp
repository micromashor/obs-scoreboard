#ifndef CONFIG_H
#define CONFIG_H

#include <obs.h>
#include <util/config-file.h>
#include <vector>
#include <QListWidget>

#define USE_AS_STR 0
#define USE_AS_BOOL 1
#define USE_AS_SWITCH 2

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

class Config {
public:
	Config();
	void save();

	bool receiverRunning;

	bool connectToUDS;
	QString UDSAddr;
	uint16_t UDSPort;
	QString listenAddr;
	uint16_t listenPort;
	bool validateChecksums;

	std::vector<Binding> bindings;
};

#endif // CONFIG_H