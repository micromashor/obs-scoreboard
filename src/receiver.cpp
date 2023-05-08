#include <util/dstr.hpp>
#include <util/config-file.h>
#include <obs-frontend-api.h>
#include <obs.hpp>

#include <exception>

#include <QMainWindow>
#include <QMessageBox>

#include "receiver.hpp"

#include "plugin-macros.generated.h"

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
#define BINDING_PARENT_PROP_ELEM "parent_prop_elem"
#define BINDING_TRIM_STR "trim_str"
#define BINDING_INVERT_BOOL "invert_bool"
#define BINDING_FLAG_VALUE "flag_value"

Binding::Binding()
{
	enabled = false;
	name = obs_module_text("OBSScoreboard.Bindings.NewName");
	item_number = 1;
	field_length = 1;
	source_id = "";
	parent_prop = {};
	trim_str = false;
	invert_bool = false;
	flag_value = 0;
}

Binding::Binding(obs_data_t *json)
{
	enabled = obs_data_get_bool(json, BINDING_ENABLED);
	name = obs_data_get_string(json, BINDING_NAME);
	item_number = obs_data_get_int(json, BINDING_ITEMNO);
	field_length = obs_data_get_int(json, BINDING_LENGTH);
	source_id = obs_data_get_string(json, BINDING_SOURCE);

	OBSDataArrayAutoRelease parent_prop_arr =
		obs_data_get_array(json, BINDING_PARENT_PROP);
	size_t parent_prop_components = obs_data_array_count(parent_prop_arr);
	for (size_t i = 0; i < parent_prop_components; i++) {
		OBSDataAutoRelease obj =
			obs_data_array_item(parent_prop_arr, i);
		parent_prop.emplace_back(
			obs_data_get_string(obj, BINDING_PARENT_PROP_ELEM));
	}

	trim_str = obs_data_get_bool(json, BINDING_TRIM_STR);
	invert_bool = obs_data_get_bool(json, BINDING_INVERT_BOOL);
	flag_value = obs_data_get_int(json, BINDING_FLAG_VALUE);
}

obs_data_t *Binding::toJSON() const
{
	obs_data_t *json = obs_data_create();

	obs_data_set_bool(json, BINDING_ENABLED, enabled);
	obs_data_set_string(json, BINDING_NAME, name.c_str());
	obs_data_set_int(json, BINDING_ITEMNO, item_number);
	obs_data_set_int(json, BINDING_LENGTH, field_length);
	obs_data_set_string(json, BINDING_SOURCE, source_id.c_str());

	OBSDataArrayAutoRelease parent_prop_arr = obs_data_array_create();
	for (auto &prop : parent_prop) {
		OBSDataAutoRelease obj = obs_data_create();
		obs_data_set_string(obj, BINDING_PARENT_PROP_ELEM,
				    prop.c_str());
		obs_data_array_push_back(parent_prop_arr, obj);
	}
	obs_data_set_array(json, BINDING_PARENT_PROP, parent_prop_arr);

	obs_data_set_bool(json, BINDING_TRIM_STR, trim_str);
	obs_data_set_bool(json, BINDING_INVERT_BOOL, invert_bool);

	obs_data_set_int(json, BINDING_FLAG_VALUE, flag_value);

	return json;
}

void Binding::resetSource()
{
	enabled = false;
	source_id = "";
	parent_prop = {};
	trim_str = false;
	invert_bool = false;
	flag_value = 0;
}

Receiver::Receiver()
{
	for (auto &counter : counters)
		counter = 0;

	// set up defaults - if a config is found, these will be overwritten later
	udsAddr = QHostAddress::Null;
	udsPort = 20999;
	listenAddr = QHostAddress::Any;
	listenPort = 21000;
	socket = nullptr;
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
	bool enableReceiver =
		config_get_bool(config, CFG_SECTION, CFG_RECEIVER_RUNNING);

	if (config_get_bool(config, CFG_SECTION, CFG_CONNECT_TO_UDS)) {
		udsAddr = QHostAddress(
			config_get_string(config, CFG_SECTION, CFG_UDS_ADDR));
		udsPort = config_get_uint(config, CFG_SECTION, CFG_UDS_PORT);
	}
	listenAddr = QHostAddress(
		config_get_string(config, CFG_SECTION, CFG_LISTEN_ADDR));
	listenPort = config_get_uint(config, CFG_SECTION, CFG_LISTEN_PORT);
	validateChecksums =
		config_get_bool(config, CFG_SECTION, CFG_VALIDATE_CHECKSUMS);

	// load binding list
	const char *bindings_b64 =
		config_get_string(config, CFG_SECTION, CFG_BINDINGS_JSON);
	auto bindingsJSON = QByteArray::fromBase64(bindings_b64);
	OBSDataAutoRelease bindingsObj =
		obs_data_create_from_json(bindingsJSON.constData());
	OBSDataArrayAutoRelease bindingsArr =
		obs_data_get_array(bindingsObj, BINDINGS_JSON_KEY);

	size_t bindingsCount = obs_data_array_count(bindingsArr);
	for (size_t i = 0; i < bindingsCount; i++) {
		OBSDataAutoRelease item = obs_data_array_item(bindingsArr, i);
		bindings.emplace_back(item);
	}

	updateReceiver(enableReceiver);
}

