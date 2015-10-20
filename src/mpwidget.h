/*
 *  qmpwidget - A Qt widget for embedding MPlayer
 *  Copyright (C) 2010 by Jonas Gehring
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef MPWIDGET_H
#define MPWIDGET_H


#include <QHash>
#include <QTimer>
#include <QWidget>
#include <QStringList>
#include "mpmediainfo.h"
#include "focusstack.h"

class QSlider;
class QLabel;
class QSvgRenderer;
class OverlayQuit;
class QToolButton;
class QIcon;

#include "mpprocess.h"

class MpPlainVideoWidget;

class MpWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(MpState state READ state)
    Q_PROPERTY(double streamPosition READ current_position)
    Q_PROPERTY(QString videoOutput READ videoOutput WRITE setVideoOutput)
    Q_PROPERTY(QString mplayerPath READ mplayerPath WRITE setMPlayerPath)

public:
    typedef QWidget super;

private:
    bool m_fullscreen;
    MpProcess *m_process;
    unsigned m_process_startcount;
    MpMediaInfo m_mediaInfo;
    QString m_currently_playing;

    QWidget *m_background;
    MpPlainVideoWidget *m_widget;
    QSlider *m_seek_slider;
    QLabel *m_sliderlabel;
    QLabel *m_hourglass;
    QSvgRenderer *m_hourglass_render;

    QIcon *m_icon_tb_cycle_alang;
    QIcon *m_icon_tb_cycle_slang;
    QIcon *m_icon_tb_playpause_dopause;
    QIcon *m_icon_tb_playpause_doplay;
    QIcon *m_icon_tb_playpause_inactive;

    QToolButton *m_tb_playpause;
    QToolButton *m_tb_cycle_alang;
    QToolButton *m_tb_cycle_slang;

    QTimer m_hide_slider_timer;

    QStringList m_args;

    QStringList m_preferred_alangs;
    QStringList m_preferred_slangs;
    QString m_videooutput;
    QString m_mplayerpath;
    bool m_stay_dead;

    double m_startpos;

    QSet<QString> m_forbidden_alangs;

    OverlayQuit *m_helpscreen;

private:
    // forbid
    MpWidget();
    MpWidget(const MpWidget &);
    MpWidget &operator=(const MpWidget &in);

public:
    explicit MpWidget(QWidget *parent, bool fullscreen);
    virtual ~MpWidget();

public:

    virtual QSize sizeHint() const;
    virtual bool eventFilter(QObject *watched, QEvent *event);

protected:
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual bool event(QEvent *event);

public:
    MpState state() const;
    const MpMediaInfo &mediaInfo() const;
    double current_position() const;
    double last_position() const;

    void setVideoOutput(const QString &output);
    const QString &videoOutput() const;

    void setMplayerArgs(const QStringList &in);
    const QStringList mplayerArgs()const;

    void setMPlayerPath(const QString &path);
    const QString &mplayerPath() const;

    const QStringList &processed_mplayer_args() const;
    const QList<MpProcess::MpProcessIolog> &mplayer_out() const;

    void set_preferred_alangs(const QStringList &als);
    void set_preferred_slangs(const QStringList &sls);
    bool try_alang(const QString &alang);
    bool try_slang(const QString &slang);

    void set_forbidden_alangs(const QSet<QString> &in_forbidden);

    void set_crop(const QString &);

    void start_process();
    void load(const QString &url, const MpMediaInfo &mmi, const double startpos = 0.);

private:
    QRect compute_widget_new_geom() const;
    void connect_seekslider(bool doconnect);
    void init_process();
    QString sliderlabelstring(double pos) const;
    void adjust_slider_to(double position);
    void set_deinterlace(bool toggle);
    void submit_write(const QString &command);
    void submit_write_latin1(char const *const latin1lit);
    void print_all_info();
    void setVolume_abs(int volume);
    void setVolume_rel(int rvolume);
    void testKillMplayer();
    void toggle_playpause();
    void cycle_alang();
    void cycle_slang();

public slots:
    // level: minimum level to be visible, 0 = always
    //void slot_osd_show_property_text(const QString &format, size_t duration_ms = osd_default_duration_ms, size_t level = 0);
    //void slot_osd_show_text(const QString &str, size_t duration_ms = osd_default_duration_ms, size_t level = 0);

public slots:
    // from MpProcess
    void slot_mpStateChanged(MpState oldstate, MpState newstate);
    void slot_mpStreamPositionChanged(double position);
    void slot_mpSeekedTo(double position);
    void slot_load_is_done();
    void slot_error_received_at(const QString &s, double lastpos);

    // internal, from timers etc
    void slot_hidemouse();
    void slot_unhidemouse();
    // from seekslider
    void slot_seekslidermoved(int);
    // internal, a slot for delay
    void slot_updateWidgetSize();
    // internal, after a delay
    void slot_show_helpscreen();
    void slot_set_not_showing_help();

    // from QToolButtons
    void slot_tb_playpause_clicked();
    void slot_tb_cycle_alang_clicked();
    void slot_tb_cycle_slang_clicked();

signals:
    void sig_stateChanged(MpState oldstate, MpState newstate);
    void sig_error_while(const QString &reason, const QString &url, const MpMediaInfo &mmi, double lastpos);
    void sig_I_give_up();
    void sig_loadDone();
    void sig_recordbad(const BadMovieRecord_t &);

public slots:

    void show_and_take_focus(QWidget *oldfocusguess);
    void hide_and_return_focus();
private:
    FocusStack m_focusstack;
};


#endif // MPWIDGET_H
