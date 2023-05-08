#include <obs.hpp>

#include "configure-binding.hpp"
#include "manage-bindings.hpp"
#include "../receiver.hpp"

#include "ui_manage-bindings.h"

#include "../plugin-macros.generated.h"

extern Receiver *receiver;
extern ConfigureBinding *config;

ManageBindings::ManageBindings(QWidget *parent)
	: QDialog(parent), ui(new Ui::ManageBindings)
{
	ui->setupUi(this);

	// Remove the ? button on dialogs on Windows
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	connect(ui->bindingList, &QListWidget::currentRowChanged, this,
		&ManageBindings::rowChanged);
	connect(ui->bindingList, &QListWidget::itemChanged, this,
		&ManageBindings::rowRenamed);
	connect(ui->editButton, &QPushButton::clicked, this,
		&ManageBindings::editClicked);
	connect(ui->renameButton, &QPushButton::clicked, this,
		&ManageBindings::renameClicked);
	connect(ui->addBindingButton, &QPushButton::clicked, this,
		&ManageBindings::addClicked);
	connect(ui->removeBindingButton, &QPushButton::clicked, this,
		&ManageBindings::deleteClicked);
}

ManageBindings::~ManageBindings()
{
	delete ui;
}

void ManageBindings::toggleVisible()
{
	redraw();
	setVisible(!isVisible());
}

void ManageBindings::rowChanged(int row)
{
	bool active = row != -1;

	ui->removeBindingButton->setEnabled(active);
	ui->editButton->setEnabled(active);
	ui->renameButton->setEnabled(active);
}

void ManageBindings::renameClicked()
{
	auto current = ui->bindingList->currentItem();
	ui->bindingList->editItem(current);
}

void ManageBindings::editClicked()
{
	int index = ui->bindingList->currentIndex().row();
	auto &binding = receiver->bindings[index];
	config->openForBinding(&binding);
	return;
}

void ManageBindings::addClicked()
{
	receiver->bindings.emplace_back();
	receiver->saveConfig();
	redraw();

	// select the current row
	int newRowIndex = receiver->bindings.size() - 1;
	ui->bindingList->setCurrentRow(newRowIndex);
	rowChanged(newRowIndex);
}

void ManageBindings::deleteClicked()
{
	int currentIndex = ui->bindingList->currentRow();
	auto it = receiver->bindings.begin() + currentIndex;
	receiver->bindings.erase(it);
	receiver->saveConfig();
	redraw();
}

void ManageBindings::rowRenamed(QListWidgetItem *item)
{
	int currentIndex = ui->bindingList->indexFromItem(item).row();
	auto currentBinding = receiver->bindings.begin() + currentIndex;

	currentBinding->name = item->text().toStdString();
	receiver->saveConfig();
}

void ManageBindings::redraw()
{
	int currentRow = ui->bindingList->currentRow();

	ui->bindingList->clear();
	for (auto &binding : receiver->bindings) {
		ui->bindingList->addItem(binding.name.c_str());
	}

	int item_count = ui->bindingList->count();
	for (int i = 0; i < item_count; i++) {
		QListWidgetItem *item = ui->bindingList->item(i);
		if (!item)
			break;
		item->setFlags(item->flags() | Qt::ItemIsEditable);
	}

	ui->bindingList->setCurrentRow(currentRow);
}
