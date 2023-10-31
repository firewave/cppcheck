/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2022 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "showtypes.h"

#include "common.h"
#include "errortypes.h"

#include <QSettings>

ShowTypes::ShowTypes()
{
    load();
}

ShowTypes::~ShowTypes()
{
    save();
}

ShowTypes::ShowType ShowTypes::SeverityToShowType(Severity severity)
{
    switch (severity) {
    case Severity::none:
        return ShowNone;
    case Severity::error:
        return ShowErrors;
    case Severity::style:
        return ShowStyle;
    case Severity::warning:
        return ShowWarnings;
    case Severity::performance:
        return ShowPerformance;
    case Severity::portability:
        return ShowPortability;
    case Severity::information:
        return ShowInformation;
    default:
        return ShowNone;
    }
}

Severity ShowTypes::ShowTypeToSeverity(ShowType type)
{
    switch (type) {
    case ShowStyle:
        return Severity::style;

    case ShowErrors:
        return Severity::error;

    case ShowWarnings:
        return Severity::warning;

    case ShowPerformance:
        return Severity::performance;

    case ShowPortability:
        return Severity::portability;

    case ShowInformation:
        return Severity::information;

    case ShowNone:
    default:
        return Severity::none;
    }
}

ShowTypes::ShowType ShowTypes::VariantToShowType(const QVariant &data)
{
    const int value = data.toInt();
    if (value < ShowStyle || value > ShowErrors) {
        return ShowNone;
    }
    return (ShowType)value;
}

void ShowTypes::load()
{
    QSettings settings;
    mVisible[ShowStyle] = settings.value(SETTINGS_SHOW_STYLE, true).toBool();
    mVisible[ShowErrors] = settings.value(SETTINGS_SHOW_ERRORS, true).toBool();
    mVisible[ShowWarnings] = settings.value(SETTINGS_SHOW_WARNINGS, true).toBool();
    mVisible[ShowPortability] = settings.value(SETTINGS_SHOW_PORTABILITY, true).toBool();
    mVisible[ShowPerformance] = settings.value(SETTINGS_SHOW_PERFORMANCE, true).toBool();
    mVisible[ShowInformation] = settings.value(SETTINGS_SHOW_INFORMATION, true).toBool();
}

void ShowTypes::save() const
{
    QSettings settings;
    settings.setValue(SETTINGS_SHOW_STYLE, mVisible[ShowStyle]);
    settings.setValue(SETTINGS_SHOW_ERRORS, mVisible[ShowErrors]);
    settings.setValue(SETTINGS_SHOW_WARNINGS, mVisible[ShowWarnings]);
    settings.setValue(SETTINGS_SHOW_PORTABILITY, mVisible[ShowPortability]);
    settings.setValue(SETTINGS_SHOW_PERFORMANCE, mVisible[ShowPerformance]);
    settings.setValue(SETTINGS_SHOW_INFORMATION, mVisible[ShowInformation]);
}

bool ShowTypes::isShown(ShowType category) const
{
    return mVisible[category];
}

bool ShowTypes::isShown(Severity severity) const
{
    return isShown(SeverityToShowType(severity));
}

void ShowTypes::show(ShowType category, bool showing)
{
    mVisible[category] = showing;
}
