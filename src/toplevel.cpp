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
#include "toplevel.h"
#include "timeedit.h"
#include "settings.h"
#include "tomato.h"

#include <QString>
#include <QAction>
#include <QTimer>
#include <QPainter>
#include <QBrush>
#include <QActionGroup>

#include <KMenu>
#include <KActionCollection>
#include <KHelpMenu>
#include <KPassivePopup>
#include <KGlobalSettings>
#include <KNotification>
#include <KAboutData>


TopLevel::TopLevel(const KAboutData *aboutData, const QString &icon, QWidget *parent)
  : KStatusNotifierItem( parent ),
    m_popup( new KPassivePopup ),
    m_runningTomatoTime( 0 ),
    m_nextNotificationTime( 0 )
{
    setIconByName(icon);
    setCategory(ApplicationStatus);
    setStatus(Active);
    KSharedConfigPtr config = KSharedConfig::openConfig();
    KConfigGroup tomatolistGroup( config, "Tomatolist" );

    if( tomatolistGroup.exists() ) {
        QString key, time;
        for(unsigned index=0; tomatolistGroup.hasKey(time.sprintf("Tomato%d Time", index)) && tomatolistGroup.hasKey(key.sprintf("Tomato%d Name", index)); ++index) {
            if( int temp = ( tomatolistGroup.readEntry( time, QString() ) ).toUInt() ) {
                m_tomatolist.append( Tomato( tomatolistGroup.readEntry( key, i18n( "Unknown Tomato" ) ), temp ) );
            }
        }
    }

    // If the list of tomatos is empty insert a set of default tomatos.
    if( m_tomatolist.isEmpty() ) {
        m_tomatolist.append( Tomato(i18n( "Black Tomato" ), 180 ) );
        m_tomatolist.append( Tomato(i18n( "Earl Grey" ), 300 ) );
        m_tomatolist.append( Tomato(i18n( "Fruit Tomato" ), 480 ) );
    }


    m_tomatoActionGroup=new QActionGroup( this );

    // build the context menu
    m_stopAct = new QAction( QLatin1String( "stop" ), this );
    m_stopAct->setIcon( KIcon( QLatin1String(  "edit-delete" ) ) );
    m_stopAct->setText( i18n( "&Stop" ) );
    m_stopAct->setEnabled( false );

    m_confAct = new QAction( QLatin1String( "configure" ), this );
    m_confAct->setIcon(KIcon( QLatin1String(  "configure" ) ) );
    m_confAct->setText(i18n( "&Configure..." ) );

    m_anonAct = new QAction( QLatin1String( "anonymous" ), this );
    m_anonAct->setText(i18n( "&Anonymous..." ) );

    m_exitAct = actionCollection()->action( QLatin1String( KStandardAction::name( KStandardAction::Quit ) ));
    m_exitAct->setShortcut( 0 ); // Not sure if it is correct.

    m_helpMenu = new KHelpMenu( 0, aboutData, false );


    loadTomatoMenuItems();
    contextMenu()->addSeparator();
    contextMenu()->addAction( m_stopAct );
    contextMenu()->addAction( m_anonAct );
    contextMenu()->addSeparator();
    contextMenu()->addAction( m_confAct );
    contextMenu()->addMenu( m_helpMenu->menu() );
    contextMenu()->addAction( m_exitAct );

    m_timer = new QTimer( this );

    connect( m_timer, SIGNAL( timeout() ), this, SLOT( tomatoTimeEvent() ) );
    connect( m_stopAct, SIGNAL( triggered(bool) ), this, SLOT( cancelTomato() ) );
    connect( m_confAct, SIGNAL( triggered(bool) ), this, SLOT( showSettingsDialog() ) );
    connect( m_anonAct, SIGNAL( triggered(bool) ), this, SLOT( showTimeEditDialog() ) );
    connect( contextMenu(), SIGNAL( triggered(QAction*) ), this, SLOT( runTomato(QAction*) ) );
    connect ( this, SIGNAL( activateRequested(bool,QPoint) ), this, SLOT( showPopup(bool,QPoint) ) );

    loadConfig();
    checkState();
}


TopLevel::~TopLevel()
{
    delete m_helpMenu;
    delete m_timer;
    delete m_popup;
    delete m_tomatoActionGroup;
}


void TopLevel::checkState() {
    const bool state = m_runningTomatoTime != 0;

    m_tomatoActionGroup->setEnabled( !state );
    m_stopAct->setEnabled( state );
    m_anonAct->setEnabled( !state );

    if( !state ) {
        setTooltipText( i18n( "No steeping tomato." ) );
    }
}


void TopLevel::setTomatoList(const QList<Tomato> &tomatolist) {
    m_tomatolist=tomatolist;

    // Save list...
    KSharedConfigPtr config = KSharedConfig::openConfig();
    KConfigGroup tomatolistGroup( config, "Tomatolist" );

    tomatolistGroup.deleteGroup();

    for(int i=0; i<m_tomatolist.size(); ++i) {
        tomatolistGroup.writeEntry(QString( QLatin1String( "Tomato%1 Time" ) ).arg( i ), m_tomatolist.at( i ).time() );
        tomatolistGroup.writeEntry(QString( QLatin1String( "Tomato%1 Name" ) ).arg( i ), m_tomatolist.at( i ).name() );
    }
    tomatolistGroup.config()->sync();


    foreach(QAction* a, m_tomatoActionGroup->actions() ) {
        m_tomatoActionGroup->removeAction( a );
    }

    contextMenu()->clear();

    ( static_cast<KMenu*>( contextMenu() ) )->addTitle( qApp->windowIcon(), KGlobal::caption() );

    loadTomatoMenuItems();
    contextMenu()->addSeparator();
    contextMenu()->addAction( m_stopAct );
    contextMenu()->addAction( m_anonAct );
    contextMenu()->addSeparator();
    contextMenu()->addAction( m_confAct );
    contextMenu()->addMenu(m_helpMenu->menu() );
    contextMenu()->addAction( m_exitAct );

    loadConfig();
}


