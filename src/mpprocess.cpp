#include "mpprocess.h"
#include <QDebug>
#include <QWidget>
#include <QFile>
#include <QScopedArrayPointer>
#include <QElapsedTimer>
#include <QCoreApplication>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "mpmediainfo.h"
#include "qprocess_meta.h"
#include "vregexp.h"
#include "asyncreadfile.h"
#include "asynckillproc.h"

//#define DEBUG_MPP
//#define DEBUG_MPP_TIME

#ifdef DEBUG_ALL
#define DEBUG_MPP
#define DEBUG_MPP_TIME
#endif

#ifdef DEBUG_MPP_TIME
#define DEBUG_MPP
#endif

#ifdef DEBUG_MPP
#  ifdef DEBUG_MPP_TIME
#    define MYDBG(msg, ...) qDebug("MMP %s(%f) " msg, qPrintable(now.toLocalTime().toString(QLatin1String("hh:mm:ss.zzz"))), m_lastread_streamPosition, ##__VA_ARGS__)
#    define STATICDBG(msg, ...) qDebug("MMP %s " msg, qPrintable(QDateTime::currentDateTimeUtc().toLocalTime().toString(QLatin1String("hh:mm:ss.zzz"))), ##__VA_ARGS__)
#  else
#    define MYDBG(msg, ...) qDebug("MMP " msg, ##__VA_ARGS__)
#    define STATICDBG(msg, ...) qDebug("MMP " msg, ##__VA_ARGS__)
#  endif
#else
#  define MYDBG(msg, ...)
#  define STATICDBG(msg, ...)
#endif

#ifdef DEBUG_MPP
#define TIMEMYDBG(msg, ...) MYDBG(msg, ##__VA_ARGS__)
#else
#define TIMEMYDBG(msg, ...)
#endif

// fail if we could not load the first few blocks of the file
static const int max_time_for_preloading_file_ms = 6000;
// before attempting to load a file in mplayer, load that many KBytes of it
static const int preloading_file_KBytes = 16 * 1024;
// after a seek, we will get statuslines for a while which do not yet show the new position
static const qint64 ignore_statusline_after_seek_ms = 400;
// after a load, we might get statuslines for a while which do not yet show the new position
static const qint64 ignore_statusline_after_load_ms = 300;
// within that time, if we parse a position that is closer than this to the current
// expected position, assume that those are actually correct
static const double no_not_ignore_parsed_position_if_closer_than_sec = 4.;
// fail if we are not playing that many msec after load file in mplayer
static const int max_time_for_loading_file_ms = 5000;
// call the heartbeat() slot that often
static const int heartbeat_interval_msec = 200;
// do not allow write commands more often than this
// mplayer loop is exactly 200ms, but at each loop it might execute multiple commands
static const qint64 min_time_between_writes_ms = 150;
// allow writes even if we have not seen a read since that many ms
static const qint64 allow_writes_even_if_no_read_ms = 300;
// if the output queue is larger, try to flush it
static const int output_queue_max_size = 16;
// if the output queue is larger, reduce min times
static const int output_queue_large_size = 8;
// fail if we get a position that is that much bigger than the movie length
static const double max_overflow_position_to_length_sec = 1.;
// we expect to read something that many msec after a write
static const qint64 expect_read_after_write_ms = 400;
// emit streamPositionChanged() if we change by that much
static const double emit_position_change_if_change_larger_than_sec = 0.5;
// change by this much for speed up / slow down
static const double speed_multiplier = 2;
// try to resynchronize if the statusline A-V is larger than this
// 0 or smaller for disabling resync
static const double max_avmissync_before_explicit_seek_normal_speed_secs = -1.;
// try to resynchronize if the statusline A-V is larger than this
// 0 or smaller for disabling resync
static const double max_avmissync_before_explicit_seek_unnormal_speed_secs = 2.;
// fail if we read a position smaller than that
static const double minimum_allowed_negative_position_from_statusline = -1.;
// should be larger than 0. ignore close seeks from the slider
static const double ignore_seeks_from_slider_closer_than_sec = 5.;
// if we detect that position force a stop
static const double force_stop_sec_before_end = 1.;
// after submitting a "stop" command, sleep that long
static const int sleep_after_stop_command_msec = 800;
// then, force a buffer flush and possibly wait even longer
static const int waitforreadyread_after_stop_command_msec = 500;
// when trying to get out of mplayer, wait that long after issuing a "quit" command
static const int wait_for_mplayer_quit_command_msec = 1000;
// if the "quit" command did not work, call terminate() and wait that long before calling kill()
static const int wait_after_terminate_call_msec = 300;
// before loading a new file, try to clear the I/O buffers.
// wait for readyRead so long
static const int beforeload_waitForReadyRead_1_msec = 500;
// sleep after that first wait...
static const int beforeload_sleep_msec = 10;
// wait for readyRead again so long
static const int beforeload_waitForReadyRead_2_msec = 500;
// after issuing a seek command, sleep that long
static const int sleep_after_seeking_msec = 100;

static char const *const crap_rxlit =
    "Fontconfig warning"
    "|"
    "\\[matroska,webm \\@ 0x[\\d\\w]+\\]Unknown entry 0x[\\d\\w]+"
    "|"
    "\\[matroska,webm \\@ 0x[\\d\\w]+\\]first_dts 0 "
    "|"
    "VFILTER: Suspicious mp_image usage count"
    "|"
    "BUG in FFmpeg, draw_slice called with NULL pointer"
    "|"
    "VFILTER: scale: query\\(Planar"
    "|"
    "VIDEOOUT: VID_CREATE: \\d+"
    "|"
    "VIDEOOUT: VID CREATE: \\d+"
    "|"
    "VIDEOOUT: DRAW_OSD"
    "|"
    "FLIP_PAGE VID:\\d+"
    "|"
    "VIDEOOUT: $"
    "|"
    "DECAUDIO: \\[.+\\]DTS-ExSS: unknown marker = "
    "|"
    "ASS: \\[ass\\] shifting from \\d+ to \\d"
    "|"
    "ASS: \\[ass\\] forced line break at \\d+"
    ;
static char const *const statusline_rxlit =
    "A:"
    "|"
    "V:"
    ;
// at these states, we expect to get a read every NNN msec
// Warning: needs to be in syn with definition of MpState in .h file
static const qint64 expect_read_every_ms[MpState_maxidx] = {
    -1,  // MpState::NotStartedState
    -1,  // MpState::IdleState
    400, // MpState::LoadingState
    -1,  // MpState::StoppedState
    400, // MpState::PlayingState
    400, // MpState::BufferingState
    -1,  // MpState::PausedState
    -1   // MpState::ErrorState
};

static bool predicate_screensaver_should_be_active(void *data)
{
    MpProcess *proc = (MpProcess *)(data);
    return proc->screensaver_should_be_active();
}

MpProcess::MpProcess(QObject *parent, MpMediaInfo *mip)
    : QObject(parent),
      m_proc(new QProcess(this)),
      m_curr_state(MpState::NotStartedState),
      m_cfg_mplayerPath(QLatin1String("mplayer")),
      m_mediaInfo(mip),
      m_heartbeattimer(this),
      m_current_aid(0),
      m_ssmanager(this, &predicate_screensaver_should_be_active, this)
{
    {
#ifdef DEBUG_MPP_TIME
        QDateTime now = QDateTime::currentDateTimeUtc();
#endif
        MYDBG("MpProcess::MpProcess");
    }

    qRegisterMetaType<MpState>();
    qRegisterMetaType<MpProcess::SeekMode>();

    resetValues();

    setObjectName(QLatin1String("MPProcess"));
    m_cfg_acc_maxlines = 0;
    m_cfg_output_accumulator_ignore = NULL;

    m_stopped_because_of_long_seek = false;

    m_cfg_output_accumulator_mode = Input | Output;

    m_cfg_videoOutput = QLatin1String("gl");

    qRegisterMetaType<QProcess::ExitStatus>();
    qRegisterMetaType<QProcess::ProcessError>();

    XCONNECT(m_proc, SIGNAL(readyReadStandardOutput()), this, SLOT(slot_readStdout()), QUEUEDCONN);
    XCONNECT(m_proc, SIGNAL(readyReadStandardError()), this, SLOT(slot_readStderr()), QUEUEDCONN);
    XCONNECT(m_proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slot_finished(int, QProcess::ExitStatus)), QUEUEDCONN);
    XCONNECT(m_proc, SIGNAL(error(QProcess::ProcessError)), this, SLOT(slot_error_received(QProcess::ProcessError)), QUEUEDCONN);

    m_heartbeattimer.setInterval(heartbeat_interval_msec);
    m_heartbeattimer.setObjectName(QLatin1String("QMPP heartbeat"));
    m_heartbeattimer.setSingleShot(false);
    XCONNECT(&m_heartbeattimer, SIGNAL(timeout()), this, SLOT(slot_heartbeat()), QUEUEDCONN);
    make_heartbeat_active_or_not();
}

