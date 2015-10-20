#ifndef MPMEDIAINFO_H
#define MPMEDIAINFO_H

#include <QSize>
#include <QHash>
#include <QString>
#include <QRect>

#include "util.h"
#include "checkedget.h"

class MpMediaInfo
{
private:
    QString m_desc;

    QHash<QString, QString> m_tags;

    QHash<int, QString> m_id2alangs;
    QHash<int, QString> m_id2slangs;
    // reverse of above, but only first instance of each language
    QHash<QString, int> m_alang2id;
    QHash<QString, int> m_slang2id;

    CheckedGet<QString> mc_videoFormat;
    CheckedGet<int> mc_videoBitrate;
    CheckedGet<int> mc_width;
    CheckedGet<int> mc_height;
    CheckedGet<double> mc_DAR;
    CheckedGet<double> mc_PAR;
    CheckedGet<double> mc_framesPerSecond;

    // mplayer outputs quite a mess for multiple tracks
    // so far could not figure out how ID_AUDIO output
    // should be parsed
    // It does look like it outputs ID_AUDIO* when we switch to a given track
    CheckedGet<QString> mc_audioFormat;
    CheckedGet<double> mc_audioBitrate;
    CheckedGet<int> mc_sampleRate;
    CheckedGet<int> mc_numChannels;

    bool m_finalized;
    CheckedGet<double> mc_length;
    CheckedGet<bool> mc_seekable;
    CheckedGet<QRect> mc_crop;
    CheckedGetDefault<Interlaced_t, Interlaced_t::PIUnknown> mc_interlaced;

    void assert_is_finalized() const
    {
        if(!m_finalized) {
            PROGRAMMERERROR("trying to access noninitialized MpMediaInfo");
        }
    }
    void assert_is_not_finalized() const
    {
        if(m_finalized) {
            PROGRAMMERERROR("trying to modify initialized MpMediaInfo");
        }
    }


public:

    explicit MpMediaInfo()
    {
        clear();
    }

    explicit MpMediaInfo(const QString &in_desc)
    {
        clear();
        m_desc = in_desc;

        if(m_desc.isEmpty()) {
            PROGRAMMERERROR("MpMediaInfo with empty description");
        }
    }
    MpMediaInfo(const MpMediaInfo &in):
        m_desc(in.m_desc),
        m_tags(in.m_tags),
        m_id2alangs(in.m_id2alangs),
        m_id2slangs(in.m_id2slangs),
        m_alang2id(in.m_alang2id),
        m_slang2id(in.m_slang2id),
        mc_videoFormat(in.mc_videoFormat),
        mc_videoBitrate(in.mc_videoBitrate),
        mc_width(in.mc_width),
        mc_height(in.mc_height),
        mc_DAR(in.mc_DAR),
        mc_PAR(in.mc_PAR),
        mc_framesPerSecond(in.mc_framesPerSecond),
        mc_audioFormat(in.mc_audioFormat),
        mc_audioBitrate(in.mc_audioBitrate),
        mc_sampleRate(in.mc_sampleRate),
        mc_numChannels(in.mc_numChannels),
        m_finalized(in.m_finalized),
        mc_length(in.mc_length),
        mc_seekable(in.mc_seekable),
        mc_crop(in.mc_crop),
        mc_interlaced(in.mc_interlaced)
    {
    }
    void assign(const MpMediaInfo &in)
    {
        m_desc = in.m_desc;
        m_tags = in.m_tags;
        m_id2alangs = in.m_id2alangs;
        m_id2slangs = in.m_id2slangs;
        m_alang2id = in.m_alang2id;
        m_slang2id = in.m_slang2id;
        mc_videoFormat = in.mc_videoFormat;
        mc_videoBitrate = in.mc_videoBitrate;
        mc_width = in.mc_width;
        mc_height = in.mc_height;
        mc_DAR = in.mc_DAR;
        mc_PAR = in.mc_PAR;
        mc_framesPerSecond = in.mc_framesPerSecond;
        mc_audioFormat = in.mc_audioFormat;
        mc_audioBitrate = in.mc_audioBitrate;
        mc_sampleRate = in.mc_sampleRate;
        mc_numChannels = in.mc_numChannels;
        m_finalized = in.m_finalized;
        mc_length = in.mc_length;
        mc_seekable = in.mc_seekable;
        mc_crop = in.mc_crop;
        mc_interlaced = in.mc_interlaced;
    }
    MpMediaInfo &operator=(const MpMediaInfo &in)
    {
        m_desc = in.m_desc;
        m_tags = in.m_tags;
        m_id2alangs = in.m_id2alangs;
        m_id2slangs = in.m_id2slangs;
        m_alang2id = in.m_alang2id;
        m_slang2id = in.m_slang2id;
        mc_videoFormat = in.mc_videoFormat;
        mc_videoBitrate = in.mc_videoBitrate;
        mc_width = in.mc_width;
        mc_height = in.mc_height;
        mc_DAR = in.mc_DAR;
        mc_PAR = in.mc_PAR;
        mc_framesPerSecond = in.mc_framesPerSecond;
        mc_audioFormat = in.mc_audioFormat;
        mc_audioBitrate = in.mc_audioBitrate;
        mc_sampleRate = in.mc_sampleRate;
        mc_numChannels = in.mc_numChannels;
        m_finalized = in.m_finalized;
        mc_length = in.mc_length;
        mc_seekable = in.mc_seekable;
        mc_crop = in.mc_crop;
        mc_interlaced = in.mc_interlaced;
        return *this;
    }
    void clear()
    {
        m_tags.clear();
        m_id2alangs.clear();
        m_id2slangs.clear();
        m_alang2id.clear();
        m_slang2id.clear();
        mc_videoFormat.clear();
        mc_videoBitrate.clear();
        mc_width.clear();
        mc_height.clear();
        mc_DAR.clear();
        mc_PAR.clear();
        mc_framesPerSecond.clear();
        mc_audioFormat.clear();
        mc_audioBitrate.clear();
        mc_sampleRate.clear();
        mc_numChannels.clear();
        m_finalized = false;
        mc_length.clear();
        mc_seekable.clear();
        mc_crop.clear();
        mc_interlaced.clear();
    }
    void set_finalized()
    {
        assert_is_not_finalized();
        mc_width.seal();
        mc_height.seal();
        mc_videoFormat.seal();
        mc_videoBitrate.seal();
        mc_DAR.seal();
        mc_PAR.seal();
        mc_framesPerSecond.seal();
        mc_audioFormat.seal();
        mc_audioBitrate.seal();
        mc_sampleRate.seal();
        mc_numChannels.seal();
        mc_length.seal();
        mc_seekable.seal();
        mc_crop.seal();
        mc_interlaced.seal();
        m_finalized = true;
    }
    void make_unfinalized()
    {
        mc_width.unseal();
        mc_height.unseal();
        mc_videoFormat.unseal();
        mc_videoBitrate.unseal();
        mc_DAR.unseal();
        mc_PAR.unseal();
        mc_framesPerSecond.unseal();
        mc_audioFormat.unseal();
        mc_audioBitrate.unseal();
        mc_sampleRate.unseal();
        mc_numChannels.unseal();
        mc_length.unseal();
        mc_seekable.unseal();
        mc_crop.unseal();
        mc_interlaced.unseal();
        m_finalized = false;
    }

    bool is_finalized() const
    {
        return m_finalized;
    }
    void add_tag(const QString &k, const QString &v)
    {
        assert_is_not_finalized();
        m_tags.insert(k, v);
    }
    void set_videoFormat(const QString &f)
    {
        mc_videoFormat.set(f);
    }
    void set_videoBitrate(int b)
    {
        mc_videoBitrate.set(b);
    }
    void set_size(const QSize &s)
    {
        mc_width.set(s.width());
        mc_height.set(s.height());
    }
    void set_size(int w, int h)
    {
        mc_width.set(w);
        mc_height.set(h);
    }
    bool has_size() const
    {
        return mc_width.isset() && mc_height.isset();
    }
    QSize size() const
    {
        const int w = mc_width.get();
        const int h = mc_height.get();
        QSize ret(w, h);
        return ret;
    }
    void set_width(int w)
    {
        mc_width.set(w);
    }
    void set_height(int h)
    {
        mc_height.set(h);
    }

    int width() const
    {
        return mc_width.get();
    }
    int height() const
    {
        return mc_height.get();
    }
    double length() const
    {
        return mc_length.get();
    }
    bool has_length() const
    {
        return mc_length.isset();
    }
    void set_length(double d)
    {
        mc_length.set(d);
    }

