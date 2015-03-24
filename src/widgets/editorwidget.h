/* This file is part of Zanshin Todo.

   Copyright 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
   USA.
*/


#ifndef EDITORWIDGET_H
#define EDITORWIDGET_H

#include <QWidget>

class KToolBar;
class QToolButton;
class KRichTextWidget;
class KActionCollection;

namespace Widgets {

/**
 * An editorwidget, with the toolbox buttons
 */
class EditorWidget: public QWidget
{
    Q_OBJECT
public:
    EditorWidget(QWidget *parent = 0);

    KRichTextWidget *editor();
    QList<QAction*> editActions() const;

public slots:
    void toggleToolbarVisibility();

protected:
    virtual void changeEvent(QEvent* );
    virtual void keyPressEvent(QKeyEvent* );

signals:
    void fullscreenToggled(bool);
    void toolbarVisibilityToggled(bool);

private:
    KRichTextWidget *m_editor;
    QToolButton *m_fullscreenButton;
    KToolBar *m_toolbar;
    const QColor m_defaultColor;
    QList<QAction*> m_editActions;
};

}

#endif // EDITORWIDGET_H