void MpProcess::quit()
{
#ifdef DEBUG_MPP_TIME
    {
        QDateTime now = QDateTime::currentDateTimeUtc();
        MYDBG("MpProcess::quit");
    }
#else
    MYDBG("MpProcess::quit");
#endif

    pid_t qpid = m_proc->pid();

    m_heartbeattimer.stop();

    if(m_proc->state() != QProcess::NotRunning) {
        force_cmd(QLatin1String("quit"));
        m_outputq.clear();

        if(!m_proc->waitForFinished(wait_for_mplayer_quit_command_msec) && m_proc->state() != QProcess::NotRunning) {
#ifdef DEBUG_MPP_TIME
            QDateTime now = QDateTime::currentDateTimeUtc();
#endif
            MYDBG("terminate");
            m_proc->terminate();

            if(!m_proc->waitForFinished(wait_after_terminate_call_msec)  && m_proc->state() != QProcess::NotRunning) {
#ifdef DEBUG_MPP_TIME
                now = QDateTime::currentDateTimeUtc();
#endif
                MYDBG("kill");
                m_proc->kill();
            }
        }
    }

    if(qpid > 0) {
        async_kill_process(qpid, "making sure it is dead", qPrintable(QString::number(qpid)));
    }

    clear_out_incremental_stdouterr();
    m_outputq.clear();
    resetValues();
    m_ssmanager.enable();

}

MpProcess::~MpProcess()
{
#ifdef DEBUG_MPP_TIME
    {
        QDateTime now = QDateTime::currentDateTimeUtc();
        MYDBG("MpProcess::~MpProcess");
    }
#else
    MYDBG("MpProcess::~MpProcess");
#endif
    quit();

    if(m_cfg_output_accumulator_ignore != NULL) {
        delete m_cfg_output_accumulator_ignore;
    }

#ifdef DEBUG_MPP_TIME
    QDateTime now = QDateTime::currentDateTimeUtc();
#endif

    for(unsigned idx = 0; idx < MpState_maxidx; idx++) {
        const qint64 maxlat = expect_read_every_ms[idx];
        const qint64 lat = m_max_encountered_readlatency_ms[idx];
        m_max_encountered_readlatency_ms[idx] = (-1);

        if(lat >= 0 && maxlat > 0) {
            MYDBG("%s: max latency encountered %lu ms", convert_MpStateidx_2_asciidesc(idx), (unsigned long)lat);
        }
    }

    if(m_max_encountered_writelatency_ms >= 0) {
        MYDBG("max write latency encountered %lu ms", (unsigned long)m_max_encountered_writelatency_ms);
        m_max_encountered_writelatency_ms = (-1);
    }

    if(m_max_encountered_writequeuelatency_ms > 0) {
        MYDBG("max time commands stayed in write queue: %lu ms", (unsigned long)m_max_encountered_writequeuelatency_ms);
        m_max_encountered_writequeuelatency_ms = (-1);
    }

    resetValues();

    delete m_proc;
    m_proc = NULL;
}

// Starts the MPlayer process in idle mode
void MpProcess::start_process(int xwinid, const QStringList &args)
{
    QStringList myargs;
    myargs += QLatin1String("-slave");
    myargs += QLatin1String("-idle");
    myargs += QLatin1String("-noquiet");
    myargs += QLatin1String("-v");
    myargs += QLatin1String("-identify");
    myargs += QLatin1String("-nomouseinput");
    myargs += QLatin1String("-nokeepaspect");
    myargs += QLatin1String("-nostop-xscreensaver");
    myargs += QLatin1String("-msgmodule");
    myargs += QLatin1String("-msglevel");
    // all=level:module=level:....
    // default level 5
    //myargs += QLatin1String("all=6:demuxer=5:ds=5:demux=5");
    //myargs += QLatin1String("global=7:cplayer=7:avsync=6:statusline=7");
    myargs += QLatin1String("all=9:demux=5:decvideo=5:header=5:spudec=5");
    //myargs += QLatin1String("-monitorpixelaspect");
    //myargs += QLatin1String("1");

    //myargs += QLatin1String("-input");
    //myargs += QLatin1String("nodefault-bindings:conf=/dev/null");

    //myargs += QLatin1String("-vf");
    //myargs += QLatin1String("yadif=3");

    myargs += QLatin1String("-wid");
    myargs += QString::number(xwinid);

    if(!m_cfg_videoOutput.isEmpty()) {
        myargs += QLatin1String("-vo");
        myargs += m_cfg_videoOutput;

        if(m_cfg_videoOutput.contains(QLatin1String("vdpau"))) {
            myargs += QLatin1String("-vc");
            myargs += QLatin1String("ffmpeg12vdpau,ffwmv3vdpau,ffvc1vdpau,ffh264vdpau,ffodivxvdpau,");
        }
    }

    // crashes with gl VO on gallium/radeon
    //myargs += QLatin1String("-lavdopts");
    //myargs += QLatin1String("threads=2");

    myargs += args;
#ifdef DEBUG_MPP_TIME
    QDateTime now = QDateTime::currentDateTimeUtc();
#endif
    MYDBG("%s", qPrintable(myargs.join(QLatin1String(" "))));

    resetValues();

    m_proc->start(m_cfg_mplayerPath, myargs, QIODevice::Unbuffered | QIODevice::ReadWrite);
    m_saved_mplayer_args = myargs;
    QDateTime started = QDateTime::currentDateTimeUtc();
#ifdef DEBUG_MPP_TIME
    now = started;
#endif
    TIMEMYDBG("start");
    changeState(MpState::IdleState, started);

}

void MpProcess::resetValues()
{
    set_screensaver_by_state();
    m_acc.clear();
    m_lastread_streamPosition = -1;
    m_streamposition_readt = QDateTime();
    m_last_emited_streampos = (-1);
    m_curr_speed = 1;
    m_currently_muted = false;
    m_stopped_because_of_long_seek = false;

    for(unsigned idx = 0; idx < MpState_maxidx; idx++) {
        m_max_encountered_readlatency_ms[idx] = (-1);
    }

    m_max_encountered_writelatency_ms = (-1);
    m_max_encountered_writequeuelatency_ms = (-1);
    m_previous_seek_direction_is_fwd = true;

    m_cfg_currently_parsing_mplayer_text = false;
    m_current_aid = 0;

}

void MpProcess::force_cmd(const QString &cmd)
{
    QDateTime now = QDateTime::currentDateTimeUtc();
    MYDBG("force_cmd %s", qPrintable(cmd));
    MpProcessCmd mpc(mpctag_string(), cmd, now);
    m_outputq.prepend(mpc);
    write_one_cmd(now, "forcing");
}

void MpProcess::write_one_cmd(const QDateTime &now, const char *reason)
{
#ifndef DEBUG_MPP_TIME
    Q_UNUSED(reason);
#endif

    if(m_outputq.isEmpty()) {
        return;
    }

    MpProcessCmd mpc = m_outputq.dequeue();
    make_heartbeat_active_or_not();
    const QString command = mpc.command(*this);

    if(command.isEmpty()) {
        return;
    }

    const qint64 writequeuelatency_ms = mpc.created().msecsTo(now);

    if(writequeuelatency_ms < 0) {
        PROGRAMMERERROR("WTF");
    }

    if(writequeuelatency_ms > m_max_encountered_writequeuelatency_ms) {
        m_max_encountered_writequeuelatency_ms = writequeuelatency_ms;
    }

    MYDBG("in: \"%s\" [%s] (%lu msec in Q of depth %u)", qPrintable(command), reason, (unsigned long)writequeuelatency_ms, (unsigned)m_outputq.size());
    m_lastwritet = now;
    TIMEMYDBG("write_one_cmd: m_lastwritet set to now");

    m_proc->write(command.toLocal8Bit() + '\n');

    if(m_cfg_output_accumulator_mode & Input) {
        if(m_cfg_acc_maxlines > 0) {
            MpProcessIolog iolog;
            iolog.line = command;
            iolog.dt = now;
            iolog.c = Input;
            m_acc.append(iolog);

            while(size_t(m_acc.size()) > m_cfg_acc_maxlines) {
                m_acc.removeFirst();
            }
        }
    }

    if(mpc.type() == Seek) {
        const double target = mpc.seektarget(*this);
        emit sig_seekedTo(target);
    }
}

bool MpProcess::allowed_to_write_now(const QDateTime &now, char const **reason)
{
    if(m_lastwritet.isNull()) {
        *reason = "first command";
        return true;
    }

    if(m_lastreadt.isNull()) {
        *reason = "never read before";
        return true;
    }

    const qint64 ms_since_last_write = m_lastwritet.msecsTo(now);

    if(!m_outputq.isEmpty()) {
        const MpProcessCmd &firstc = m_outputq.first();

        if(firstc.type() == Delay) {
            const double delay = firstc.delay();

            if(delay > ms_since_last_write) {
                *reason = "queued delay not gone yet";
                return false;
            }
            else {
                *reason = "queued delay gone";
                return true;
            }
        }
    }

    if(m_outputq.size() >= output_queue_large_size && ms_since_last_write > allow_writes_even_if_no_read_ms / 2) {
        *reason = "large queue and last write more than allow_writes_even_if_no_read_ms/2 ago";
        return true;
    }

    if(ms_since_last_write > allow_writes_even_if_no_read_ms) {
        *reason = "last write more than allow_writes_even_if_no_read_ms ago";
        return true;
    }

    if(m_lastreadt < m_lastwritet) {
        *reason = "no read since last write";
        return false;
    }

    if(m_outputq.size() >= output_queue_large_size && ms_since_last_write > min_time_between_writes_ms / 2) {
        *reason = "large queue and last write more than min_time_between_writes_ms/2 ago";
        return true;
    }

    if(ms_since_last_write > min_time_between_writes_ms) {
        *reason = "last write more than min_time_between_writes_ms ago";
        return true;
    }

    if(m_outputq.size() > output_queue_max_size) {
        *reason = "queue too large";
        return true;
    }

    *reason = "too soon";
    return false;
}

