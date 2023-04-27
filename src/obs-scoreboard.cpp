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

#include "plugin-macros.generated.h"

OBS_DECLARE_MODULE()

OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

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

	return true;
}

void obs_module_unload()
{
	blog(LOG_INFO, "Goodbye!");
}
