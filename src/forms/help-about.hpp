#ifndef HelpAbout_H
#define HelpAbout_H

#include <QDialog>
#include <QLabel>

namespace Ui {
class HelpAbout;
}

class HelpAbout : public QDialog {
	Q_OBJECT

public:
	enum counters { packetsReceived, framesReceived, framesDropped };

	explicit HelpAbout(QWidget *parent);
	~HelpAbout();

	void toggleVisible(bool checked);

	void incrementCounter(int which);

private slots:

	void counterChanged(int which, unsigned long long newval);

private:
	Ui::HelpAbout *ui;
};

#endif // HelpAbout_H