void MpProcess::try_to_write(const QDateTime &now)
{

    unsigned i = 0;

    while(true) {

        if(m_outputq.isEmpty()) {
            return;
        }

        char const *reason;

        QDateTime writetime;

        if(i == 0) {
            writetime = now;
        }
        else {
            writetime = QDateTime::currentDateTimeUtc();
        }

        if(allowed_to_write_now(writetime, &reason)) {
            write_one_cmd(writetime, reason);
        }
        else {
            TIMEMYDBG("try_to_write: %s", reason);
            return;
        }

    }

}

void MpProcess::slot_try_to_write_now()
{
    QDateTime now = QDateTime::currentDateTimeUtc();
    //TIMEMYDBG("try_to_write_now");
    try_to_write(now);
}

void MpProcess::slot_submit_write(const MpProcessCmd &mpc, const QDateTime &submittime)
{
    Q_UNUSED(submittime);

    // if this is a Seek, and there are already seeks in here, do not submit a duplicate
    if(0 && mpc.type() == Seek) {
        double thistarget = mpc.seektarget(*this);
        // if such a target already exists, do not put it in
        foreach(const MpProcessCmd & qcmd, m_outputq) {
            if(qcmd.type() != Seek) {
                continue;
            }

            double qtarget = qcmd.seektarget(*this);

            if(qAbs(qtarget - thistarget) < 0.1) {
                QDateTime now = QDateTime::currentDateTimeUtc();
                MYDBG("not submitting duplicate seek to %f", thistarget);
                return;
            }
        }
    }

    m_outputq.enqueue(mpc);
    INVOKE_DELAY(slot_try_to_write_now());

    make_heartbeat_active_or_not();

    // error path: too many elements in queue

    if(m_outputq.size() <= output_queue_max_size) {
        return;
    }

    qWarning() << "output queue size is " << m_outputq.size() << "! Will try to empty it";

    unsigned i = 0;

    // try to empty within one second, but not slower than one per min_time_between_writes_ms / 3
    const useconds_t sleeptime = qMin(useconds_t(1000000 / m_outputq.size()), useconds_t(min_time_between_writes_ms / 3));

    while(!m_outputq.isEmpty()) {
        QDateTime now = QDateTime::currentDateTimeUtc();
        write_one_cmd(now, "too large output queue");
        usleep(sleeptime);
        // might have accumulated output in the meantime
        slot_readStdout();
        slot_readStderr();
        i++;

        if(i > 2 * output_queue_max_size) {
            PROGRAMMERERROR("WTF");
        }
    }
}
void MpProcess::slot_submit_write(const QString &command)
{
    QDateTime submitted = QDateTime::currentDateTimeUtc();
#ifdef DEBUG_MPP_TIME
    const QDateTime &now = submitted;
#endif
    TIMEMYDBG("slot_submit_write");

    MpProcessCmd mpc(mpctag_string(), command, submitted);
    slot_submit_write(mpc, submitted);
}
void MpProcess::slot_submit_write_latin1(char const *const latin1lit)
{
    slot_submit_write(QString::fromLatin1(latin1lit));
}
void MpProcess::slot_osd_show_location()
{
    QDateTime now = QDateTime::currentDateTimeUtc();
    MpProcessCmd mpc(mpctag_osdsl(), now);
    slot_submit_write(mpc, now);
}
/*!
 * \brief Media playback seeking
 *
 * \param offset Seeking offset in seconds
 * \param whence Seeking mode
 * \returns \p true If the seeking mode is valid
 * \sa tell()
 */
void MpProcess::slot_seek_from_slider(int position)
{
    QDateTime now = QDateTime::currentDateTimeUtc();
    MYDBG("Got a seek(%d) from the slider", position);

    const double pos = expectedPositionAt(now, false);
    const double tarpos = position;

    if(qAbs(pos - tarpos) < ignore_seeks_from_slider_closer_than_sec) {
        MYDBG("Got a seek(%d) from the slider but we already are at %f", position, pos);
        return;
    }

    MYDBG("Got a seek(%d) from the slider", position);
    core_seek(tarpos, AbsoluteSeek);
}

void MpProcess::slot_rel_seek_from_keyboard(double offset)
{
#ifdef DEBUG_MPP_TIME
    QDateTime now = QDateTime::currentDateTimeUtc();
#endif
    MYDBG("Got a relative seek(%f) from the keyboard", offset);
    core_seek(offset, RelativeSeek);
}

void MpProcess::slot_abs_seek_from_keyboard(double position)
{
#ifdef DEBUG_MPP_TIME
    QDateTime now = QDateTime::currentDateTimeUtc();
#endif
    MYDBG("Got an absolute seek(%f) from the keyboard", position);
    core_seek(position, AbsoluteSeek);
}

/*!
 * \brief Media playback seeking
 *
 * \param offset Seeking offset in seconds
 * \param whence Seeking mode
 * \returns \p true If the seeking mode is valid
 * \sa tell()
 */
void MpProcess::core_seek(double offset, SeekMode whence)
{
    QDateTime now = QDateTime::currentDateTimeUtc();

    MpProcessCmd mpc(mpctag_seek(), whence, offset, now);
    slot_submit_write(mpc, now);

    slot_submit_write_latin1("pausing_keep_force get_time_pos");
}
void MpProcess::foundReadSpeed(double rspeed)
{
#ifdef DEBUG_MPP_TIME
    QDateTime now = QDateTime::currentDateTimeUtc();
#endif

    if(qAbs(m_curr_speed - rspeed) > 0.01) {

        if(qAbs(rspeed - 1.) < 0.01) { // nromal speed - should not be muted
            if(m_currently_muted) { // but currently muted
                MYDBG("currently muted but at a speed of %f it should not be - scheduling unmuting", rspeed);
                INVOKE_DELAY(slot_unmute());
            }
        }
        else { // should be muted
            if(!m_currently_muted) { // but currently not muted
                MYDBG("currently unmuted but at a speed of %f it should be - scheduling muting", rspeed);
                INVOKE_DELAY(slot_mute());
            }
        }
    }

    m_curr_speed = rspeed;
}

void MpProcess::slot_speed_up()
{
    if(m_curr_state != MpState::PlayingState) {
        return;
    }

    if(m_curr_speed > 100) {
        return;
    }

    if(m_curr_speed > 1. / speed_multiplier && m_curr_speed < 1.) {
        slot_submit_write_latin1("speed_set 1.0");

        if(m_currently_muted) {
#ifdef DEBUG_MPP_TIME
            QDateTime now = QDateTime::currentDateTimeUtc();
#endif
            MYDBG("speeding up to 1 - unmuting after");
            slot_unmute();
        }
    }
    else {
        if(!m_currently_muted) {
#ifdef DEBUG_MPP_TIME
            QDateTime now = QDateTime::currentDateTimeUtc();
#endif
            MYDBG("speeding up - muting first");
            slot_mute();
        }

        QString cmd = QLatin1String("speed_mult %1");
        cmd = cmd.arg(speed_multiplier);
        slot_submit_write(cmd);
    }

    slot_submit_write_latin1("get_property speed");
}
void MpProcess::slot_slow_down()
{
    if(m_curr_state != MpState::PlayingState) {
        return;
    }

    if(m_curr_speed <= 0.01) {
        return;
    }

    if(m_curr_speed < speed_multiplier && m_curr_speed > 1.) {
        slot_submit_write_latin1("speed_set 1.0");

        if(m_currently_muted) {
#ifdef DEBUG_MPP_TIME
            QDateTime now = QDateTime::currentDateTimeUtc();
#endif
            MYDBG("slowing down to 1 - unmuting after");
            slot_unmute();
        }
    }
    else {
        if(!m_currently_muted) {
#ifdef DEBUG_MPP_TIME
            QDateTime now = QDateTime::currentDateTimeUtc();
#endif
            MYDBG("slowing down - muting first");
            slot_mute();
        }

        QString cmd = QLatin1String("speed_mult %1");
        cmd = cmd.arg(1. / speed_multiplier);
        slot_submit_write(cmd);
    }

    slot_submit_write_latin1("get_property speed");
}


void MpProcess::slot_pause()
{
    if(m_curr_state == MpState::PlayingState) {
        slot_submit_write_latin1("pause");
        QDateTime now = QDateTime::currentDateTimeUtc();
        changeState(MpState::PausedState, now);
    }

    slot_submit_write_latin1("pausing_keep_force get_property pause");
    slot_submit_write_latin1("pausing_keep_force get_time_pos");
}

void MpProcess::slot_play()
{
    if(m_curr_state == MpState::PausedState) {
        slot_submit_write_latin1("pause");
        QDateTime now = QDateTime::currentDateTimeUtc();
        changeState(MpState::PlayingState, now);
    }

    slot_submit_write_latin1("pausing_keep_force get_property pause");
    slot_submit_write_latin1("pausing_keep_force get_time_pos");
}

void MpProcess::slot_stop()
{
    m_outputq.clear();
    force_cmd(QLatin1String("stop"));
    m_cfg_currently_parsing_mplayer_text = false;
    usleep(sleep_after_stop_command_msec * 1000);
    m_proc->waitForReadyRead(waitforreadyread_after_stop_command_msec); // force a buffer flush and wait even longer
    QCoreApplication::processEvents();
    clear_out_incremental_stdouterr();
    QDateTime now = QDateTime::currentDateTimeUtc();
    TIMEMYDBG("stop");
    changeState(MpState::StoppedState, now);
}

void MpProcess::clear_out_incremental_stdouterr()
{
    slot_readStdout();
    slot_readStderr();

    if(m_not_yet_processed_out.size() > 0) {
        proc_incremental_std(m_not_yet_processed_out, Output);
        m_not_yet_processed_out.clear();
    }


    if(m_not_yet_processed_err.size() > 0) {
        proc_incremental_std(m_not_yet_processed_err, Error);
        m_not_yet_processed_err.clear();
    }
}

void MpProcess::clear_out_iobuffer_before_load()
{
    m_proc->waitForReadyRead(beforeload_waitForReadyRead_1_msec);
    usleep(beforeload_sleep_msec * 1000);
    QCoreApplication::processEvents();
    QByteArray b;
    b = m_proc->readAllStandardOutput();
    b = m_proc->readAllStandardError();
    m_proc->waitForReadyRead(beforeload_waitForReadyRead_2_msec);
    QCoreApplication::processEvents();
    b = m_proc->readAllStandardOutput();
    b = m_proc->readAllStandardError();
}

void MpProcess::dbg_out(char const *pref, const QString &line, const QDateTime &now)
{
#ifndef DEBUG_MPP_TIME
    Q_UNUSED(now);
#endif

#ifdef DEBUG_MPP_TIME

    static const VRegExp rx_statusline((QLatin1String("(") + QLatin1String(statusline_rxlit) + QLatin1String(")")));
    static const VRegExp rx_crap((QLatin1String("(") + QLatin1String(crap_rxlit) + QLatin1String(")")));

    static const bool QMP_PRINT_CRAP_OUTPUT = setand1_getenv("QMP_PRINT_CRAP_OUTPUT");
    static const bool QMP_PRINT_STATUS_OUTPUT = setand1_getenv("QMP_PRINT_STATUS_OUTPUT");

    const bool line_is_crap = (rx_crap.indexIn(line) >= 0);
    const bool line_is_status = (rx_statusline.indexIn(line) >= 0);

    bool doprint;

    if(line_is_crap) {
        doprint = QMP_PRINT_CRAP_OUTPUT;
    }
    else if(line_is_status) {
        doprint = QMP_PRINT_STATUS_OUTPUT;
    }
    else {
        doprint = true;
    }

    if(doprint) {
        MYDBG("%s: \"%s\"", pref, qPrintable(line));
    }

#else
    Q_UNUSED(pref);
    Q_UNUSED(line);
    Q_UNUSED(now);
#endif
}

void MpProcess::set_output_accumulator_ignore_default()
{
    const size_t pattlen = 1 + strlen(statusline_rxlit) + 1 + strlen(crap_rxlit) + 1 + 1;
    char *patt = new char[pattlen];
    strcpy(patt, "(");
    strcat(patt, statusline_rxlit);
    strcat(patt, "|");
    strcat(patt, crap_rxlit);
    strcat(patt, ")");
#ifdef DEBUG_MPP_TIME
    QDateTime now = QDateTime::currentDateTimeUtc();
#endif
    MYDBG("ignoring all MP output matching \"%s\"", patt);
    set_output_accumulator_ignore(patt);
    delete[] patt;
}

void MpProcess::proc_incremental_std(const QByteArray &bytes, IOChannel c)
{
    if(bytes.isEmpty()) {
        return;
    }

    QDateTime readtime = QDateTime::currentDateTimeUtc();
    //TIMEMYDBG("proc_incremental_std");

    QByteArray *arr = (c == Output ? &m_not_yet_processed_out : &m_not_yet_processed_err);
    arr->append(bytes);

    QStringList lines;
    bool found_valid = false;

    while(true) {
        const int idxn = arr->indexOf('\n');
        const int idxr = arr->indexOf('\r');

        if(idxn < 0 && idxr < 0) {
            break;
        }

        int idx;

        if(idxn < 0) {
            idx = idxr;
        }
        else if(idxr < 0) {
            idx = idxn;
        }
        else {
            idx = (idxr < idxn ? idxr : idxn);
        }

        QByteArray bline = arr->left(idx);
        arr->remove(0, idx + 1);

        QString line = warn_xbin_2_local_qstring(bline);
        line.remove(QLatin1String("\r"));

        if(line.size() < 1) {
            continue;
        }

        dbg_out(c == Output ? "out" : "err", line, readtime);

        if(m_cfg_acc_maxlines > 0) {
            if(m_cfg_output_accumulator_mode & c) {
                if(m_cfg_output_accumulator_ignore == NULL || m_cfg_output_accumulator_ignore->indexIn(line) < 0) {
                    MpProcessIolog iolog;
                    iolog.line = line;
                    iolog.c = c;
                    iolog.dt = readtime;
                    m_acc.append(iolog);

                    while(size_t(m_acc.size()) > m_cfg_acc_maxlines) {
                        m_acc.removeFirst();
                    }
                }
            }
        }

        lines.append(line);

        if(!line.contains(QLatin1String(": ["))) {
            found_valid = true;
        }

    }

    if(found_valid) {
        update_lastreadt(readtime);
    }

    QStringList positionlines;
    QList<MpState> newstates;
    QStringList errorreasons;
    QList<double> foundspeeds;

    foreach(const QString & line, lines) {
        /* might
            * slot_submit_write()
            * go to error state and emit error string
            * emit loaddone
            * change m_mediainfo
        */
        parseLine(line, readtime, positionlines, newstates, errorreasons, foundspeeds);

    }

    if(!newstates.isEmpty()) {
        MpState newstate = newstates.last();
        changeState(newstate, readtime);
    }

    if(!positionlines.isEmpty()) {
        QString lastpositionline = positionlines.last();
        parsePosition(lastpositionline, readtime);
    }

    if(!errorreasons.isEmpty()) {
        QString reason = errorreasons.join(QLatin1String(", "));
        changeToErrorState(reason, readtime);
    }

    if(!foundspeeds.isEmpty()) {
        double speed = foundspeeds.last();
        foundReadSpeed(speed);
    }

    if(found_valid) {
        if(!m_outputq.isEmpty()) {
            INVOKE_DELAY(slot_try_to_write_now());
        }
    }
}

void MpProcess::slot_readStdout()
{
    QByteArray bytes = m_proc->readAllStandardOutput();
    proc_incremental_std(bytes, Output);
}

void MpProcess::slot_readStderr()
{
    QByteArray bytes = m_proc->readAllStandardError();
    proc_incremental_std(bytes, Error);
}

static char const *ProcessError_2_latin1str(QProcess::ProcessError e)
{
    switch(e) {
        case QProcess::FailedToStart:
            return "The process failed to start. Either the invoked program is missing, or you may have insufficient permissions to invoke the program.";

        case QProcess::Crashed:
            return "The process crashed some time after starting successfully.";

        case QProcess::Timedout:
            return "The last waitFor...() function timed out. The state of QProcess is unchanged, and you can try calling waitFor...() again.";

        case QProcess::WriteError:
            return "An error occurred when attempting to write to the process. For example, the process may not be running, or it may have closed its input channel.";

        case QProcess::ReadError:
            return "An error occurred when attempting to read from the process. For example, the process may not be running.";

        case QProcess::UnknownError:
            return "An unknown error occurred. This is the default return value of error().";

        default:
            return "unknown error";
    }
}
void MpProcess::slot_error_received(QProcess::ProcessError e)
{
    return;
    const QString err = QLatin1String(ProcessError_2_latin1str(e));
    QDateTime now = QDateTime::currentDateTimeUtc();
    TIMEMYDBG("error_received");
    m_proc->disconnect(this);
    m_proc->disconnect();
    disconnect(m_proc);
    changeToErrorState(err, now);
}

void MpProcess::slot_finished(int exitcode, QProcess::ExitStatus exitStatus)
{
    QDateTime now = QDateTime::currentDateTimeUtc();
    MYDBG("process finished with exitcode %d status %s", exitcode, (exitStatus == QProcess::NormalExit ? "normal" : "crash"));

    // Called if the *process* has finished
    if(exitStatus == QProcess::NormalExit) {
        changeState(MpState::NotStartedState, now);
        return;
    }

    m_proc->disconnect(this);
    m_proc->disconnect();
    disconnect(m_proc);

    if(exitcode == 0) {
        QString msg = QLatin1String("mplayer failed");
        changeToErrorState(msg, now);
        return;
    }

    QString msg = QLatin1String("mplayer failed with code ") + QString::number(exitcode);
    changeToErrorState(msg, now);
}

void MpProcess::slot_ask_for_metadata()
{
    if(m_curr_state == MpState::LoadingState || m_curr_state == MpState::BufferingState) {
        slot_submit_write_latin1("get_property metadata");
    }
}
void MpProcess::slot_quit_if_we_are_not_playing_yet()
{
    if(m_curr_state != MpState::LoadingState) {
        {
#ifdef DEBUG_MPP_TIME
            QDateTime now = QDateTime::currentDateTimeUtc();
#endif
            MYDBG("quit_if_we_are_not_playing_yet - but not in loadingstate");
        }
        return;
    }


    const qint64 elapsed_ms = m_loadingtimer.elapsed();

    if(elapsed_ms < max_time_for_loading_file_ms) {
#ifdef DEBUG_MPP_TIME
        QDateTime now = QDateTime::currentDateTimeUtc();
#endif
        MYDBG("quit_if_we_are_not_playing_yet - but not enough time elapsed");
        return;
    }

    const QDateTime now = QDateTime::currentDateTimeUtc();
    MYDBG("quit_if_we_are_not_playing_yet");
    const QString msg = QString(QLatin1String("playback did not start, waited %1 msecs")).arg(m_loadingtimer.elapsed());
    changeToErrorState(msg, now);
}