Receiver::~Receiver()
{
	if (socket)
		delete socket;
}

void Receiver::saveConfig() const
{
	config_t *config = obs_frontend_get_global_config();

	config_set_bool(config, CFG_SECTION, CFG_RECEIVER_RUNNING,
			socket != nullptr);
	config_set_bool(config, CFG_SECTION, CFG_CONNECT_TO_UDS,
			!udsAddr.isNull());
	if (!udsAddr.isNull()) {
		config_set_string(config, CFG_SECTION, CFG_UDS_ADDR,
				  udsAddr.toString().toUtf8().constData());
		config_set_uint(config, CFG_SECTION, CFG_UDS_PORT, udsPort);
	}
	config_set_string(config, CFG_SECTION, CFG_LISTEN_ADDR,
			  listenAddr.toString().toUtf8().constData());
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

void Receiver::updateReceiver(bool enabled)
{
	blog(LOG_INFO, "updating server (%s)", enabled ? "ON" : "OFF");
	if (socket) {
		delete socket;
		socket = nullptr;
	}

	if (!enabled)
		return;

	socket = new QUdpSocket(this);
	connect(socket, &QIODevice::readyRead, this, &Receiver::socketReady);
	connect(socket, &QAbstractSocket::errorOccurred, this,
		&Receiver::socketError);
	if (!socket->bind(listenAddr, listenPort)) {
		QMainWindow *mainWindow =
			(QMainWindow *)obs_frontend_get_main_window();
		QMessageBox::critical(mainWindow,
				      T("OBSScoreboard.Error.Critical"),
				      T("OBSScoreboard.Error.BindFailed"));
		delete socket;
		socket = nullptr;
		return;
	}
	if (!udsAddr.isNull()) {
		socket->connectToHost(udsAddr, udsPort);
	}

	saveConfig();
}

void Receiver::socketReady()
{
	// process all available datagrams
	while (socket->hasPendingDatagrams()) {
		qint64 len = socket->pendingDatagramSize();

		char *buf = new char[len];

		socket->readDatagram(buf, len);

		std::string_view data(buf, len);

		processDatagram(data);

		delete buf;
	}

	// no need to update sources until we've dealt with all pending packets
	updateSources();
}

void Receiver::socketError(QAbstractSocket::SocketError err)
{
	auto msg = socket->errorString().toUtf8().constData();
	blog(LOG_ERROR, "Socket error: %s (%d)", msg, err);
}

#define SYN '\x16'
#define SOH '\x01'
#define STX '\x02'
#define EOT '\x04'
#define ETB '\x17'

#define IS_CONTROL_CHAR(c) (c < 0x20)

void Receiver::processDatagram(const std::string_view &data)
{
	std::string_view::iterator begin = data.begin();

	for (auto it = data.begin(); it != data.end(); it++) {
		switch (*it) {
		case SYN:
			begin = it;
			break;
		case ETB:
			std::string_view frame(begin, it - begin);
			const char *error = processFrame(frame);

			if (!error) {
				incrementCounter(COUNTER_FRAMES);
				break;
			}

			blog(LOG_WARNING, "dropping frame: %s (packet: %s)",
			     error,
			     QByteArray(frame.data(), frame.size())
				     .toBase64()
				     .constData());

			incrementCounter(COUNTER_ERRORS);
			break;
		}
	}

	incrementCounter(COUNTER_PACKETS);
}

const char *Receiver::processFrame(const std::string_view &frame)
{
	enum { NONE, SYNC, HEAD, BODY, CHECKSUM } section = NONE;

	size_t offset = 0;
	uint8_t calculatedChecksum = 0, receivedChecksum = 0;
	auto bodyStart = frame.cbegin(), bodyEnd = frame.cend();

	for (auto it = frame.cbegin(); it != frame.cend(); it++) {
		char c = *it;

		if (section != NONE && section != CHECKSUM)
			calculatedChecksum += c;

		if (IS_CONTROL_CHAR(c)) {
			// advance the section in lock-step by control characters
			// drop the frame if anything unexpected happens
			switch (c) {
			case SYN:
				if (section != NONE)
					return "unexpected SYN";
				section = SYNC;
				break;
			case SOH:
				if (section != SYNC)
					return "unexpected SOH";
				section = HEAD;
				break;
			case STX:
				if (section != HEAD)
					return "unexpected STX";
				section = BODY;
				bodyStart = it + 1;
				break;
			case EOT:
				// I don't want to clutter the logs with "unexpected EOT"
				// error messages, since the AS5000 actually does send frames
				// with no body from time to time, but I don't know why.
				// Since I don't consider this a packet error, I don't increment
				// the "dropped frames" counter
				if (section != BODY)
					return nullptr;
				section = CHECKSUM;
				bodyEnd = it;
				break;
			default:
				return "illegal control character";
			}
			// no further processing needed
			continue;
		}
		switch (section) {
		case NONE:
			// unexpected; abort processing
			return "data outside of frame";
		case SYNC:
			// this is for synchronization of the electrical current loop, so it doesn't pertain to us
			break;
		case HEAD:
			if (c < '0' || c > '9')
				return "non-digit in offset field";

			offset *= 10;
			offset += c - '0';
			offset %= 100000;
			break;
		case BODY:
			// we just store iterators to the body, so no need to do anything here
			break;
		case CHECKSUM:
			receivedChecksum <<= 4;

			if (c >= '0' && c <= '9') {
				receivedChecksum += c - '0';
			} else if (c >= 'A' && c <= 'F') {
				receivedChecksum += c - 'A' + 0xA;
			} else {
				return "non-hex character in checksum field";
			}

			break;
		}
	}

	if (section != CHECKSUM)
		return "unexpected end of frame";

	if (calculatedChecksum != receivedChecksum)
		return "invalid checksum";

	size_t length = bodyEnd - bodyStart;

	if (offset + length > scoreData.size())
		scoreData.append(offset + length - scoreData.size(), ' ');

	scoreData.replace(offset, length, bodyStart, length);

	return nullptr;
}

static bool dataRangeToBool(const std::string_view &dataRange)
{
	for (char c : dataRange) {
		if (c != ' ')
			return true;
	}
	return false;
}

void Receiver::updateSources()
{
	for (auto &binding : bindings) {
		// skip over disabled bindings
		if (!binding.enabled)
			continue;

		OBSSourceAutoRelease source =
			obs_get_source_by_uuid(binding.source_id.c_str());

		if (!source.Get()) {
			binding.resetSource();
			continue;
		}

		auto dataRangeBegin =
			scoreData.begin() + binding.item_number - 1;
		auto dataRangeEnd = dataRangeBegin + binding.field_length;
		std::string_view dataRange(dataRangeBegin.base(),
					   dataRangeEnd - dataRangeBegin);

		OBSDataAutoRelease settings = obs_source_get_settings(source);
		obs_properties_t *props = obs_source_properties(source);

		for (auto it = binding.parent_prop.begin();
		     it != binding.parent_prop.end() - 1; it++) {
			obs_property_t *prop =
				obs_properties_get(props, it->c_str());
			if (obs_property_get_type(prop) == OBS_PROPERTY_GROUP) {
				props = obs_property_group_content(prop);
				settings = obs_data_get_obj(
					settings, obs_property_name(prop));
			}
		}

		obs_property_t *prop = obs_properties_get(
			props, binding.parent_prop.back().c_str());

		obs_property_type type = obs_property_get_type(prop);

		if (type == OBS_PROPERTY_BOOL) {
			bool val = dataRangeToBool(dataRange);
			if (binding.invert_bool)
				val = !val;
			obs_data_set_bool(settings, obs_property_name(prop),
					  val);
		} else if (type == OBS_PROPERTY_TEXT) {
			if (binding.trim_str) {
				while (dataRange.front() == ' ') {
					++dataRangeBegin;
					dataRange = std::string_view(
						dataRangeBegin.base(),
						dataRangeEnd - dataRangeBegin);
				}
				while (dataRange.back() == ' ') {
					--dataRangeEnd;
					dataRange = std::string_view(
						dataRangeBegin.base(),
						dataRangeEnd - dataRangeBegin);
				}
			}
			std::string str(dataRange);
			obs_data_set_string(settings, obs_property_name(prop),
					    str.c_str());
		} else if (type == OBS_PROPERTY_FONT) {
			bool val = dataRangeToBool(dataRange);
			if (binding.invert_bool)
				val = !val;

			OBSDataAutoRelease fontobj = obs_data_get_obj(
				settings, obs_property_name(prop));

			uint32_t flags = obs_data_get_int(fontobj, "flags");

			if (val)
				// set the flag
				flags |= binding.flag_value;
			else
				// clear the flag
				flags &= ~binding.flag_value;

			obs_data_set_int(fontobj, "flags", flags);
		}

		obs_source_update(source, settings);
		obs_source_update_properties(source);
	}
}
