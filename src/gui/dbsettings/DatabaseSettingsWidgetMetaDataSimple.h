/*
 *  Copyright (C) 2018 KeePassXC Team <team@keepassxc.org>
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

#ifndef KEEPASSXC_DATABASESETTINGSWIDGEMETADATA_H
#define KEEPASSXC_DATABASESETTINGSWIDGEMETADATA_H

#include "DatabaseSettingsWidget.h"

class Database;
namespace Ui
{
    class DatabaseSettingsWidgetMetaDataSimple;
}

class DatabaseSettingWidgetMetaData : public DatabaseSettingsWidget
{
    Q_OBJECT

public:
    explicit DatabaseSettingWidgetMetaData(QWidget* parent = nullptr);
    Q_DISABLE_COPY(DatabaseSettingWidgetMetaData);
    ~DatabaseSettingWidgetMetaData() override;

    inline bool hasAdvancedMode() const override
    {
        return false;
    }

    void initialize() override;
    void uninitialize() override;
    bool save() override;

protected:
    void showEvent(QShowEvent* event) override;

private:
    const QScopedPointer<Ui::DatabaseSettingsWidgetMetaDataSimple> m_ui;
};

#endif // KEEPASSXC_DATABASESETTINGSWIDGEMETADATA_H