template<typename T>
static inline T MIN(const T a, const T b)
{
    if(a < b) {
        return a;
    }
    else {
        return b;
    }
}
void MpProcess::slot_load(const QString &url)
{
    Q_ASSERT_X(m_proc->state() != QProcess::NotRunning, "QMPProcess::load()", "MPlayer process not started yet");

    m_outputq.clear();

    // From the MPlayer slave interface documentation:
    // "Try using something like [the following] to switch to the next file.
    // It avoids audio playback starting to play the old file for a short time
    // before switching to the new one.
    if(m_curr_state == MpState::PausedState || m_curr_state == MpState::PlayingState) {
        slot_submit_write_latin1("pausing_keep_force pt_step 1");
    }
    else if(m_curr_state == MpState::StoppedState) {
        slot_submit_write_latin1("get_property pause");
    }

    QChar sep = QLatin1Char('"');

    if(url.contains(sep)) {
        sep = QLatin1Char('\'');

        if(url.contains(sep)) {
            PROGRAMMERERROR("bad url %s: contains both single and double quote", qPrintable(url));
        }
    }

    // try to read the start of the file, fail if not possible
    // this can have significant latency, so it needs to happen before the changeState()
    const qint64 failafter_msec = max_time_for_preloading_file_ms;
    const qint64 sleep_usec = failafter_msec * 10;
    const off_t maxreadsize = preloading_file_KBytes * 1024;

    const QString load_start_error = try_load_start_of_file(url, maxreadsize, failafter_msec, sleep_usec);

    if(!load_start_error.isEmpty()) {
        QDateTime now = QDateTime::currentDateTimeUtc();
        changeToErrorState(load_start_error, now);
        return;
    }

    QCoreApplication::processEvents();

    clear_out_iobuffer_before_load();
    resetValues();

    changeState(MpState::LoadingState, QDateTime::currentDateTimeUtc());

    resetValues();
    m_cfg_currently_parsing_mplayer_text = true;
    slot_submit_write(QString(QLatin1String("loadfile %1%2%1")).arg(sep).arg(url));
    m_loadingtimer.start();

    QDateTime now = QDateTime::currentDateTimeUtc();
    TIMEMYDBG("load");
    m_dont_trust_time_from_statusline_till = now.addMSecs(ignore_statusline_after_load_ms);
    m_previous_seek_direction_is_fwd = true;

    QTimer::singleShot(max_time_for_loading_file_ms, this, SLOT(slot_quit_if_we_are_not_playing_yet()));

}

static int QSToInt(const QString &s)
{
    bool ok = false;
    int ret = s.toInt(&ok);

    if(ok == false) {
        PROGRAMMERERROR("could not convert \"%s\" to int", qPrintable(s));
    }

    return ret;
}
static double QSToDouble(const QString &s)
{
    bool ok = false;
    double ret = s.toDouble(&ok);

    if(ok == false) {
        PROGRAMMERERROR("could not convert \"%s\" to double", qPrintable(s));
    }

    return ret;
}

void MpProcess::slot_mute()
{
    slot_submit_write_latin1("mute 1");
    m_currently_muted = true;
    slot_submit_write_latin1("get_property mute");
}
void MpProcess::slot_unmute()
{
    slot_submit_write_latin1("mute 0");
    m_currently_muted = false;
    slot_submit_write_latin1("get_property mute");
}

void MpProcess::parseLine(const QString &line, const QDateTime &readtime, QStringList &positionlines, QList<MpState> &newstates, QStringList &errorreasons, QList<double> &foundspeeds)
{
    const QString tline = line.trimmed();
#ifdef DEBUG_MPP_TIME
    const QDateTime &now = readtime;
#endif

    if(!m_cfg_currently_parsing_mplayer_text) {
        MYDBG("ignoring \"%s\" from mplayer", qPrintable(line));
        return;
    }


    if(tline.startsWith(QLatin1String("Playing "))) {
    }

    else if(tline == QLatin1String("GLOBAL: ANS_pause=no")) {
        if(m_curr_state == MpState::PausedState) {
            newstates.append(MpState::PlayingState);
            MYDBG("got ANS_pause=no, guessing on PlayingState");
        }
    }
    else if(tline == QLatin1String("GLOBAL: ANS_pause=yes")) {
        if(m_curr_state == MpState::PlayingState) {
            newstates.append(MpState::PausedState);
            MYDBG("got ANS_pause=yes, guessing on PausedState");
        }
    }
    else if(tline == QLatin1String("GLOBAL: ANS_mute=no")) {
        m_currently_muted = false;
    }
    else if(tline == QLatin1String("GLOBAL: ANS_mute=yes")) {
        m_currently_muted = true;
    }
    else if(tline.startsWith(QLatin1String("GLOBAL: ANS_switch_audio="))) {
        const QString aids = tline.mid(25);
        int aid = QSToInt(aids);

        if(aid == m_current_aid) {
            MYDBG("current AID %d (%s). Same read back from %s", m_current_aid, qPrintable(m_mediaInfo->aid_2_alang(m_current_aid)), qPrintable(aids));
        }
        else {
            MYDBG("current AID %d (%s). read AID %d from %s \"%s\"", m_current_aid, qPrintable(m_mediaInfo->aid_2_alang(m_current_aid)), aid,  qPrintable(aids), qPrintable(m_mediaInfo->aid_2_alang(aid)));
            m_current_aid = aid;
        }
    }
    else if(tline.startsWith(QLatin1String("GLOBAL: ANS_speed="))) {
        const QString sps = tline.mid(18);
        double readspeed = QSToDouble(sps);
        MYDBG("read speed %f from %s", readspeed, qPrintable(sps));
        foundspeeds.append(readspeed);
    }
    else if(tline.startsWith(QLatin1String("GLOBAL: ANS_time_pos="))) {
        positionlines.append(tline);
    }
    else if(tline.startsWith(QLatin1String("GLOBAL: ANS_TIME_POSITION="))) {
        positionlines.append(tline);
    }

    else if(tline.startsWith(QLatin1String("Cache fill:"))) {
    }
    else if(tline.startsWith(QLatin1String("CPLAYER: Starting playback..."))) {
        MYDBG("got Starting playback, submitting the metadata request");
        INVOKE_DELAY(slot_ask_for_metadata());
        // do not trigger state on this - it will still output ID_... info after encountering this line
    }
    else if(tline.startsWith(QLatin1String("File not found: "))) {
        MYDBG("got File not found, error!");
        errorreasons.append(tline);
    }
    else if(tline.endsWith(QLatin1String("IDENTIFY: ID_PAUSED"))) {
        MYDBG("got ID_PAUSED, guessing on PausedState");
        newstates.append(MpState::PausedState);
    }
    else if(tline.startsWith(QLatin1String("IDENTIFY: ID_SIGNAL"))) {
        MYDBG("got ID_SIGNAL, error!");
        errorreasons.append(tline);
    }
    else if(tline.startsWith(QLatin1String("FATAL: Could not initialize video filters (-vf) or video output (-vo)"))) {
        MYDBG("got Could not initialize video filters, error!");
        errorreasons.append(tline);
    }
    else if(tline.startsWith(QLatin1String("IDENTIFY: ID_EXIT"))) {
        MYDBG("got ID_EXIT, going to StoppedState");
        newstates.append(MpState::StoppedState);
    }
    else if(tline.startsWith(QLatin1String("IDENTIFY: ID_"))) {
        parseMediaInfo(tline, readtime);
    }
    else if(tline.contains(QLatin1String("No stream found"))) {
        MYDBG("got No stream found, error!");
        errorreasons.append(tline);
    }
    else if((tline.startsWith(QLatin1String("STATUSLINE: A:")) || tline.startsWith(QLatin1String("STATUSLINE: V:")))) {
        positionlines.append(tline);
    }
    else if(tline.startsWith(QLatin1String("Exiting..."))) {
        MYDBG("got Exiting, going to StoppedState");
        newstates.append(MpState::StoppedState);
    }
    else if(tline.startsWith(QLatin1String("GLOBAL: EOF code:"))) {
        MYDBG("got EOF code, going to StoppedState");
        newstates.append(MpState::StoppedState);
    }
    else if(tline.startsWith(QLatin1String("DEMUXER: ds_fill_buffer: EOF reached (stream: video)"))) {
        // this can also happen if the cache is empty
        //newstates.append(MpState::StoppedState);
    }
    else if(tline.startsWith(QLatin1String("GLOBAL: ANS_metadata="))) {
        if(!m_mediaInfo->is_finalized()) {
            m_mediaInfo->set_finalized(); // No more info here
            MYDBG("got ANS_metadata=, loading is done and going to PlayingState");
            newstates.append(MpState::PlayingState);
            emit sig_loadDone();
        }
    }
    else if(tline.startsWith(QLatin1String("Fontconfig warning:"))) {
    }
    else {
        //MYDBG("did not understand line %s", qPrintable(tline));
    }
}