void TopLevel::loadTomatoMenuItems() {
    int i=0;

    foreach(const Tomato &t, m_tomatolist) {
        QAction *a = contextMenu()->addAction(
                   i18nc( "%1 - name of the tomato, %2 - the predefined time for "
                          "the tomato", "%1 (%2)", t.name(), t.timeToString() )
                     );

        a->setData( ++i );
        m_tomatoActionGroup->addAction( a );
    }
}


void TopLevel::showSettingsDialog()
{
    SettingsDialog( this, m_tomatolist ).exec();
}


void TopLevel::showTimeEditDialog()
{
    TimeEditDialog( this ).exec();
}


void TopLevel::runTomato(QAction *a)
{
    int index = a->data().toInt();
    if( index <= 0 ) {
        return;
    }

    --index;

    if( index > m_tomatolist.size() ) {
        return;
    }

    runTomato( m_tomatolist.at( index ) );
}


void TopLevel::runTomato(const Tomato &tomato)
{
    m_runningTomato = tomato;
    m_runningTomatoTime = m_runningTomato.time();

    checkState();
    repaintTrayIcon();

    m_timer->start( 1000 );
}


void TopLevel::repaintTrayIcon()
{
    if( m_runningTomatoTime != 0 && m_usevisualize) {
        setOverlayIconByName ( QLatin1String("task-ongoing") );
    }
    else
    {
        setOverlayIconByName ( QString() );
    }
}


void TopLevel::tomatoTimeEvent()
{
    QString title = i18n( "The Tomato Cooker" );

    if( m_runningTomatoTime == 0 ) {
        m_timer->stop();

        QString content = i18n( "%1 is now ready!", m_runningTomato.name() );

        //NOTICE Timeout is set to ~24 days when no auto hide is request. Ok - nearly the same...
        if( m_usepopup ) {
            showMessage( title, content, iconName(), m_autohide ? m_autohidetime*1000 : 2100000000 );
        }

        KNotification::event( QLatin1String( "ready" ), content );

        if( m_usereminder && m_remindertime > 0 ) {
            setTooltipText( content );

            m_timer->start( 1000 );
            m_nextNotificationTime -= m_remindertime;
            --m_runningTomatoTime;
        }
        checkState();
        repaintTrayIcon();
    }
    else if(m_runningTomatoTime<0) {
        QString content = i18n( "%1 is ready since %2!", m_runningTomato.name(), Tomato::int2time( m_runningTomatoTime*-1, true ) );
        setTooltipText( content );

        if( m_runningTomatoTime == m_nextNotificationTime ) {
            if( m_usepopup ) {
                showMessage( title, content, iconName(), m_autohide ? m_autohidetime*1000 : 2100000000 );
            }

            KNotification::event( QLatin1String( "reminder" ), content );

            m_nextNotificationTime -= m_remindertime;
        }
        --m_runningTomatoTime;
    }
    else {
        --m_runningTomatoTime;
        setTooltipText( i18nc( "%1 is the time, %2 is the name of the tomato", "%1 left for %2.", Tomato::int2time( m_runningTomatoTime, true ), m_runningTomato.name() ) );
    }

    if( m_usevisualize ) {
        repaintTrayIcon();
    }
}


void TopLevel::cancelTomato()
{
    m_timer->stop();
    m_runningTomatoTime = 0;
    checkState();
    repaintTrayIcon();
}


void TopLevel::loadConfig()
{
    KSharedConfigPtr config = KSharedConfig::openConfig();
    KConfigGroup generalGroup( config, "General" );

    m_usepopup=generalGroup.readEntry( "UsePopup", true );
    m_autohide=generalGroup.readEntry( "PopupAutoHide", false );
    m_autohidetime=generalGroup.readEntry( "PopupAutoHideTime", 30 );
    m_usereminder=generalGroup.readEntry( "UseReminder", false );
    m_remindertime=generalGroup.readEntry( "ReminderTime", 60 );
    m_usevisualize=generalGroup.readEntry( "UseVisualize", true );
}


void TopLevel::showPopup(bool active, const QPoint& point)
{
    Q_UNUSED( active )
    if ( m_popup->isVisible() )
    {
        m_popup->hide();
    }
    else
    {
        QPoint p = point;
        QRect r = KGlobalSettings::desktopGeometry( p );
        QSize popupSize = m_popup->minimumSizeHint();
        if ( p.x() + popupSize.width() > r.right() )
        {
            p.rx() -= popupSize.width();
        }
        if ( p.y() + popupSize.height() > r.bottom() )
        {
            p.ry() -= popupSize.height();
        }
        m_popup->show( p );
    }
}


void TopLevel::setTooltipText(const QString& content)
{
    const QString title = i18n( "The Tomato Cooker" );
    setToolTip( iconName(), title, content );
    m_popup->setView( title, content, KIcon( iconName() ).pixmap( 22,22 ) );
}


// kate: word-wrap off; encoding utf-8; indent-width 4; tab-width 4; line-numbers on; mixed-indent off; remove-trailing-space-save on; replace-tabs-save on; replace-tabs on; space-indent on;
// vim:set spell et sw=4 ts=4 nowrap cino=l1,cs,U1: