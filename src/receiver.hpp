#ifndef OBSSB_RECEIVER_HPP
#define OBSSB_RECEIVER_HPP

#include <obs.h>

#include <vector>

#include <QString>
#include <QUdpSocket>
#include <QHostAddress>

class Binding {
public:
	Binding();
	Binding(obs_data_t *json);
	obs_data_t *toJSON() const;
	void resetSource();

	bool enabled;
	bool trim_str;
	bool invert_bool;
	uint32_t flag_value;
	std::string name;
	uint32_t item_number;
	uint32_t field_length;
	std::string source_id;
	std::vector<std::string> parent_prop;
};

#define COUNTER_PACKETS 0
#define COUNTER_FRAMES 1
#define COUNTER_ERRORS 2

#define COUNTERS_COUNT 3

class Receiver : public QObject {
	Q_OBJECT

public:
	Receiver();
	~Receiver();

	inline bool isEnabled() { return socket != nullptr; }

	void updateReceiver(bool enabled);

	QHostAddress udsAddr;
	quint16 udsPort;
	QHostAddress listenAddr;
	quint16 listenPort;

	bool validateChecksums;

	std::vector<Binding> bindings;

public slots:
	void socketReady();

	void socketError(QAbstractSocket::SocketError err);

	void saveConfig() const;

signals:
	void counterChanged(int which, unsigned long long newval);

private:
	QUdpSocket *socket;

	// counter mechanics
	unsigned long long counters[COUNTERS_COUNT];
	inline void incrementCounter(int which)
	{
		counters[which]++;
		emit counterChanged(which, counters[which]);
	}

	std::string scoreData;

	void processDatagram(const std::string_view &data);
	const char *processFrame(const std::string_view &frame);
	void updateSources();
};

#endif // OBSSB_RECEIVER_HPP
