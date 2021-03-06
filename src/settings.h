/*
 * Copyright 2011 by Ruurd Pels <ruurd@tiscali.nl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include "ui_settings.h"

class TopLevel;
class TomatoListModel;
class Tomato;
class SettingsUI;

/**
 * @short the settings dialog
 *
 * @author Ruurd Pels <ruurd@tiscali.nl>
 */
class SettingsDialog: public KDialog
{
    Q_OBJECT
public:
    SettingsDialog(TopLevel *toplevel, const QList<Tomato> &tomatos);
    ~SettingsDialog();

private slots:
    void updateSelection(const QItemSelection &selected, const QItemSelection &deselected);
    void accept();
    void checkPopupButtonState(bool b);
    void confButtonClicked();

    void newButtonClicked();
    void removeButtonClicked();
    void upButtonClicked();
    void downButtonClicked();

    void nameValueChanged(const QString &text);
    void timeValueChanged();

private:
    void moveSelectedItem(bool moveup);

private:
    SettingsUI *ui;
    TopLevel *m_toplevel;
    TomatoListModel *m_model;
};


#endif

// kate: word-wrap off; encoding utf-8; indent-width 4; tab-width 4; line-numbers on; mixed-indent off; remove-trailing-space-save on; replace-tabs-save on; replace-tabs on; space-indent on;
// vim:set spell et sw=4 ts=4 nowrap cino=l1,cs,U1:
