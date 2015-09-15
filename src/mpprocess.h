#ifndef MPPROCESS_H
#define MPPROCESS_H

#include <QSize>
#include <QHash>
#include <QString>
#include <QTimer>
#include <QFlags>
#include <QDateTime>
#include <QElapsedTimer>
#include <QQueue>
#include <QProcess>

#include "util.h"
#include "mpmediainfo.h"
#include "mpstate.h"
#include "vregexp.h"

const unsigned osd_default_duration_ms = 2000;

#include "screensavermanager.h"

// A custom QProcess designed for the MPlayer slave interface
class MpProcess : public QObject
{
    Q_OBJECT

public:
    enum IOChannel {
        Input = 1,
        Output = 2,
        Error = 4
    };
    Q_DECLARE_FLAGS(IOChannels, IOChannel)

    enum SeekMode {
        RelativeSeek,
        PercentageSeek,
        AbsoluteSeek
    };
    struct MpProcessIolog {
        QString line;
        IOChannel c;
        QDateTime dt;
    };

private:
    enum MpCommandType {
        String,
        Seek,
        OSDShowLocation,
        Delay
    };

    class mpctag_string
    {
    };
    class mpctag_seek
    {
    };
    class mpctag_osdsl
    {
    };
    class mpctag_delay
    {
    };

    class MpProcessCmd
    {
    private:
        MpCommandType m_type;
        QString m_command;
        SeekMode m_seekmode;
        double m_seektarget;
        double m_delay;
        QDateTime m_created;
        MpProcessCmd();
    public:
        MpProcessCmd(mpctag_string /*tag*/, const QString &in_cmd, const QDateTime &in_dt):
            m_type(String),
            m_command(in_cmd),
            m_created(in_dt) {}
        MpProcessCmd(mpctag_seek /*tag*/, const SeekMode &in_smode, double in_starget, const QDateTime &in_dt):
            m_type(Seek),
            m_seekmode(in_smode),
            m_seektarget(in_starget),
            m_created(in_dt) {}
        MpProcessCmd(mpctag_osdsl /*tag*/, const QDateTime &in_dt):
            m_type(OSDShowLocation),
            m_created(in_dt) {}
        MpProcessCmd(mpctag_delay /*tag*/, const double delay, const QDateTime &in_dt):
            m_type(Delay),
            m_delay(delay),
            m_created(in_dt) {}
        QString command(MpProcess &proc) const;
        const QDateTime &created() const
        {
            return m_created;
        }
        MpCommandType type() const
        {
            return m_type;
        }
        double delay() const
        {
            if(m_type != Delay) {
                PROGRAMMERERROR("WTF");
            }

            return m_delay;
        }
        double seektarget(MpProcess &proc) const
        {
            if(m_type != Seek) {
                PROGRAMMERERROR("WTF");
            }

            return proc.compute_seektarget_for_seek(m_seektarget, m_seekmode);
        }
    };

    QProcess *m_proc;
    MpState m_curr_state;

    QString m_cfg_mplayerPath;
    QString m_cfg_videoOutput;

    MpMediaInfo *m_mediaInfo;
    QDateTime m_streamposition_readt; // this is the time when the streamposition was read
    QElapsedTimer m_loadingtimer;

    double m_lastread_streamPosition; // This is the video position
    double m_last_emited_streampos;
    double m_curr_speed;
    // Looks like this should really be a QElapsedTimer, but there are two problems:
    // * we use two different timeout values:
    //    - ignore_statusline_after_seek_ms after a seek
    //    - 300 after a load()
    // * parsePosition() is not running relative to "now" but relative to the stored readtime
    QDateTime m_dont_trust_time_from_statusline_till;
    bool m_previous_seek_direction_is_fwd;
    bool m_currently_muted;

    bool m_stopped_because_of_long_seek;

    QString m_currentTag;

    QStringList m_saved_mplayer_args;

    QQueue<MpProcessCmd> m_outputq;

    bool m_cfg_currently_parsing_mplayer_text;
    IOChannels m_cfg_output_accumulator_mode;
    size_t m_cfg_acc_maxlines;
    VRegExp *m_cfg_output_accumulator_ignore;
    QList<MpProcessIolog> m_acc;

    QDateTime m_lastreadt;
    QDateTime m_lastwritet;
    qint64 m_max_encountered_readlatency_ms[MpState_maxidx];
    qint64 m_max_encountered_writelatency_ms;
    qint64 m_max_encountered_writequeuelatency_ms;
    QTimer m_heartbeattimer;

    QByteArray m_not_yet_processed_out;
    QByteArray m_not_yet_processed_err;

    int m_current_aid;

    ScreenSaverManager m_ssmanager;

public:

    MpState state() const
    {
        return m_curr_state;
    }
    Q_PID pid() const
    {
        if(m_proc == NULL) {
            return -1;
        }

        return m_proc->pid();
    }
    const QString &videoOutput() const
    {
        return m_cfg_videoOutput;
    }
    void setVideoOutput(const QString &vo)
    {
        if(vo.isEmpty()) {
            qWarning("setVideoOutput(): given empty string??");
        }

        m_cfg_videoOutput = vo;
    }
    const QString &mplayerPath() const
    {
        return m_cfg_mplayerPath;
    }
    void setMplayerPath(const QString &mp)
    {
        m_cfg_mplayerPath = mp;
    }
    const QStringList &mplayer_args() const
    {
        return m_saved_mplayer_args;
    }
    void set_accumulated_output_maxlines(size_t in)
    {
        m_cfg_acc_maxlines = in;

        while(size_t(m_acc.size()) > m_cfg_acc_maxlines) {
            m_acc.removeFirst();
        }
    }
    const QList<MpProcessIolog> &accumulated_output() const
    {
        return m_acc;
    }
    void set_output_accumulator_mode(IOChannels in)
    {
        m_cfg_output_accumulator_mode = in;
    }
    void set_output_accumulator_ignore(const VRegExp &in)
    {
        if(m_cfg_output_accumulator_ignore != NULL) {
            delete m_cfg_output_accumulator_ignore;
            m_cfg_output_accumulator_ignore = NULL;
        }

        m_cfg_output_accumulator_ignore = new VRegExp(in);
    }
    void set_output_accumulator_ignore(char const *const in)
    {
        if(m_cfg_output_accumulator_ignore != NULL) {
            delete m_cfg_output_accumulator_ignore;
            m_cfg_output_accumulator_ignore = NULL;
        }

        if(in != NULL && in[0] != '\0') {
            m_cfg_output_accumulator_ignore = new VRegExp(in);
        }
    }
    void set_output_accumulator_ignore_default();
    QProcess::ProcessState processState() const
    {
        return m_proc->state();
    }

    explicit MpProcess(QObject *parent, MpMediaInfo *mip);
    virtual ~MpProcess();

    double lastPositionRead() const;
    double currentExpectedPosition(bool maskinbadstates) const;
    double expectedPositionAt(const QDateTime &now, bool maskinbadstates) const;

    void set_assume_aid(int aid);
    int set_assume_next_aid();
    int find_next_alang_not_in(const QSet<QString> &forbidden) const;

    // Starts the MPlayer process in idle mode
    void start_process(int xwinid, const QStringList &args);
    // kills the mplayer process
    void quit();
    bool screensaver_should_be_active() const;

public slots:


    // loads a file and starts to play. a slot so it can be called delayed
    void slot_load(const QString &url);
    // slot so it can be run delayed
    void slot_pause();
    void slot_play();
    // stops playback
    void slot_stop();
    // a version with int offset as targets for the seekslider
    void slot_seek_from_slider(int position);
    void slot_rel_seek_from_keyboard(double offset);
    void slot_abs_seek_from_keyboard(double position);
    // currently unused
    void slot_osd_show_location();

    void slot_submit_write(const MpProcessCmd &mpc, const QDateTime &submittime);
    void slot_submit_write(const QString &command);
    void slot_submit_write_latin1(char const *const latin1lit);

    void slot_speed_up();
    void slot_slow_down();

    void slot_mute();
    void slot_unmute();

    // internal

    // from the QProcess
    void slot_readStdout();
    void slot_readStderr();
    void slot_finished(int, QProcess::ExitStatus);
    void slot_error_received(QProcess::ProcessError);
    // delay
    void slot_ask_for_metadata();
    // timer - started when beginning to load
    void slot_quit_if_we_are_not_playing_yet();
    void slot_heartbeat();
    void slot_try_to_write_now();

signals:

    void sig_stateChanged(MpState oldstate, MpState newstate);
    void sig_streamPositionChanged(double position);
    void sig_error(const QString &reason);
    void sig_error_at_pos(const QString &reason, double lastpos);
    void sig_seekedTo(double position);
    void sig_loadDone();

private:

    bool allowed_to_write_now(const QDateTime &now, char const **reason);
    void write_one_cmd(const QDateTime &now, const char *reason);
    void try_to_write(const QDateTime &now);
    void force_cmd(const QString &cmd);

    void core_seek(double offset, MpProcess::SeekMode whence);

    // Parses a line of MPlayer output
    void parseLine(const QString &line, const QDateTime &readtime, QStringList &positionlines, QList<MpState> &newstates, QStringList &errorreasons, QList<double> &foundspeeds);
    // Parses MPlayer's media identification output
    void parseMediaInfo(const QString &line, const QDateTime &readtime);
    // Parses MPlayer's position output
    void parsePosition(const QString &line, const QDateTime &readtime);
    // Changes the current state, possibly emitting multiple signals
    void changeState(MpState state, const QDateTime &now);
    // Changes the current state, possibly emitting multiple signals
    void changeToErrorState(const QString &comment, const QDateTime &now);
    // Resets the media info and position values
    void resetValues();

    void proc_incremental_std(const QByteArray &bytes, IOChannel c);
    void clear_out_incremental_stdouterr();
    void clear_out_iobuffer_before_load();
    void update_lastreadt(const QDateTime &readtime);
    bool heartbeat_should_be_active() const;
    void make_heartbeat_active_or_not();
    void streamPositionReadAt(double newpos, const QDateTime &now);
    double compute_seektarget_for_seek(double target, SeekMode smode) const;
    double mediainfo_length() const
    {
        if(!m_mediaInfo->has_length()) {
            return (-1);
        }

        return m_mediaInfo->length();
    }
    void foundReadSpeed(double rspeed);
    void dbg_out(char const *pref, const QString &line, const QDateTime &now);
    void set_screensaver_by_state();


};

Q_DECLARE_OPERATORS_FOR_FLAGS(MpProcess::IOChannels)
Q_DECLARE_METATYPE(MpProcess::SeekMode)
Q_DECLARE_METATYPE(MpState)


#endif // MPPROCESS_H