// Parses MPlayer's media identification output
void MpProcess::parseMediaInfo(const QString &tline, const QDateTime &readtime)
{
#ifndef DEBUG_MPP_TIME
    Q_UNUSED(readtime);
#endif
    static VRegExp rxalang("^ID_AID_(\\d+)_LANG$");
    static VRegExp rxslang("^ID_SID_(\\d+)_LANG$");

    QString line = tline.trimmed();
    line.remove(QLatin1String("IDENTIFY:"));
    line = line.trimmed();

    QStringList info = line.split(QLatin1Char('='));

    if(info.count() < 2) {
        return;
    }

    if(info[0] == QLatin1String("ID_VIDEO_FORMAT")) {
        m_mediaInfo->set_videoFormat(info[1]);
    }
    else if(info[0] == QLatin1String("ID_VIDEO_BITRATE")) {
        m_mediaInfo->set_videoBitrate(QSToInt(info[1]));
    }
    else if(info[0] == QLatin1String("ID_VIDEO_WIDTH")) {
        m_mediaInfo->set_width(QSToInt(info[1]));
    }
    else if(info[0] == QLatin1String("ID_VIDEO_HEIGHT")) {
        m_mediaInfo->set_height(QSToInt(info[1]));
    }
    else if(info[0] == QLatin1String("ID_VIDEO_FPS")) {
        m_mediaInfo->set_framesPerSecond(QSToDouble(info[1]));

    }
    else if(info[0] == QLatin1String("ID_AUDIO_FORMAT")) {
        // this can still be output when switching tracks
        if(!m_mediaInfo->is_finalized()) {
            m_mediaInfo->set_audioFormat(info[1]);
        }
    }
    else if(info[0] == QLatin1String("ID_AUDIO_BITRATE")) {
        // this can still be output when switching tracks
        if(!m_mediaInfo->is_finalized()) {
            m_mediaInfo->set_audioBitrate(QSToInt(info[1]));
        }
    }
    else if(info[0] == QLatin1String("ID_AUDIO_RATE")) {
        // this can still be output when switching tracks
        if(!m_mediaInfo->is_finalized()) {
            m_mediaInfo->set_sampleRate(QSToInt(info[1]));
        }
    }
    else if(info[0] == QLatin1String("ID_AUDIO_NCH")) {
        // this can still be output when switching tracks
        if(!m_mediaInfo->is_finalized()) {
            m_mediaInfo->set_numChannels(QSToInt(info[1]));
        }

    }
    else if(info[0] == QLatin1String("ID_LENGTH")) {
        m_mediaInfo->set_length(QSToDouble(info[1]));
    }
    else if(info[0] == QLatin1String("ID_SEEKABLE")) {
        m_mediaInfo->set_seekable((bool)QSToInt(info[1]));
    }
    else if(info[0].startsWith(QLatin1String("ID_CLIP_INFO_NAME"))) {
        m_currentTag = info[1];
    }
    else if(info[0].startsWith(QLatin1String("ID_CLIP_INFO_VALUE")) && !m_currentTag.isEmpty()) {
        m_mediaInfo->add_tag(m_currentTag, info[1]);
    }
    else if(info[0].startsWith(QLatin1String("ID_CHAPTER"))) {
        m_mediaInfo->add_tag(m_currentTag, info[1]);
    }
    else if(rxalang.indexIn(info[0]) >= 0) {
        const QString &aid = rxalang.cap(1);
        const QString &alang = info[1];
        m_mediaInfo->add_alang(QSToInt(aid), alang);
    }
    else if(rxslang.indexIn(info[0]) >= 0) {
        const QString &sid = rxslang.cap(1);
        const QString &slang = info[1];
        m_mediaInfo->add_slang(QSToInt(sid), slang);
    }
    else if(info[0] == QLatin1String("ID_START_TIME")) {
    }
    else if(info[0] == QLatin1String("ID_DEMUXER")) {
    }
    else if(info[0] == QLatin1String("ID_VIDEO_ASPECT")) {
        QString sDAR = info[1];
        double DAR = QSToDouble(sDAR);
        m_mediaInfo->set_DAR(DAR);
    }
    else if(info[0] == QLatin1String("ID_VIDEO_ID")) {
    }
    else if(info[0] == QLatin1String("ID_VIDEO_CODEC")) {
    }
    else if(info[0] == QLatin1String("ID_AUDIO_CODEC")) {
    }
    else if(info[0] == QLatin1String("ID_AUDIO_TRACK")) {
    }
    else if(info[0] == QLatin1String("ID_AUDIO_ID")) {
    }
    else if(info[0] == QLatin1String("ID_SUBTITLE_ID")) {
    }
    else if(info[0] == QLatin1String("ID_CLIP_INFO_N")) {
    }
    else if(info[0] == QLatin1String("ID_FILENAME")) {
    }
    else {
#ifdef DEBUG_MPP_TIME
        const QDateTime &now = readtime;
#endif
        MYDBG("unknown mediainfo %s=%s", qPrintable(info[0]), qPrintable(info[1]));
    }
}

// Parses MPlayer's position output
// possible outcomes:
//    * nothing
//    * new position
//    * seek to a specific position to fix AV mis-sync
void MpProcess::parsePosition(const QString &tline, const QDateTime &readtime)
{

#ifdef DEBUG_MPP_TIME
    const QDateTime &now = readtime;
#endif

    switch(m_curr_state) {
        case(MpState::ErrorState):
        case(MpState::IdleState):
        case(MpState::LoadingState):
        case(MpState::NotStartedState):
        case(MpState::StoppedState):
            return;
            break;

        default:
            break;
    }

    const double oldpos = currentExpectedPosition(false);
    QList<double> contenders;
    bool trust_this_after_seek = false;
    double seektarget = (-1);

    static const QString atp1 = QLatin1String("GLOBAL: ANS_time_pos=");
    static const QString atp2 = QLatin1String("GLOBAL: ANS_TIME_POSITION=");

    if(tline.startsWith(atp1)) {
        QString ps = tline.mid(atp1.length());
        bool ok = false;
        double res = ps.toDouble(&ok);

        if(!ok) {
            PROGRAMMERERROR("could not convert \"%s\" to double", qPrintable(ps));
        }

        contenders.append(res);
    }
    else if(tline.startsWith(atp2)) {
        QString ps = tline.mid(atp2.length());
        bool ok = false;
        double res = ps.toDouble(&ok);

        if(!ok) {
            PROGRAMMERERROR("could not convert \"%s\" to double", qPrintable(ps));
        }

        contenders.append(res);
        trust_this_after_seek = true;
    }
    else {
        // STATUSLINE: A: 913.6 V: 897.9 A-V: 15.733 ct:  3.697   0/  0 28%  3%  1.2% 465 0 50%
        static const VRegExp rx("[ :]");

        QStringList info = tline.split(rx, QString::SkipEmptyParts);

        for(int i = 0; i < info.count(); i++) {
            if((info[i] == QLatin1String("V") || info[i] == QLatin1String("A")) && info.count() > i) {
                bool ok = false;
                const QString &ps = info[i + 1];
                double res = ps.toDouble(&ok);

                if(!ok) {
                    PROGRAMMERERROR("could not convert \"%s\" to double", qPrintable(ps));
                }

                contenders.append(res);
            }
            else if(info[i] == QLatin1String("A-V")) {
                bool ok = false;
                const QString &amvs = info[i + 1];
                double amv = amvs.toDouble(&ok);

                if(!ok) {
                    PROGRAMMERERROR("could not convert \"%s\" to double",  qPrintable(amvs));
                }

                const bool normalspeed = qAbs(m_curr_speed - 1.) < 0.01;
                const double max_avmissynax = (normalspeed ? max_avmissync_before_explicit_seek_normal_speed_secs : max_avmissync_before_explicit_seek_unnormal_speed_secs);

                if(max_avmissynax > 0. && qAbs(amv) > max_avmissynax) {
                    double min = contenders.first();
                    double max = contenders.first();
                    foreach(const double & v, contenders) {
                        if(v < min) {
                            min = v;
                        }

                        if(v > min) {
                            max = v;
                        }
                    }
                    min = int(min);
                    max = ceil(max);
                    seektarget = (normalspeed ? min : max);
                }
            }
        }
    }

    if(contenders.isEmpty()) {
        qWarning("could not get position from \"%s\"", qPrintable(tline));
        return;
    }

    double sum = 0;
    foreach(double v, contenders) {
        sum += v;
    }

    double parsedpos = sum / contenders.size();

    if(parsedpos < 0) {
        if(parsedpos < minimum_allowed_negative_position_from_statusline) {
            MYDBG("parsed negative position %f from \"%s\"? ignoring", parsedpos, qPrintable(tline));
            return;
        }
        else {
            MYDBG("parsed negative position %f from \"%s\"? assuming 0", parsedpos, qPrintable(tline));
            parsedpos = 0.;
        }
    }

    if(m_mediaInfo->has_length()) {
        const double len = m_mediaInfo->length();

        if(len > 0 && parsedpos > 0) {
            if(parsedpos > len - force_stop_sec_before_end && parsedpos < len + max_overflow_position_to_length_sec) {
                //MYDBG("parsed position %f close to end %f, stopping movie", parsedpos, len);
                //INVOKE_DELAY(stop());
                //return;
            }

            else if(len < parsedpos) {
                PROGRAMMERERROR("parsed stream position %f much larger than media %f", parsedpos, len);
            }
        }
    }

    if(seektarget > 0) {
        if(!trust_this_after_seek && !m_dont_trust_time_from_statusline_till.isNull() && m_dont_trust_time_from_statusline_till > readtime) {
            // but a seek is in progress
        }
        else {
            const MpProcessCmd seekcmd(mpctag_seek(), MpProcess::AbsoluteSeek, seektarget, readtime);
            const QString cmd = seekcmd.command(*this);
            force_cmd(cmd);
            usleep(sleep_after_seeking_msec * 100);
            const QLatin1String gettimecmd("pausing_keep_force get_time_pos");
            force_cmd(gettimecmd);
            return;
        }
    }

    if(!m_dont_trust_time_from_statusline_till.isNull()) {
        if(m_dont_trust_time_from_statusline_till > readtime) {
            if(trust_this_after_seek) {
                // do not reset m_dont_trust_time_from_statusline_till here!
                TIMEMYDBG("not ignoring read position - I trust this input \"%s\"",  qPrintable(tline));
            }
            else if(m_previous_seek_direction_is_fwd && oldpos >= 0 && oldpos < parsedpos) {
                m_dont_trust_time_from_statusline_till = QDateTime();
                TIMEMYDBG("not ignoring read position - going forward from %f [%s]", oldpos, qPrintable(tline));
            }
            else if(!m_previous_seek_direction_is_fwd && oldpos >= 0 && oldpos > parsedpos) {
                m_dont_trust_time_from_statusline_till = QDateTime();
                TIMEMYDBG("not ignoring read position - going backwards from %f [%s]", oldpos, qPrintable(tline));
            }
            else if(oldpos >= 0 && qAbs(parsedpos - oldpos) < no_not_ignore_parsed_position_if_closer_than_sec) {
                m_dont_trust_time_from_statusline_till = QDateTime();
                TIMEMYDBG("not ignoring read position - assuming more true than %f [%s]", oldpos, qPrintable(tline));
            }
            else {
                TIMEMYDBG("ignoring read position - assuming %f is more true [%s]", currentExpectedPosition(false), qPrintable(tline));
                return;
            }
        }
        else {
            m_dont_trust_time_from_statusline_till = QDateTime();
        }
    }

    streamPositionReadAt(parsedpos, readtime);

}

