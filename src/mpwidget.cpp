#include "mpwidget.h"

#include <QAbstractSlider>
#include <QKeyEvent>
#include <QPainter>
#include <QProcess>
#include <QStringList>
#include <QTemporaryFile>
#include <QtDebug>
#include <QSlider>
#include <QLabel>
#include <QStyle>
#include <QProxyStyle>
#include <QSvgRenderer>
#include <QApplication>
#include <QHash>
#include <QIcon>
#include <QToolButton>

#include <unistd.h>

// for kill(2)
#include <sys/types.h>
#include <signal.h>

#include "mpprocess.h"
#include "mpplainvideowidget.h"
#include "util.h"
#include "gui_overlayquit.h"
#include "safe_signals.h"
#include "event_desc.h"

#include <QLoggingCategory>

#define THIS_SOURCE_FILE_LOG_CATEGORY "QMP"
static Q_LOGGING_CATEGORY(category, THIS_SOURCE_FILE_LOG_CATEGORY)
#define MYDBG(msg, ...) qCDebug(category, msg, ##__VA_ARGS__)

#include <QLoggingCategory>
#define THIS_SOURCE_FILE_LOG_CATEGORY_CWNG "CWNG"
static Q_LOGGING_CATEGORY(category_cwng, THIS_SOURCE_FILE_LOG_CATEGORY_CWNG)
#define CWNGMYDBG(msg, ...) qCDebug(category_cwng, msg, ##__VA_ARGS__)

// give up after that many crashes
static_var const unsigned MP_PROCESS_MAX_START_COUNT = 20;
// accumulate that many output lines
static_var const unsigned max_lines_to_accumulate = 10000;
// hide the seek slider after that many milliseconds
static_var const qint64 hide_slider_timeout_ms = 2000;
// how far we seek on cursor, shift-cursor and ctrl-cursor
static_var const double seek_distance_1_sec = 10.;
static_var const double seek_distance_2_sec = 60.;
static_var const double seek_distance_3_sec = 600.;

#define INVOKE_DELAY_MP(X) QTimer::singleShot(0, m_process, SLOT(X));

class MySliderStyle : public QProxyStyle
{
public:
    typedef QProxyStyle super;

public:
    explicit MySliderStyle(QStyle *baseStyle = 0) : super(baseStyle) {}

