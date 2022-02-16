/*
 *  Copyright (C) 2021 KeePassXC Team <team@keepassxc.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 or (at your option)
 *  version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KEEPASSXC_REPORTSPAGEBROWSERSTATISTICS_H
#define KEEPASSXC_REPORTSPAGEBROWSERSTATISTICS_H

#include <QIcon>
#include <QSharedPointer>
#include <QString>

#include "ReportsDialog.h"

class ReportsWidgetBrowserStatistics;
class Database;
class QWidget;
template <class T> class QSharedPointer;

class ReportsPageBrowserStatistics : public IReportsPage
{
public:
    ReportsWidgetBrowserStatistics* m_browserWidget;

    ReportsPageBrowserStatistics();

    QString name() override;
    QIcon icon() override;
    QWidget* createWidget() override;
    void loadSettings(QWidget* widget, QSharedPointer<Database> db) override;
    void saveSettings(QWidget* widget) override;
};

#endif // KEEPASSXC_REPORTSPAGEBROWSERSTATISTICS_H