double MpProcess::lastPositionRead() const
{
    if(m_stopped_because_of_long_seek) {
        double len = mediainfo_length();

        if(len > 0) {
            return len;
        }
    }

    return m_lastread_streamPosition;
}

double MpProcess::expectedPositionAt(const QDateTime &now, bool maskinbadstates) const
{

    if(maskinbadstates) {
        switch(m_curr_state) {
            case(MpState::ErrorState):
            case(MpState::IdleState):
            case(MpState::NotStartedState):
            case(MpState::StoppedState):
                return (-1);
                break;

            default:
                break;
        }
    }

    double lrp = m_lastread_streamPosition;

    if(lrp < 0) {
        return lrp;
    }

    if(m_curr_state == MpState::PlayingState && !now.isNull() && !m_streamposition_readt.isNull()) {
        //TIMEMYDBG("streamPosition()");
        const qint64 offset_ms = m_streamposition_readt.msecsTo(now);

        if(offset_ms != 0) {
            if(offset_ms < 0) {
                PROGRAMMERERROR("WTF");
            }

            const double offset_secs = double(offset_ms) / 1000.0;

            //TIMEMYDBG("streamPosition(): adding %lu*%f msec to last marker %f", (unsigned long)offset_ms, m_speed, lrp);
            if(m_curr_speed < 0.00001 || m_curr_speed > 100) {
                PROGRAMMERERROR("WTF");
            }

            if(offset_secs > 1.) {
                qWarning("adjusting position from %f: no position read in %f seconds? thats a lot...", lrp, offset_secs);
            }

            const double change = m_curr_speed * offset_secs;
            lrp += change;
        }
    }

    if(m_mediaInfo->has_length()) {
        const double len = m_mediaInfo->length();

        if(len > 0) {
            if(len < lrp) {
                double diff = lrp - len;

                if(diff < max_overflow_position_to_length_sec) {
                    return len;
                }

                PROGRAMMERERROR("current stream position %f much larger than media %f", lrp, len);
            }
        }
    }

    return lrp;
}
double MpProcess::currentExpectedPosition(bool maskinbadstates) const
{

    if(m_curr_state == MpState::PlayingState) {
        QDateTime now = QDateTime::currentDateTimeUtc();
        return expectedPositionAt(now, maskinbadstates);
    }
    else {
        return expectedPositionAt(QDateTime(), maskinbadstates);
    }
}

void MpProcess::streamPositionReadAt(double newpos, const QDateTime &now)
{

    if(newpos < 0) {
        PROGRAMMERERROR("WTF");
    }

    if(newpos > 60 * 60 * 24 * 10) {
        PROGRAMMERERROR("WTF");
    }

    double expectedpos = currentExpectedPosition(false);
    double diff_to_expected = qAbs(expectedpos - newpos);
    double diff_to_last = qAbs(m_last_emited_streampos - newpos);

    m_lastread_streamPosition = newpos;
    m_streamposition_readt = now;

    if(diff_to_expected > emit_position_change_if_change_larger_than_sec || m_last_emited_streampos == (-1) || diff_to_last > emit_position_change_if_change_larger_than_sec) {
        TIMEMYDBG("setStreamPosition: emiting new position %f, old position %f", newpos, expectedpos);
        m_last_emited_streampos = m_lastread_streamPosition;
        emit sig_streamPositionChanged(m_lastread_streamPosition);
    }
    else {
        TIMEMYDBG("setStreamPosition: new position %f, old position %f (not emiting)", newpos, expectedpos);
    }

}

// Changes the current state, possibly emitting multiple signals
void MpProcess::changeState(MpState newstate, const QDateTime &now)
{

    const MpState oldstate = m_curr_state;

    if(newstate == MpState::ErrorState) {
        PROGRAMMERERROR("Please call changeToErrorState() instead");
    }

    if(oldstate == newstate) {
        return;
    }

    MYDBG("state changed from %s to %s", convert_MpState_2_asciidesc(oldstate), convert_MpState_2_asciidesc(newstate));

    if(oldstate == MpState::LoadingState && newstate == MpState::PlayingState) {
        MYDBG("now playing, after %lu msec of loading", (unsigned long) m_loadingtimer.elapsed());
    }

    // reset read timer - we might go from idle to loading,
    // with nothing happening for a long time before this
    m_lastreadt = now;
    TIMEMYDBG("changeState(%s): m_lastreadt set to now", convert_MpState_2_asciidesc(newstate));

    m_curr_state = newstate;

    set_screensaver_by_state();

    make_heartbeat_active_or_not();

    if(newstate == MpState::NotStartedState) {
        MYDBG("newstate is NotStartedState - resetvalues()");
        resetValues();
    }

    if(oldstate == MpState::PlayingState || newstate == MpState::PlayingState) {
        MYDBG("oldstate or newstate is PlayingState - setting m_streamposition_readt to null");
        m_streamposition_readt = QDateTime();
    }

    emit sig_stateChanged(oldstate, newstate);

}

// Changes the current state, possibly emitting multiple signals
void MpProcess::changeToErrorState(const QString &comment, const QDateTime &now)
{
    const MpState oldstate = m_curr_state;
    const MpState newstate = MpState::ErrorState;

    if(oldstate == newstate) {
        return;
    }

    MYDBG("state changed from %s to %s: %s", convert_MpState_2_asciidesc(oldstate), convert_MpState_2_asciidesc(newstate), qPrintable(comment));

    // reset read timer - we might go from idle to loading,
    // with nothing happening for a long time before this
    m_lastreadt = now;
    TIMEMYDBG("changeToErrorState: m_lastreadt set to now");

    m_curr_state = newstate;
    set_screensaver_by_state();
    make_heartbeat_active_or_not();
    emit sig_stateChanged(oldstate, newstate);

    emit sig_error(comment);
    emit sig_error_at_pos(comment, m_lastread_streamPosition);
    resetValues();

}

void MpProcess::update_lastreadt(const QDateTime &readtime)
{
    if(!m_lastreadt.isNull()) {
        qint64 msecs = m_lastreadt.msecsTo(readtime);

        if(msecs < 0) {
            qDebug("last read at %s", qPrintable(m_lastreadt.toLocalTime().toString(QLatin1String("hh:mm:ss.zzz"))));
            qDebug("readtime %s", qPrintable(readtime.toLocalTime().toString(QLatin1String("hh:mm:ss.zzz"))));
            PROGRAMMERERROR("WTF");
        }

        const unsigned stateidx = MpState_2_idx(m_curr_state);

        if(m_max_encountered_readlatency_ms[stateidx] < 0 || msecs > m_max_encountered_readlatency_ms[stateidx]) {
            m_max_encountered_readlatency_ms[stateidx] = msecs;
        }

        if((!m_lastwritet.isNull()) && m_lastwritet > m_lastreadt) {
            qint64 msecs_since_write = m_lastwritet.msecsTo(readtime);

            if(msecs_since_write < 0) {
                qDebug("last write at %s", qPrintable(m_lastwritet.toLocalTime().toString(QLatin1String("hh:mm:ss.zzz"))));
                qDebug("readtime %s", qPrintable(readtime.toLocalTime().toString(QLatin1String("hh:mm:ss.zzz"))));
                PROGRAMMERERROR("WTF");
            }

            if(m_max_encountered_writelatency_ms < 0 || msecs_since_write > m_max_encountered_writelatency_ms) {
                m_max_encountered_writelatency_ms = msecs_since_write;
            }
        }
    }

    m_lastreadt = readtime;
    // const QDateTime&now=readtime;
    //TIMEMYDBG("update_lastreadt: m_lastreadt set to readtime");
}