    bool is_seekable() const
    {
        return mc_seekable.get();
    }
    void set_seekable(bool b)
    {
        mc_seekable.set(b);
    }

    void set_framesPerSecond(double d)
    {
        mc_framesPerSecond.set(d);
    }
    void set_audioFormat(const QString &s)
    {
        mc_audioFormat.set(s);
    }
    void set_audioBitrate(double i)
    {
        mc_audioBitrate.set(i);
    }
    void set_sampleRate(int i)
    {
        mc_sampleRate.set(i);
    }
    void set_numChannels(int i)
    {
        mc_numChannels.set(i);
    }

    void set_crop(const QString &str);
    void set_crop(const QRect &rect);

    void set_crop(size_t w, size_t h, size_t x, size_t y)
    {
        QRect r(x, y, w, h);
        set_crop(r);
    }

    size_t getCropLeft() const
    {
        if(!mc_crop.isset()) {
            return 0;
        }

        return mc_crop.get().left();
    }
    size_t getCropTop() const
    {
        if(!mc_crop.isset()) {
            return 0;
        }

        return mc_crop.get().top();
    }
    size_t getCropRight() const
    {
        if(!mc_width.isset()) {
            return 0;
        }

        if(!mc_crop.isset()) {
            return 0;
        }

        const int w = width();

        if(w == 0) {
            return 0;
        }

        return w - getCropLeft() - getCroppedWidth();
    }
    size_t getCropBottom() const
    {
        if(!mc_height.isset()) {
            return 0;
        }

        if(!mc_crop.isset()) {
            return 0;
        }

        const int h = height();

        if(h == 0) {
            return 0;
        }

        return h - getCropTop() - getCroppedHeight();
    }
    size_t getCroppedWidth() const
    {
        if(!mc_crop.isset()) {
            return width();
        }

        const int w = mc_crop.get().width();

        if(w == 0) {
            return width();
        }

        return w;
    }
    size_t getCroppedHeight() const
    {
        if(!mc_crop.isset()) {
            return height();
        }

        const int h = mc_crop.get().height();

        if(h == 0) {
            return height();
        }

        return h;
    }
    void set_interlaced(Interlaced_t i)
    {
        mc_interlaced.set(i);
    }
    Interlaced_t is_interlaced() const
    {
        return mc_interlaced.get();
    }


    // DAR = w/h*PAR
    // PAR = DAR*h/w

    void set_DAR(double D);
    void set_PAR(double P);
    bool has_AR() const
    {
        return (mc_PAR.isset() || mc_DAR.isset());
    }
    double PAR() const;
    double DAR() const;

    QSize displaySize() const
    {
        QSize ret = size();
        const double ppar = PAR();
        const int w = ret.width();
        ret.setWidth(double(w) * ppar);
        return ret;
    }

    void add_alang(int id, const QString &alang)
    {
        assert_is_not_finalized();
        m_id2alangs[id] = alang;

        if(!m_alang2id.contains(alang)) {
            m_alang2id[alang] = id;
        }
    }
    void add_slang(int id, const QString &slang)
    {
        assert_is_not_finalized();
        m_id2slangs[id] = slang;

        if(!m_slang2id.contains(slang)) {
            m_slang2id[slang] = id;
        }
    }

    int alang_2_aid(const QString &alang) const
    {
        assert_is_finalized();

        if(m_alang2id.contains(alang)) {
            return m_alang2id[alang];
        }

        return (-1);
    }
    QString aid_2_alang(const int aid) const
    {
        if(aid < 0) {
            return QStringLiteral("MUTE");
        }

        if(!m_id2alangs.contains(aid)) {
            return QStringLiteral("UNKNOWN");
        }

        return m_id2alangs[aid];
    }
    int highest_aid() const
    {
        QList<int> all_aids = m_id2alangs.keys();
        qSort(all_aids);
        const int last = all_aids.last();
        return last;
    }
    int slang_2_sid(const QString &slang) const
    {
        assert_is_finalized();

        if(m_slang2id.contains(slang)) {
            return m_slang2id[slang];
        }

        return (-1);
    }

};

Q_DECLARE_METATYPE(MpMediaInfo)

#endif // MPMEDIAINFO_H

