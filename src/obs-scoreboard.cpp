/*
OBS-Scoreboard
Copyright (C) 2023 Eric Johnson <micromashor@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <QMainWindow>
#include <QAction>
#include <QMenu>

#include "receiver.hpp"
#include "forms/settings.hpp"
#include "forms/help-about.hpp"
#include "forms/manage-bindings.hpp"
#include "forms/configure-binding.hpp"

#include "plugin-macros.generated.h"

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

Settings *settings;
HelpAbout *helpAbout;
ManageBindings *bindings;
ConfigureBinding *config;
Receiver *receiver;

const char *obs_module_name()
{
	return PLUGIN_NAME;
}

const char *obs_module_description()
{
	return "Integration to bind scoreboard information into OBS Sources";
}

bool obs_module_load()
{
	blog(LOG_INFO, "Hello! (%s)", PLUGIN_VERSION);

	QMainWindow *mainWindow = (QMainWindow *)obs_frontend_get_main_window();
	if (!mainWindow)
		return true;

	receiver = new Receiver();

	obs_frontend_push_ui_translation(obs_module_get_string);
	settings = new Settings(mainWindow);
	helpAbout = new HelpAbout(mainWindow);
	bindings = new ManageBindings(mainWindow);
	config = new ConfigureBinding(mainWindow);
	obs_frontend_pop_ui_translation();

	QMenu *menu = new QMenu(T("OBSScoreboard.Menu"), mainWindow);

	QAction *settingsAction =
		new QAction(T("OBSScoreboard.Menu.Settings"), menu);
	QObject::connect(settingsAction, &QAction::triggered, settings,
			 &Settings::toggleVisible);
	menu->addAction(settingsAction);

	QAction *helpAction = new QAction(T("OBSScoreboard.Menu.Help"), menu);
	QObject::connect(helpAction, &QAction::triggered, helpAbout,
			 &HelpAbout::toggleVisible);
	menu->addAction(helpAction);

	QAction *bindingsAction =
		new QAction(T("OBSScoreboard.Menu.Bindings"), menu);
	QObject::connect(bindingsAction, &QAction::triggered, bindings,
			 &ManageBindings::toggleVisible);
	menu->addAction(bindingsAction);

	QAction *toolsQAction = (QAction *)obs_frontend_add_tools_menu_qaction(
		T("OBSScoreboard.Menu"));
	toolsQAction->setMenu(menu);

	return true;
}

void obs_module_unload()
{
	delete settings;

	blog(LOG_INFO, "Goodbye!");
}