bool MpProcess::heartbeat_should_be_active() const
{
    if(m_curr_state == MpState::LoadingState) {
        return true;
    }

    if(m_curr_state == MpState::PlayingState) {
        return true;
    }

    if(m_curr_state == MpState::BufferingState) {
        return true;
    }

    if(!m_outputq.isEmpty()) {
        return true;
    }

    return false;
}

void MpProcess::make_heartbeat_active_or_not()
{
    const bool should = heartbeat_should_be_active();
    const bool curr = m_heartbeattimer.isActive();

    if(should == curr) {
        return;
    }

#ifdef DEBUG_MPP_TIME
    QDateTime now = QDateTime::currentDateTimeUtc();
#endif

    if(should) {
        MYDBG("starting heartbeat timer");
        m_heartbeattimer.start();
    }
    else {
        MYDBG("stopping heartbeat timer");
        m_heartbeattimer.stop();
    }
}


void MpProcess::slot_heartbeat()
{

    /*
     this should be active if any of these conditions are met:
      * LoadingState,    PlayingState,    BufferingState,
      * write queue is not empty
    */

    const QDateTime now = QDateTime::currentDateTimeUtc();
    TIMEMYDBG("heartbeat");

    try_to_write(now);

    if(m_lastreadt.isNull()) {
        return;
    }

    if(m_lastwritet.isNull()) {
        return;
    }

    const qint64 readmsecs = m_lastreadt.msecsTo(now);

    if(readmsecs < 0) {
        PROGRAMMERERROR("WTF");
    }

    const unsigned sidx = MpState_2_idx(m_curr_state);
    const qint64 max_readlatency = expect_read_every_ms[sidx];

    if(max_readlatency > 0) {
        if(max_readlatency * 10 < readmsecs) {
            char const *desc = convert_MpStateidx_2_asciidesc(sidx);
            QString msg = QString(QLatin1String("%1: found read latency of %2 ms")).arg(QLatin1String(desc)).arg(readmsecs);
            changeToErrorState(msg, now);
        }
        else if(max_readlatency < readmsecs) {
            char const *desc = convert_MpStateidx_2_asciidesc(sidx);
            qWarning("%s: found read latency of %lu ms", desc, (unsigned long)readmsecs);
        }
    }

    if(m_lastwritet > m_lastreadt) {
        const qint64 writemsecs = m_lastwritet.msecsTo(now);

        if(writemsecs < 0) {
            PROGRAMMERERROR("WTF");
        }

        if(expect_read_after_write_ms * 10 < writemsecs) {
            char const *desc = convert_MpStateidx_2_asciidesc(sidx);
            QString msg = QString(QLatin1String("%1: found write latency of %2 ms")).arg(QLatin1String(desc)).arg(writemsecs);
            changeToErrorState(msg, now);
        }
        else if(expect_read_after_write_ms < writemsecs) {
            char const *desc = convert_MpStateidx_2_asciidesc(sidx);
            qWarning("%s: found write latency of %lu ms", desc, (unsigned long)writemsecs);
        }
    }
}
// -1 - can not compute
// -2 - after end
// 0 - start
// else - that position
double MpProcess::compute_seektarget_for_seek(double target, SeekMode smode) const
{
    const double len = mediainfo_length();
    double newpos;

    switch(smode) {
        case RelativeSeek:
            newpos = currentExpectedPosition(false) + target;
            break;

        case PercentageSeek:
            if(len <= 0) {
                return -1.;
            }

            newpos = target * 100.0 / len;
            break;

        case AbsoluteSeek:
            newpos = target;
            break;

        default:
            return -1.;
    }

    newpos = round(newpos);

    if(newpos <= 0.) {
        return 0.;
    }

    if(len > 0 && newpos > len) {
        return -2.;
        //return len - 2.;
    }

    return newpos;
}

QString MpProcess::MpProcessCmd::command(MpProcess &proc) const
{
    if(m_type == String) {
        return m_command;
    }

    else if(m_type == Seek) {
        const double oldpos = proc.currentExpectedPosition(false);
        const double newpos = proc.compute_seektarget_for_seek(m_seektarget, m_seekmode);

        if(newpos < -1.5) {
#ifdef DEBUG_MPP_TIME
            const QDateTime now = QDateTime::currentDateTimeUtc();
            const double m_lastread_streamPosition = proc.m_lastread_streamPosition;
#endif
            MYDBG("seeking past end, will stop() now");

            proc.m_stopped_because_of_long_seek = true;

            QTimer::singleShot(0, &proc, SLOT(slot_stop()));
            //proc.stop();
            return QString();
        }

        if(newpos < -0.5) {
            return QString();
        }

        const QDateTime now = QDateTime::currentDateTimeUtc();
#ifdef DEBUG_MPP_TIME
        const double m_lastread_streamPosition = proc.m_lastread_streamPosition;
#endif
        TIMEMYDBG("MpProcessCmd::command(seek)");
        QString cmd = QString(QLatin1String("seek %1 2")).arg(newpos);
        proc.streamPositionReadAt(newpos, now);
        proc.m_dont_trust_time_from_statusline_till = now.addMSecs(ignore_statusline_after_seek_ms);
        proc.m_previous_seek_direction_is_fwd = (newpos > oldpos ? true : false);
        return cmd;
    }
    else if(m_type == OSDShowLocation) {

        //double pos = proc.streamPosition();

        //if(pos < 0) {
        //    pos = 0;
        //}

        double len = proc.mediainfo_length();

        if(len <= 0) {
            len = 0;
        }

        //QString spos = tlenpos_2_human(pos);
        QString slen = tlenpos_2_human(len);

        //QString str = spos + QLatin1String(" / ") + slen;
        //QString f = QLatin1String("pausing_keep osd_show_text \"%1\" %2 %3");
        QString str = QLatin1String("${time_pos} ") + QLatin1String(" / ") + slen;
        QString f = QLatin1String("pausing_keep osd_show_property_text \"%1\" %2 %3");
        f = f.arg(str).arg(osd_default_duration_ms).arg(0);
        return f;
    }
    else if(m_type == Delay) {
        return QString();
    }
    else {
        PROGRAMMERERROR("WTF");
        return QString();
    }
}

void MpProcess::set_assume_aid(int in_aid)
{
    const QString alang = m_mediaInfo->aid_2_alang(in_aid);
#ifdef DEBUG_MPP_TIME
    const QDateTime now = QDateTime::currentDateTimeUtc();
#endif
    MYDBG("assuming AID will be %d in short order \"%s\"", in_aid, qPrintable(alang));
    m_current_aid = in_aid;
}

static int cycle_alang(const int highest, const int curr)
{
    if(curr < 0) {
        return 0;
    }

    if(curr >= highest) {
        return (-2);
    }

    return curr + 1;
}

int MpProcess::set_assume_next_aid()
{

    const int haid = m_mediaInfo->highest_aid();
    const int next = cycle_alang(haid, m_current_aid);

    const QString alang = m_mediaInfo->aid_2_alang(next);
#ifdef DEBUG_MPP_TIME
    const QDateTime now = QDateTime::currentDateTimeUtc();
#endif
    MYDBG("assuming AID will cycle to %d in short order \"%s\"", next, qPrintable(alang));
    m_current_aid = next;
    return next;
}


// return -11 if impossible
int MpProcess::find_next_alang_not_in(const QSet<QString> &forbidden) const
{

    const int haid = m_mediaInfo->highest_aid();
    int next = cycle_alang(haid, m_current_aid);

    if(forbidden.isEmpty()) {
        return next;
    }

    while(next != m_current_aid) {
        const QString lang = m_mediaInfo->aid_2_alang(next);

        if(!forbidden.contains(lang)) {
#ifdef DEBUG_MPP_TIME
            const QDateTime now = QDateTime::currentDateTimeUtc();
#endif
            MYDBG("going from AID %d (%s) to %d (%s): is not one of %s", m_current_aid, qPrintable(m_mediaInfo->aid_2_alang(m_current_aid)), next, qPrintable(m_mediaInfo->aid_2_alang(next)), qPrintable(QStringList(forbidden.toList()).join(QLatin1String(","))));
            return next;
        }

        next = cycle_alang(haid, next);
    }

#ifdef DEBUG_MPP_TIME
    const QDateTime now = QDateTime::currentDateTimeUtc();
#endif
    MYDBG("can not go anywhere from AID %d (%s): all are in %s", m_current_aid, qPrintable(m_mediaInfo->aid_2_alang(m_current_aid)), qPrintable(QStringList(forbidden.toList()).join(QLatin1String(","))));
    return -11;

}

bool MpProcess::screensaver_should_be_active() const
{
    if(m_curr_state == MpState::PlayingState || m_curr_state == MpState::LoadingState || m_curr_state == MpState::BufferingState) {
        return false;
    }
    else {
        return true;
    }
}

void MpProcess::set_screensaver_by_state()
{
    if(screensaver_should_be_active()) {
        m_ssmanager.enable();
    }
    else {
        m_ssmanager.disable();
    }

}

