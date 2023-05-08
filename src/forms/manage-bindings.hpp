#ifndef ManageBindings_H
#define ManageBindings_H

#include <QDialog>
#include <QLabel>
#include <QListWidgetItem>

namespace Ui {
class ManageBindings;
}

class ManageBindings : public QDialog {
	Q_OBJECT

public:
	explicit ManageBindings(QWidget *parent);
	~ManageBindings();

	void toggleVisible();

private slots:

	void rowChanged(int row);

	void rowRenamed(QListWidgetItem *item);

	void editClicked();

	void renameClicked();

	void addClicked();

	void deleteClicked();

private:
	void redraw();

	Ui::ManageBindings *ui;
};

#endif // ManageBindings_H