    int styleHint(QStyle::StyleHint hint, const QStyleOption *option = 0,
                  const QWidget *widget = 0, QStyleHintReturn *returnData = 0) const
    {
        if(hint == QStyle::SH_Slider_AbsoluteSetButtons) {
            return (Qt::LeftButton | Qt::MidButton | Qt::RightButton);
        }

        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

void MpWidget::init_process()
{
    if(m_stay_dead) {
        return;
    }

    if(m_process != NULL) {
        MYDBG("init_process(): previous process detected, trying to call quit() on it");
        MpProcess *copy = m_process;
        m_process = NULL;
        copy->disconnect(this);
        copy->disconnect();
        copy->quit();
        copy->deleteLater();
    }

    if(m_process_startcount > MP_PROCESS_MAX_START_COUNT) {
        MYDBG("EMIT sig_I_give_up");
        emit sig_I_give_up();
        return;
    }

    m_process_startcount++;

    m_process = new MpProcess(this, &m_mediaInfo);

    m_process->set_output_accumulator_mode(MpProcess::Input | MpProcess::Output | MpProcess::Error);
    m_process->set_accumulated_output_maxlines(max_lines_to_accumulate);
    m_process->set_output_accumulator_ignore_default();

    if(!m_videooutput.isEmpty()) {
        m_process->setVideoOutput(m_videooutput);
    }

    if(!m_mplayerpath.isEmpty()) {
        m_process->setMplayerPath(m_mplayerpath);
    }

    XCONNECT(m_process, SIGNAL(sig_stateChanged(MpState, MpState)), this, SLOT(slot_mpStateChanged(MpState, MpState)), QUEUEDCONN);
    XCONNECT(m_process, SIGNAL(sig_streamPositionChanged(double)), this, SLOT(slot_mpStreamPositionChanged(double)), QUEUEDCONN);
    XCONNECT(m_process, SIGNAL(sig_error_at_pos(QString, double)), this, SLOT(slot_error_received_at(QString, double)), QUEUEDCONN);
    XCONNECT(m_process, SIGNAL(sig_loadDone()), this, SIGNAL(sig_loadDone()), QUEUEDCONN);
    XCONNECT(m_process, SIGNAL(sig_loadDone()), this, SLOT(slot_load_is_done()), QUEUEDCONN);
    XCONNECT(m_process, SIGNAL(sig_seekedTo(double)), this, SLOT(slot_mpSeekedTo(double)), QUEUEDCONN);

}

static QToolButton *make_button(char const *const oN_latin1lit, QWidget *parent, QIcon &icon)
{
    QToolButton *but = new QToolButton;
    but->setObjectName(QLatin1String(oN_latin1lit));
    but->setParent(parent);
    but->setIcon(icon);
    but->setFocusPolicy(Qt::NoFocus);
    but->setToolButtonStyle(Qt::ToolButtonIconOnly);
    but->setAutoFillBackground(true);
    but->setAutoRaise(false);
    but->setMouseTracking(true);
    but->installEventFilter(parent);

    {
        QPalette p = but->palette();
        p.setColor(QPalette::Window, Qt::white);
        but->setPalette(p);
    }

    but->setStyleSheet(QStringLiteral("QToolButton { border: 0px; background-color: white; }"));

    return but;
}

/*!
 * \brief Constructor
 *
 * \param parent Parent widget
 */
MpWidget::MpWidget(QWidget *parent, bool fullscreen)
    : super()
    , m_fullscreen(fullscreen)
    , m_process(NULL)
    , m_process_startcount(0)
    , m_background(NULL)
    , m_widget(NULL)
    , m_seek_slider(NULL)
    , m_sliderlabel(NULL)
    , m_hourglass(NULL)
    , m_hourglass_render(NULL)
    , m_icon_tb_cycle_alang(NULL)
    , m_icon_tb_cycle_slang(NULL)
    , m_icon_tb_playpause_dopause(NULL)
    , m_icon_tb_playpause_doplay(NULL)
    , m_icon_tb_playpause_inactive(NULL)
    , m_tb_playpause(NULL)
    , m_tb_cycle_alang(NULL)
    , m_tb_cycle_slang(NULL)
    , m_helpscreen(NULL)
    , m_focusstack(this)
{
    setObjectName(QStringLiteral("QMPWidget"));
    setParent(parent);

    m_stay_dead = false;
#ifdef FOCUSCRAP
    setFocusPolicy(Qt::StrongFocus);
#endif
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_background = new QWidget;
    m_background->setObjectName(QStringLiteral("QMPWidget_bckgnd"));
    m_background->setParent(this);
    m_background->setAutoFillBackground(true);
    m_background->setFocusPolicy(Qt::NoFocus);

    m_widget = new MpPlainVideoWidget(this);

    {
        QPalette p = palette();
        p.setColor(QPalette::Window, Qt::black);
        m_background->setPalette(p);
    }

    init_process();

    m_seek_slider = new QSlider(Qt::Horizontal);
    m_seek_slider->setObjectName(QStringLiteral("QMPWidget_sksl"));
    m_seek_slider->setParent(this);
    m_seek_slider->setTracking(false);
    m_seek_slider->setCursor(Qt::ArrowCursor);
    m_seek_slider->setPageStep(60);
    m_seek_slider->setAutoFillBackground(true);
    {
        QPalette p = m_seek_slider->palette();
        p.setColor(QPalette::Window, Qt::white);
        m_seek_slider->setPalette(p);
    }
    m_seek_slider->setMouseTracking(true);
    m_seek_slider->installEventFilter(this);
    this->setMouseTracking(true);
    m_seek_slider->setEnabled(false);
    connect_seekslider(true);
    m_seek_slider->setStyle(new MySliderStyle(m_seek_slider->style()));
    m_seek_slider->setFocusPolicy(Qt::NoFocus);

    m_sliderlabel = new QLabel;
    m_sliderlabel->setObjectName(QStringLiteral("QMPWidget_sksllab"));
    m_sliderlabel->setParent(this);
    m_sliderlabel->setAutoFillBackground(true);
    {
        QPalette p = m_sliderlabel->palette();
        p.setColor(QPalette::Window, Qt::white);
        m_sliderlabel->setPalette(p);
    }
    m_sliderlabel->setMouseTracking(true);
    m_sliderlabel->setTextInteractionFlags(Qt::NoTextInteraction);
    m_sliderlabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    m_sliderlabel->setFocusPolicy(Qt::NoFocus);
    m_sliderlabel->setScaledContents(true);
    m_sliderlabel->setWordWrap(false);
    m_sliderlabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_sliderlabel->installEventFilter(this);

    m_hide_slider_timer.setObjectName(QStringLiteral("QMPWidget_hd_sli_tim"));
    m_hide_slider_timer.setParent(this);
    m_hide_slider_timer.setSingleShot(true);
    m_hide_slider_timer.setInterval(hide_slider_timeout_ms);
    XCONNECT(&m_hide_slider_timer, SIGNAL(timeout()), this, SLOT(slot_hidemouse()), QUEUEDCONN);
    slot_hidemouse();

    // Note: these really want to be square. If not-square is required we'd have to be more fancy
    m_icon_tb_cycle_alang = new QIcon(QStringLiteral(":/application/Mouth.svg"));
    m_icon_tb_cycle_slang = new QIcon(QStringLiteral(":/application/Subtitles.svg"));
    m_icon_tb_playpause_dopause = new QIcon(QStringLiteral(":/application/High-contrast-media-playback-pause.svg"));
    m_icon_tb_playpause_doplay = new QIcon(QStringLiteral(":/application/High-contrast-media-playback-start.svg"));
    //m_icon_tb_playpause_inactive = new QIcon(QStringLiteral(":/"));
    m_icon_tb_playpause_inactive = new QIcon();

    m_tb_playpause = make_button("QMPWidget_tb_pp", this, *m_icon_tb_playpause_inactive);
    XCONNECT(m_tb_playpause, SIGNAL(clicked()), this, SLOT(slot_tb_playpause_clicked()), QUEUEDCONN);

    m_tb_cycle_alang = make_button("QMPWidget_tb_ca", this, *m_icon_tb_cycle_alang);
    XCONNECT(m_tb_cycle_alang, SIGNAL(clicked()), this, SLOT(slot_tb_cycle_alang_clicked()), QUEUEDCONN);

    m_tb_cycle_slang = make_button("QMPWidget_tb_cs", this, *m_icon_tb_cycle_slang);
    XCONNECT(m_tb_cycle_slang, SIGNAL(clicked()), this, SLOT(slot_tb_cycle_slang_clicked()), QUEUEDCONN);

    m_hourglass = new QLabel;
    m_hourglass->setObjectName(QStringLiteral("QMPWidget_hougls"));
    m_hourglass->setParent(this);
    m_hourglass->move(0, 0);
    m_hourglass->setAutoFillBackground(true);
    m_hourglass->setTextInteractionFlags(Qt::NoTextInteraction);
    m_hourglass->setFocusPolicy(Qt::NoFocus);
    m_hourglass->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_hourglass->setScaledContents(false);

    m_hourglass_render = new QSvgRenderer(QStringLiteral(":/application/Hourglass_2.svg"));
    m_hourglass_render->setObjectName(QStringLiteral("QMPWidget_houglasrndr"));
    m_hourglass_render->setParent(this);

    if(!m_hourglass_render->isValid()) {
        PROGRAMMERERROR("could not load hourglass SVG");
    }

    slot_updateWidgetSize();

    const QString syntaxhelp = QStringLiteral(
                                   "<table width=\"100%\">"
                                   "<tr><td>P or Space</td><td>Play / Pause</td></tr>"
                                   "<tr><td>Q or Esc</td><td>Stop</td></tr>"
                                   "<tr><td>Left</td><td>10 seconds back</td></tr>"
                                   "<tr><td>Shift-Left</td><td>1 minute back</td></tr>"
                                   "<tr><td>Ctrl-Left</td><td>10 minutes back</td></tr>"
                                   "<tr><td>Right</td><td>10 seconds forward</td></tr>"
                                   "<tr><td>Shift-Right</td><td>1 minute forward</td></tr>"
                                   "<tr><td>Ctrl-Right</td><td>10 minutes forward</td></tr>"
                                   "<tr><td>Up / Down</td><td>Faster / Slower</td></tr>"
                                   "<tr><td>Home</td><td>Back to start</td></tr>"
                                   "<tr><td>J or K</td><td>cycle subtitle track</td></tr>"
                                   "<tr><td>#</td><td>cycle audio track</td></tr>"
                                   "<tr><td>Ctrl-D</td><td>Kill mplayer for testing</td></tr>"
                                   "<tr><td>F7</td><td>record bad moviefile (if hooked up)</td></tr>"
                                   "</table>"
                               );

    // parent of ui object, parent of body and quit button
    m_helpscreen = new OverlayQuit(this, "OQ_mpwhelp", syntaxhelp);

}

void MpWidget::slot_hidemouse()
{
    MYDBG("slot_hidemouse");

    if(m_seek_slider->isSliderDown()) {
        MYDBG("seek slider is down, calling slot_unhidemouse");
        slot_unhidemouse();
        return;
    }

    if(m_seek_slider->isVisible()) {
        MYDBG("seek slider is visible, hidemouse etc");
        setCursor(Qt::BlankCursor);
        m_seek_slider->hide();
        m_sliderlabel->hide();
        m_tb_playpause->hide();
        m_tb_cycle_alang->hide();
        m_tb_cycle_slang->hide();
    }
}
void MpWidget::slot_unhidemouse()
{
    MYDBG("slot_unhidemouse");

    if(!m_seek_slider->isVisible()) {
        MYDBG("seek slider is not visible, showmouse etc");
        setCursor(Qt::ArrowCursor);
        m_seek_slider->show();
        m_sliderlabel->show();
        m_tb_playpause->show();
        m_tb_cycle_alang->show();
        m_tb_cycle_slang->show();
    }

    // will stop it first if already active
    m_hide_slider_timer.start();

}

bool MpWidget::event(QEvent *event)
{
    log_qevent(category(), this, event);

    return super::event(event);
}

bool MpWidget::eventFilter(QObject *watched, QEvent *event)
{
    log_qeventFilter(category(), watched, event);

    if((watched == m_seek_slider || watched == m_sliderlabel  || watched == m_tb_playpause || watched ==  m_tb_cycle_alang || watched ==  m_tb_cycle_slang) && event->type() == QEvent::MouseMove) {
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        mouseMoveEvent(me);
    }

    return super::eventFilter(watched, event);
}

void MpWidget::mouseMoveEvent(QMouseEvent *)
{
    if(!this->isVisible()) {
        return;
    }

    slot_unhidemouse();

}


/*!
 * \brief Destructor
 * \details
 * This function will ask the MPlayer process to quit and block until it has really
 * finished.
 */
MpWidget::~MpWidget()
{
    MYDBG("MpWidget::~MpWidget");
    m_stay_dead = true;


    if(m_process != NULL) {
        MYDBG("deleting m_process");
        MpProcess *copy = m_process;
        m_process = NULL;

        if(copy->processState() == QProcess::Running) {
            copy->disconnect(this);
            copy->quit();
        }

        delete copy;
    }
    else {
        MYDBG("m_process is NULL");
    }

    delete m_background;
    m_background = NULL;

    delete m_widget;
    m_widget = NULL;
}

void MpWidget::slot_error_received_at(const QString &s, double lastpos)
{
    if(m_stay_dead) {
        MYDBG("slot_error_received_at(%s, %f), and m_stay_dead", qPrintable(s), lastpos);
        return;
    }

    MYDBG("slot_error_received_at(%s, %f)", qPrintable(s), lastpos);

    if(lastpos < 0) {
        lastpos = 0.;
    }

    QString url = m_currently_playing;
    MpMediaInfo mmi = m_mediaInfo;

    qWarning("restarting process after error \"%s\" with %s at %f", qPrintable(s), qPrintable(url), lastpos);

    init_process();
    start_process();

    MYDBG("EMIT sig_error_while(%s, %s, %f)", qPrintable(s), qPrintable(url), lastpos);
    emit sig_error_while(s, url, mmi, lastpos);
}

void MpWidget::slot_load_is_done()
{
    MYDBG("slot_load_is_done");

    //if(m_fullscreen) {
    //    slot_submit_write_latin1("set_property fullscreen 1");
    //}

    if(m_mediaInfo.is_interlaced() == Interlaced_t::Progressive) {
        set_deinterlace(false);
    }
    else {
        set_deinterlace(true);
    }

    foreach(const QString &al, m_preferred_alangs) {
        if(try_alang(al)) {
            break;
        }

        MYDBG("no %s audio track found", qPrintable(al));
    }

    foreach(const QString &sl, m_preferred_slangs) {
        if(try_slang(sl)) {
            break;
        }

        MYDBG("no %s subtitle track found", qPrintable(sl));
    }

    //print_all_info();

    if(m_startpos > 0.) {
        MYDBG("xinvokeMethod m_process.slot_abs_seek_from_keyboard(%s)", m_startpos);
        xinvokeMethod(m_process, "slot_abs_seek_from_keyboard", QUEUEDCONN, Q_ARG(double, m_startpos));
    }

    m_startpos = 0.;
}
void MpWidget::set_preferred_alangs(const QStringList &als)
{
    m_preferred_alangs = als;
}
void MpWidget::set_preferred_slangs(const QStringList &sls)
{
    m_preferred_slangs = sls;
}
void MpWidget::set_forbidden_alangs(const QSet<QString> &in_forbidden)
{
    m_forbidden_alangs = in_forbidden;
}


/*!
 * \brief Returns the current MPlayer process state
 *
 * \returns The process state
 */
MpState MpWidget::state() const
{
    return m_process->state();
}

/*!
 * \brief Returns the current media info object
 * \details
 * Please check QMPwidget::MediaInfo::ok to make sure the media
 * information has been fully parsed.
 *
 * \returns The media info object
 */
const MpMediaInfo &MpWidget::mediaInfo() const
{
    return m_mediaInfo;
}

/*!
 * \brief Returns the current playback position
 *
 * \returns The current playback position in seconds
 * \sa seek()
 */
double MpWidget::current_position() const
{
    return m_process->currentExpectedPosition(true);
}

/*!
 * \brief Returns the last playback position
 *
 * \returns The last read playback position in seconds
 * \sa seek()
 */
double MpWidget::last_position() const
{
    return m_process->lastPositionRead();
}

/*!
 * \brief Sets the video output mode
 * \details
 * The video output mode string will be passed to MPlayer using its \p -vo option.
 * Please see http://www.mplayerhq.hu/DOCS/HTML/en/video.html for an overview of
 * available video output modes.
 *
 * \param output The video output mode string
 * \sa videoOutput()
 */
void MpWidget::setVideoOutput(const QString &output)
{
    m_process->setVideoOutput(output);
    m_videooutput = output;
}

/*!
 * \brief Returns the current video output mode
 *
 * \returns The current video output mode
 * \sa setVideoOutput()
 */
const QString &MpWidget::videoOutput() const
{
    return m_process->videoOutput();
}

void MpWidget::setMplayerArgs(const QStringList &in)
{
    m_args = in;
}

const QStringList MpWidget::mplayerArgs()const
{
    return m_args;
}


/*!
 * \brief Sets the path to the MPlayer executable
 * \details
 * Per default, it is assumed the MPlayer executable is
 * available in the current OS path. Therefore, this value is
 * set to "mplayer".
 *
 * \param path Path to the MPlayer executable
 * \sa mplayerPath()
 */
void MpWidget::setMPlayerPath(const QString &path)
{
    m_process->setMplayerPath(path);
    m_mplayerpath = path;
}

/*!
 * \brief Returns the current path to the MPlayer executable
 *
 * \returns The path to the MPlayer executable
 * \sa setMPlayerPath()
 */
const QString &MpWidget::mplayerPath() const
{
    return m_process->mplayerPath();
}

const QStringList &MpWidget::processed_mplayer_args() const
{
    return m_process->mplayer_args();
}

const QList<MpProcess::MpProcessIolog> &MpWidget::mplayer_out() const
{
    return m_process->accumulated_output();
}

QString MpWidget::sliderlabelstring(double pos) const
{
    QString spos = tlenpos_2_human(pos);
    const double len = m_mediaInfo.has_length() ? m_mediaInfo.length() : 100.00;
    QString slen = tlenpos_2_human(len);
    QString label = spos + QLatin1String(" / ") + slen;
    return label;
}

void MpWidget::slot_seekslidermoved(int val)
{
    MYDBG("slot_seekslidermoved(%d)", val);
    m_sliderlabel->setText(sliderlabelstring(val));
}

void MpWidget::slot_mpSeekedTo(double val)
{
    MYDBG("slot_mpSeekedTo(%f)", val);
    adjust_slider_to(val);
    slot_unhidemouse();
}

void MpWidget::connect_seekslider(bool doconnect)
{
    if(doconnect) {
        XCONNECT(m_seek_slider, SIGNAL(valueChanged(int)), m_process, SLOT(slot_seek_from_slider(int)), QUEUEDCONN);
        XCONNECT(m_seek_slider, SIGNAL(sliderMoved(int)), this, SLOT(slot_seekslidermoved(int)), QUEUEDCONN);
    }
    else {
        m_seek_slider->disconnect();
        m_seek_slider->disconnect(this);
        disconnect(m_seek_slider);
    }
}

/*!
 * \brief Returns a suitable size hint for this widget
 * \details
 * This function is used internally by Qt.
 */
QSize MpWidget::sizeHint() const
{
    if(m_mediaInfo.has_size()) {
        return m_mediaInfo.size();
    }

    return super::sizeHint();
}

/*!
 * \brief Starts the MPlayer process with the given arguments
 * \details
 * If there's another process running, it will be terminated first. MPlayer
 * will be run in idle mode and is avaiting your commands, e.g. via load().
 *
 * \param args MPlayer command line arguments
 */
void MpWidget::start_process()
{
    if(m_process->processState() == QProcess::Running) {
        MYDBG("start() called but previously running, invoking quit()");
        m_process->quit();
    }

    int xwinid = m_widget->winId();

    QStringList args = m_args;

    if(!m_preferred_alangs.isEmpty() && m_preferred_alangs.first() != QLatin1String("FIRST")) {
        args += QStringLiteral("-alang");
        args += m_preferred_alangs.join(QStringLiteral(","));
    }

    if(!m_preferred_slangs.isEmpty() && m_preferred_slangs.first() != QLatin1String("FIRST")) {
        args += QStringLiteral("-slang");
        args += m_preferred_slangs.join(QStringLiteral(","));
    }

    m_process->start_process(xwinid, args);
}

/*!
 * \brief Loads a file or url and starts playback
 *
 * \param url File patho or url
 */
void MpWidget::load(const QString &url, const MpMediaInfo &mmi, const double startpos)
{
    m_widget->hide();
    m_hourglass->show();
    m_hourglass->raise();

    MYDBG("showing hourglass ");
    // force a redraw of the screen including the hourglass
    QCoreApplication::processEvents();

    m_mediaInfo.clear();
    m_mediaInfo.assign(mmi);
    m_mediaInfo.make_unfinalized();

    connect_seekslider(false);
    m_seek_slider->setEnabled(false);
    m_seek_slider->setValue(0);

    if(mmi.has_length()) {
        m_seek_slider->setRange(0, int(mmi.length()));
    }

    connect_seekslider(true);

    m_startpos = startpos;

    m_currently_playing = url;

    MYDBG("xinvokeMethod m_process.slot_load(%s)", qPrintable(url));
    xinvokeMethod(m_process, "slot_load", QUEUEDCONN, Q_ARG(QString, url));
}

/*!
 * \brief Sends a command to the MPlayer process
 * \details
 * Since MPlayer is being run in slave mode, it reads commands from the standard
 * input. It is assumed that the interface provided by this class might not be
 * sufficient for some situations, so you can use this functions to directly
 * control the MPlayer process.
 *
 * For a complete list of commands for MPlayer's slave mode, see
 * http://www.mplayerhq.hu/DOCS/tech/slave.txt .
 *
 * \param command The command line. A newline character will be added internally.
 */
void MpWidget::submit_write(const QString &command)
{
    m_process->slot_submit_write(command);
}
void MpWidget::submit_write_latin1(char const *const latin1lit)
{
    submit_write(QString::fromLatin1(latin1lit));
}

void MpWidget::testKillMplayer()
{
    if(m_process == NULL) {
        MYDBG("testKillMplayer called but currently no MpProcess object");
        return;
    }

    Q_PID mppid = m_process->pid();

    if(mppid < 1) {
        MYDBG("testKillMplayer called but the MpProcess object does not have a unix process under it currently");
        return;
    }

    if(0 != kill(mppid, SIGSEGV)) {
        MYDBG("testKillMplayer: kill(%d, SIGSEGV) failed: %s", int(mppid), strerror(errno));
    }
    else {
        MYDBG("testKillMplayer: kill(%d, SIGSEGV) successfull", int(mppid));
    }
}

void MpWidget::slot_show_helpscreen()
{
    // parent of ui object, parent of body and quit button
    MYDBG("slot_show_helpscreen");
    m_helpscreen->show_and_take_focus(this);
    XCONNECT(m_helpscreen, SIGNAL(sig_stopped()), this, SLOT(slot_set_not_showing_help()), QUEUEDCONN);
    // switch on mouse cursor?
}

void MpWidget::slot_set_not_showing_help()
{
    MYDBG("slot_set_not_showing_help - help screen is going away");
    m_helpscreen->hide();
}

void MpWidget::toggle_playpause()
{
    if(state() == MpState::PlayingState) {
        MYDBG("INVOKE_DELAY_MP slot_pause");
        INVOKE_DELAY_MP(slot_pause());
    }
    else if(state() == MpState::PausedState) {
        MYDBG("INVOKE_DELAY_MP slot_play");
        INVOKE_DELAY_MP(slot_play());
    }
    else {
        MYDBG("toggle_playpause: not playing or paused, ignoring");
    }
}

void MpWidget::cycle_alang()
{
    if(m_forbidden_alangs.isEmpty()) {
        const QString cmd = QStringLiteral("switch_audio");
        MYDBG("xinvokeMethod m_process.slot_submit_write(%s)", qPrintable(cmd));
        xinvokeMethod(m_process, "slot_submit_write", QUEUEDCONN, Q_ARG(const QString &, cmd));
        (void)m_process->set_assume_next_aid();
    }
    else {
        int next_aid = m_process->find_next_alang_not_in(m_forbidden_alangs);

        if(next_aid >= -10) {
            QString cmd = QString(QStringLiteral("switch_audio %1")).arg(next_aid);
            MYDBG("xinvokeMethod m_process.slot_submit_write(%s)", qPrintable(cmd));
            xinvokeMethod(m_process, "slot_submit_write", QUEUEDCONN, Q_ARG(const QString &, cmd));
            m_process->set_assume_aid(next_aid);
        }
    }

    const QString cmd2 = QStringLiteral("get_property switch_audio");
    MYDBG("xinvokeMethod m_process.slot_submit_write(%s)", qPrintable(cmd2));
    xinvokeMethod(m_process, "slot_submit_write", QUEUEDCONN, Q_ARG(const QString &, cmd2));
}
void MpWidget::cycle_slang()
{
    const QString cmd = QStringLiteral("sub_select");
    MYDBG("xinvokeMethod m_process.slot_submit_write(%s)", qPrintable(cmd));
    xinvokeMethod(m_process, "slot_submit_write", QUEUEDCONN, Q_ARG(const QString &, cmd));
}

/*!
 * \brief Keyboard press event handler
 * \details
 * This implementation tries to resemble the classic MPlayer interface. For a
 * full list of supported key codes, see \ref shortcuts.
 *
 * \param event Key event
 */
void MpWidget::keyPressEvent(QKeyEvent *event)
{
    log_qevent(category(), this, event);

    bool accept = true;

    switch(event->key()) {

        case Qt::Key_F1: {
            if(event->modifiers() == Qt::NoModifier) {
                MYDBG("   = F1");

                if(state() == MpState::PlayingState) {
                    MYDBG("INVOKE_DELAY_MP slot_pause");
                    INVOKE_DELAY_MP(slot_pause());
                }

                MYDBG("scheduling slot_show_helpscreen in %d msec", 1000);
                QTimer::singleShot(1000, this, SLOT(slot_show_helpscreen()));

                return;
            }
        }
        break;

        case Qt::Key_P:
        case Qt::Key_Space: {
            MYDBG("got P or SPACE, toggline play/pause");
            toggle_playpause();
        }

        break;

        case Qt::Key_Q:
        case Qt::Key_Escape: {
            MYDBG("got keyboard Q / Esc, INVOKE_DELAY_MP slot_stop");
            INVOKE_DELAY_MP(slot_stop());
        }
        break;

        case Qt::Key_Left: {

            if(event->modifiers() == Qt::NoModifier) {
                MYDBG("xinvokeMethod m_process.slot_rel_seek_from_keyboard(%f)", -seek_distance_1_sec - 5);
                xinvokeMethod(m_process, "slot_rel_seek_from_keyboard", QUEUEDCONN, Q_ARG(double, -seek_distance_1_sec - 5));
            }
            else if(event->modifiers() == Qt::ShiftModifier) {
                MYDBG("xinvokeMethod m_process.slot_rel_seek_from_keyboard(%f)", -seek_distance_2_sec - 5);
                xinvokeMethod(m_process, "slot_rel_seek_from_keyboard", QUEUEDCONN, Q_ARG(double, -seek_distance_2_sec - 5));
            }
            else if(event->modifiers() == Qt::ControlModifier) {
                MYDBG("xinvokeMethod m_process.slot_rel_seek_from_keyboard(%f)", -seek_distance_3_sec - 5);
                xinvokeMethod(m_process, "slot_rel_seek_from_keyboard", QUEUEDCONN, Q_ARG(double, -seek_distance_3_sec - 5));
            }
        }

        break;

        case Qt::Key_Right: {

            if(event->modifiers() == Qt::NoModifier) {
                MYDBG("xinvokeMethod m_process.slot_rel_seek_from_keyboard(%f)", seek_distance_1_sec);
                xinvokeMethod(m_process, "slot_rel_seek_from_keyboard", QUEUEDCONN, Q_ARG(double, seek_distance_1_sec));
            }
            else if(event->modifiers() == Qt::ShiftModifier) {
                MYDBG("xinvokeMethod m_process.slot_rel_seek_from_keyboard(%f)", seek_distance_2_sec);
                xinvokeMethod(m_process, "slot_rel_seek_from_keyboard", QUEUEDCONN, Q_ARG(double, seek_distance_2_sec));
            }
            else if(event->modifiers() == Qt::ControlModifier) {
                MYDBG("xinvokeMethod m_process.slot_rel_seek_from_keyboard(%f)", seek_distance_3_sec);
                xinvokeMethod(m_process, "slot_rel_seek_from_keyboard", QUEUEDCONN, Q_ARG(double, seek_distance_3_sec));
            }

        }

        break;

        case Qt::Key_Up: {
            if(event->modifiers() == Qt::NoModifier) {
                MYDBG("INVOKE_DELAY_MP slot_speed_up");
                INVOKE_DELAY_MP(slot_speed_up());
            }
        }

        break;

        case Qt::Key_Down: {
            if(event->modifiers() == Qt::NoModifier) {
                MYDBG("INVOKE_DELAY_MP slot_slow_down");
                INVOKE_DELAY_MP(slot_slow_down());
            }
        }

        break;

        case Qt::Key_Home: {
            MYDBG("xinvokeMethod m_process.slot_abs_seek_from_keyboard(0)");
            xinvokeMethod(m_process, "slot_abs_seek_from_keyboard", QUEUEDCONN, Q_ARG(double, 00));
        }
        break;

        case Qt::Key_J:
        case Qt::Key_K: {
            MYDBG("got j or k, cycling subtitle");
            cycle_slang();
        }
        break;

        case Qt::Key_NumberSign: {
            MYDBG("got #, cycling audio");
            cycle_alang();
        }
        break;

        case Qt::Key_D: {
            if(event->modifiers() == Qt::ControlModifier) {
                MYDBG("Ctrl-D");
                testKillMplayer();
            }
            else {
                MYDBG("D");
            }
        }
        break;

        case Qt::Key_F7: {
            if(event->modifiers() == Qt::NoModifier) {
                MYDBG("   = F7");
                MYDBG("   EMIT sig_recordbad()");
                BadMovieRecord_t info;
                info[QStringLiteral("abs_movfn")] = m_currently_playing;
                emit sig_recordbad(info);
                MYDBG("   INVOKE_DELAY_MP slot_stop");
                INVOKE_DELAY_MP(slot_stop());
                return;
            }
        }
        break;

#if 0

        case Qt::Key_Asterisk: {
            setVolume_rel(10);
        }
        break;

        case Qt::Key_Slash: {
            setVolume_rel(-10);
        }
        break;
#endif

        default:
            accept = false;
            break;
    }

    event->setAccepted(accept);
    MYDBG("INVOKE_DELAY slot_unhidemouse");
    INVOKE_DELAY(slot_unhidemouse());
}

void MpWidget::slot_tb_playpause_clicked()
{
    MYDBG("slot_tb_playpause_clicked()");
    toggle_playpause();
    MYDBG("INVOKE_DELAY slot_unhidemouse");
    INVOKE_DELAY(slot_unhidemouse());
}
void MpWidget::slot_tb_cycle_alang_clicked()
{
    MYDBG("slot_tb_cycle_alang_clicked()");
    cycle_alang();
    MYDBG("INVOKE_DELAY slot_unhidemouse");
    INVOKE_DELAY(slot_unhidemouse());
}
void MpWidget::slot_tb_cycle_slang_clicked()
{
    MYDBG("slot_tb_cycle_slang_clicked()");
    cycle_slang();
    MYDBG("INVOKE_DELAY slot_unhidemouse");
    INVOKE_DELAY(slot_unhidemouse());
}

/*!
 * \brief Resize event handler
 * \details
 * If you reimplement this function, you need to call this handler, too.
 *
 * \param event Resize event
 */
void MpWidget::resizeEvent(QResizeEvent *event)
{
    log_qevent(category(), this, event);
    slot_updateWidgetSize();
}

void MpWidget::set_crop(const QString &cs)
{
    MYDBG("applying crop \"%s\" to player widget", qPrintable(cs));
    m_mediaInfo.set_crop(cs);
    slot_updateWidgetSize();
}

QRect MpWidget::compute_widget_new_geom() const
{
    if(!m_mediaInfo.has_size() || !isVisible()) {
        CWNGMYDBG("cwng: no size");
        QRect ret(QPoint(0, 0), size());
        return ret;
    }

    {

        QSize mediaSize = m_mediaInfo.displaySize();
        CWNGMYDBG("mediainfo displaysize %dx%d (PAR=%f)", mediaSize.width(), mediaSize.height(), m_mediaInfo.PAR());
        QSize widgetSize = size();

        mediaSize.scale(widgetSize, Qt::KeepAspectRatio);
        QRect wrect(0, 0, mediaSize.width(), mediaSize.height());
        wrect.moveTopLeft(rect().center() - wrect.center());
        CWNGMYDBG("m_widget new geometry without cropping: wxh %dx%d xxy %dx%d", wrect.width(), wrect.height(), wrect.x(), wrect.y());

    }

    const int movw = m_mediaInfo.width();
    const int movh = m_mediaInfo.height();
    CWNGMYDBG("movie wxh %ux%u", movw, movh);
    // the crop region display size
    QSize croppeddisplaysize(m_mediaInfo.getCroppedWidth(), m_mediaInfo.getCroppedHeight());
    croppeddisplaysize.setWidth(double(croppeddisplaysize.width())*m_mediaInfo.PAR());
    CWNGMYDBG("croppeddisplaysize wxh %ux%u", croppeddisplaysize.width(), croppeddisplaysize.height());

    QSize widgetSize = size();
    // this is the new size of the crop rectangle
    QSize scaledcroppeddisplaysize = croppeddisplaysize;
    scaledcroppeddisplaysize.scale(widgetSize, Qt::KeepAspectRatio);
    CWNGMYDBG("croppeddisplaysize scaled wxh %ux%u", scaledcroppeddisplaysize.width(), scaledcroppeddisplaysize.height());
    // we scaled the crop region up this much:
    const double scalew = (double)scaledcroppeddisplaysize.width() / (double)croppeddisplaysize.width();
    const double scaleh = (double)scaledcroppeddisplaysize.height() / (double)croppeddisplaysize.height();
    CWNGMYDBG("scale factors %g x %g", scalew, scaleh);

    const int dispmovw = double(movw) * m_mediaInfo.PAR();

    // so the whole movie needs to be sized up this much:
    QSize newwsize(double(dispmovw) * scalew, double(movh) * scaleh);

    CWNGMYDBG("movie upscaled wxh %ux%u", newwsize.width(), newwsize.height());

    int left = (-1.) * m_mediaInfo.getCropLeft() * scalew;
    int top = (-1.) * m_mediaInfo.getCropTop() * scaleh;

    // is this correct?
    int dleft = (widgetSize.width() - scaledcroppeddisplaysize.width()) / 2;
    int dtop = (widgetSize.height() - scaledcroppeddisplaysize.height()) / 2;
    CWNGMYDBG("moving viewport %dx%d and additional %dx%d", left, top, dleft, dtop);
    left += dleft;
    top += dtop;

    QRect wrect(left, top, newwsize.width(), newwsize.height());

    return wrect;
}

void MpWidget::slot_updateWidgetSize()
{
    MYDBG("slot_updateWidgetSize");

    const int sliderlw = this->width() / 5;
    const int sliderh = this->height() / 20;

    const int tb_h = sliderh;
    const int tb_w = sliderh;

    m_tb_playpause  ->setIconSize(QSize(tb_w - 2, tb_h - 2));
    m_tb_cycle_alang->setIconSize(QSize(tb_w - 2, tb_h - 2));
    m_tb_cycle_slang->setIconSize(QSize(tb_w - 2, tb_h - 2));

    // x y w h
    const int toolbar_y = this->height() - sliderh;
    m_sliderlabel   ->setGeometry(0                          , toolbar_y, sliderlw                              , sliderh);
    m_seek_slider   ->setGeometry(sliderlw                   , toolbar_y, this->width() - sliderlw - 3 * tb_w   , sliderh);

    m_tb_playpause  ->setGeometry(this->width() - 3 * tb_w   , toolbar_y, tb_w                                  , sliderh);
    m_tb_cycle_alang->setGeometry(this->width() - 2 * tb_w   , toolbar_y, tb_w                                  , sliderh);
    m_tb_cycle_slang->setGeometry(this->width() - 1 * tb_w   , toolbar_y, tb_w                                  , sliderh);

    QFont slfont = m_sliderlabel->font();
    const int newpixelsize = qMax(8, this->height() / 35);
    slfont.setPixelSize(newpixelsize);
    m_sliderlabel->setFont(slfont);

    CWNGMYDBG("QMPwidget new geometry wxh %dx%d xxy %dx%d", geometry().width(), geometry().height(), geometry().x(), geometry().y());

    if(m_background->geometry() != geometry()) {
        m_background->setGeometry(geometry());
    }

    // in: m_mediaInfo.size size() cropsize m_mediaInfo.PAR
    // out: m_widget->size

    QRect wrect = compute_widget_new_geom();

    if(m_widget->geometry() != wrect) {
        m_widget->setGeometry(wrect);

        CWNGMYDBG("m_widget new geometry wxh %dx%d xxy %dx%d", m_widget->geometry().width(), m_widget->geometry().height(), m_widget->geometry().x(), m_widget->geometry().y());

        if(m_mediaInfo.has_size()) {
            CWNGMYDBG("should crop top %u bottom %u left %u right %u", unsigned(m_mediaInfo.getCropTop()), unsigned(m_mediaInfo.getCropBottom()), unsigned(m_mediaInfo.getCropLeft()), unsigned(m_mediaInfo.getCropRight()));
        }
    }

    if(m_hourglass->pixmap() == NULL || m_hourglass->geometry() != geometry() || m_hourglass->geometry().isEmpty()) {

        MYDBG("populating hourglass %dx%d", this->width(), this->height());
        m_hourglass->setGeometry(geometry());

        QFont captionfont = m_hourglass->font();
        captionfont.setBold(true);
        const int newpixelsize = qMax(8, this->height() / 20);
        captionfont.setPixelSize(newpixelsize);
        m_hourglass->setFont(captionfont);
        m_hourglass->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

        QSize svgsize = m_hourglass_render->defaultSize();
        QSize imgsize = svgsize;
        imgsize.scale(m_hourglass->size(), Qt::KeepAspectRatio);
        QImage hour_i(imgsize, QImage::Format_RGB32);
        const QPalette palette = QApplication::palette();
        const QColor &backgroundcolor = palette.color(QPalette::Window);
        hour_i.fill(backgroundcolor.rgb());

        QPainter painter(&hour_i);
        m_hourglass_render->render(&painter);

        QPixmap hour_pix = QPixmap::fromImage(hour_i);

        if(hour_pix.isNull()) {
            qWarning("got null pixmap from hourglass image");
        }
        else {

            m_hourglass->setPixmap(hour_pix);
        }
    }

}


void MpWidget::setVolume_abs(int volume)
{
    const QString cmd = QString(QStringLiteral("volume %1 1")).arg(volume);
    submit_write(cmd);
}

void MpWidget::setVolume_rel(int rvolume)
{
    const QString cmd = QString(QStringLiteral("volume %1")).arg(rvolume);
    MYDBG("xinvokeMethod m_process.slot_submit_write(%s)", qPrintable(cmd));
    xinvokeMethod(m_process, "slot_submit_write", QUEUEDCONN, Q_ARG(const QString &, cmd));
}

static_var const char *mpproperties_latin1lit[] = {
    "fullscreen",
    "width",
    "height",
    "hue",
    "saturation",
    "length",
    "loop",
    "mute",
    "ontop",
    "osdlevel",
    "panscan",
    "path",
    "pause",
    "percent_pos",
    "rootwin",
    "samplerate",
    "speed",
    "stream_end",
    "stream_length",
    "stream_pos",
    "stream_start",
    "sub",
    "sub_delay",
    "sub_demux",
    "sub_source",
    "sub_visibility",
    "switch_audio",
    "switch_video",
    "time_pos",
    "titles",
    "video_bitrate",
    "video_codec",
    "video_format",
    "volume",
    "vsync",
    NULL
};

void MpWidget::print_all_info()
{
    for(int i = 0; i < 1000; i++) {
        char const *const prop_latin1lit = mpproperties_latin1lit[i];

        if(prop_latin1lit == NULL || prop_latin1lit[0] == '\0') {
            break;
        }

        QString cmd = QStringLiteral("pausing_keep_force get_property %1");
        cmd = cmd.arg(QLatin1String(prop_latin1lit));
        submit_write(cmd);
    }

    submit_write_latin1("pausing_keep_force get_vo_fullscreen");
}

bool MpWidget::try_alang(const QString &alang)
{
    int aid;

    if(alang == QLatin1String("FIRST")) {
        aid = 0;
    }
    else {
        aid = m_mediaInfo.alang_2_aid(alang);
    }

    if(aid < 0) {
        return false;
    }

    MYDBG("atrack %d is in %s", aid, qPrintable(alang));
    QString cmd = QString(QStringLiteral("switch_audio %1")).arg(aid);
    submit_write(cmd);
    m_process->set_assume_aid(aid);
    submit_write_latin1("get_property switch_audio");
    return true;
}

bool MpWidget::try_slang(const QString &slang)
{
    int sid;

    if(slang == QLatin1String("FIRST")) {
        sid = 0;
    }
    else {
        sid = m_mediaInfo.slang_2_sid(slang);
    }

    if(sid < 0) {
        return false;
    }

    MYDBG("strack %d is in %s", sid, qPrintable(slang));
    QString cmd = QString(QStringLiteral("sub_demux %1")).arg(sid);
    submit_write(cmd);
    //slot_submit_write_latin1("get_property sub");
    return true;
}
void MpWidget::set_deinterlace(bool toggle)
{
    if(toggle) {
        submit_write_latin1("set_property deinterlace 1");
    }
    else {
        submit_write_latin1("set_property deinterlace 0");
    }

    //slot_submit_write_latin1("get_property deinterlace");
}

/*
${NAME}       Expand to the value of the property NAME.
?(NAME:TEXT)  Expand TEXT only if the property NAME is available.
?(!NAME:TEXT) Expand TEXT only if the property NAME is not available.
*/
//void MpWidget::slot_osd_show_property_text(const QString &format, size_t duration_ms, size_t level)
//{
//    QString f = QStringLiteral("pausing_keep osd_show_property_text \"%1\" %2 %3");
//    f = f.arg(format).arg(duration_ms).arg(level);
//    slot_submit_write(f);
//}

//void MpWidget::slot_osd_show_text(const QString &str, size_t duration_ms, size_t level)
//{
//    QString f = QStringLiteral("pausing_keep osd_show_text \"%1\" %2 %3");
//    f = f.arg(str).arg(duration_ms).arg(level);
//    slot_submit_write(f);
//}

void MpWidget::slot_mpStateChanged(MpState oldstate, MpState newstate)
{
    if(oldstate == newstate) {
        return;
    }

    MYDBG("slot_mpStateChanged(from %s to %s)", convert_MpState_2_asciidesc(oldstate), convert_MpState_2_asciidesc(newstate));

    if(newstate == MpState::PlayingState) {
        m_tb_playpause->setIcon(*m_icon_tb_playpause_dopause);
    }
    else if(newstate == MpState::PausedState) {
        m_tb_playpause->setIcon(*m_icon_tb_playpause_doplay);
    }
    else {
        m_tb_playpause->setIcon(*m_icon_tb_playpause_inactive);
    }

    if(newstate == MpState::PlayingState) {
        const double len = m_mediaInfo.has_length() ? m_mediaInfo.length() : 100.00;
        MYDBG("mpStateChanged, and changing seekslider to (%f, %s)", len, m_mediaInfo.is_seekable() ? "seekable" : "not seekable");
        connect_seekslider(false);
        m_seek_slider->setRange(0, len);
        m_seek_slider->setEnabled(m_mediaInfo.is_seekable());

        if(m_seek_slider->value() > len) {
            m_seek_slider->setValue(len);
        }

        connect_seekslider(true);
    }
    else {
        m_seek_slider->setEnabled(false);
    }

    if(oldstate == MpState::LoadingState) {
        m_hourglass->hide();
        m_widget->show();
    }

    MYDBG("INVOKE_DELAY slot_updateWidgetSize");
    INVOKE_DELAY(slot_updateWidgetSize());
    MYDBG("EMIT sig_stateChanged(%s, %s)", convert_MpState_2_asciidesc(oldstate), convert_MpState_2_asciidesc(newstate));
    emit sig_stateChanged(oldstate, newstate);
}

void MpWidget::adjust_slider_to(double position)
{
    if(position < 0) {
        position = 0;
    }

    if(qAbs(m_seek_slider->value() - position) > 10) {
        MYDBG("got mpStreamPositionChanged(%f) and seekslider is at %d", position, m_seek_slider->value());
        connect_seekslider(false);
        m_seek_slider->setValue(qRound(position));
        connect_seekslider(true);
    }

    m_sliderlabel->setText(sliderlabelstring(position));

}

void MpWidget::slot_mpStreamPositionChanged(double position)
{
    if(!m_seek_slider->isSliderDown()) {
        MYDBG("slot_mpStreamPositionChanged(%f) and slider is not down", position);
        adjust_slider_to(position);
    }
}


void MpWidget::show_and_take_focus(QWidget *oldfocusguess)
{
    MYDBG("show_and_take_focus");
    m_focusstack.show_and_take_focus(NULL, oldfocusguess);
    slot_updateWidgetSize();

}
void MpWidget::hide_and_return_focus()
{
    MYDBG("hide_and_return_focus");
    m_focusstack.hide_and_return_focus();
}
