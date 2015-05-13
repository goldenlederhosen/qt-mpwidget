#ifndef CHECKEDGET_H
#define CHECKEDGET_H

#include <QDebug>

// get() fails if not set before
// set() fails if seal() has been called
template <typename T> class CheckedGet {
private:
    bool m_isset;
#ifdef CAUTION
    bool m_sealed;
#endif
    T m_val;
public:
    // needs to be same as clear()
    explicit CheckedGet<T>():
        m_isset(false)
#ifdef CAUTION
        , m_sealed(false)
#endif
    {}
    // needs to be same as operator=
    explicit CheckedGet<T>(const CheckedGet<T> &in):
        m_isset(in.m_isset)
#ifdef CAUTION
        , m_sealed(in.m_sealed)
#endif
    {
        if(m_isset) {
            m_val = in.m_val;
        }
    }
    // needs to be same as clear(), set(ival)
    explicit CheckedGet<T>(const T &ival):
        m_isset(true),
#ifdef CAUTION
        m_sealed(false),
#endif
        m_val(ival) {
    }
    CheckedGet<T> &operator=(const CheckedGet<T> &in) {
        m_isset = in.m_isset;
#ifdef CAUTION
        m_sealed = in.m_sealed;
#endif
        m_val = in.m_val;
        return *this;
    }
    void clear() {
        m_isset = false;
#ifdef CAUTION
        m_sealed = false;
#endif
        m_val = T();
    }
    const T &get() const {
#ifdef CAUTION

        if(!m_isset) {
            PROGRAMMERERROR("%s: trying to get, but not set first", __PRETTY_FUNCTION__);
        }

#endif
        return m_val;
    }
    void set(const T &ival) {
#ifdef CAUTION

        if(m_sealed) {
            PROGRAMMERERROR("%s: trying to set, but has previously been sealed", __PRETTY_FUNCTION__);
        }

#endif
        m_isset = true;
        m_val = ival;
    }
    void force_set(const T &ival) {
        m_isset = true;
        m_val = ival;
    }
    void set_and_seal(const T &ival) {
        set(ival);
        seal();
    }
    void seal() {
#ifdef CAUTION
        m_sealed = true;
#endif
    }
    void unseal() {
#ifdef CAUTION
        m_sealed = false;
#endif
    }
    bool issealed() const {
#ifdef CAUTION
        return m_sealed;
#else
        return false;
#endif
    }
    bool isset() const {
        return m_isset;
    }
};
// problem: only integer types for T (C++ problem)
// implicitly defaults to template argument
// set() fails if seal() has been called
template <typename T, const T def> class CheckedGetDefault {
private:
#ifdef CAUTION
    bool m_sealed;
#endif
    T m_val;
public:
    // needs to be same as clear()
    explicit CheckedGetDefault<T, def>():
#ifdef CAUTION
        m_sealed(false),
#endif
        m_val(def) {}
    // needs to be same as operator=
    explicit CheckedGetDefault<T, def>(const T &ival):
#ifdef CAUTION
        m_sealed(false),
#endif
        m_val(ival) {}
    // needs to be same as clear(), set(ival)
    explicit CheckedGetDefault<T, def>(const CheckedGetDefault<T, def> &in):
#ifdef CAUTION
        m_sealed(in.m_sealed),
#endif
        m_val(in.m_val) {}
    CheckedGetDefault<T, def> &operator=(const CheckedGetDefault<T, def> &in) {
#ifdef CAUTION
        m_sealed = in.m_sealed;
#endif
        m_val = in.m_val;
        return *this;
    }
    void clear() {
#ifdef CAUTION
        m_sealed = false;
#endif
        m_val = def;
    }
    const T &get() const {
        return m_val;
    }
    void set(const T &ival) {
#ifdef CAUTION

        if(m_sealed) {
            PROGRAMMERERROR("%s: trying to set, but has previously been sealed", __PRETTY_FUNCTION__);
        }

#endif
        m_val = ival;
    }
    void set_and_seal(const T &ival) {
        set(ival);
        seal();
    }
    void seal() {
#ifdef CAUTION
        m_sealed = true;
#endif
    }
    void unseal() {
#ifdef CAUTION
        m_sealed = false;
#endif
    }
    bool issealed() const {
#ifdef CAUTION
        return m_sealed;
#else
        return false;
#endif
    }
};
// problem: only integer types for T (C++ problem)
// implicitly defaults to template argument
template <typename T, T def> class GetDefault {
private:
    T m_val;
public:
    // needs to be same as clear()
    explicit GetDefault<T, def>(): m_val(def) {}
    // needs to be same as operator=
    explicit GetDefault<T, def>(const T &ival): m_val(ival) {}
    // needs to be same as clear(), set(ival)
    explicit GetDefault<T, def>(const GetDefault<T, def> &in): m_val(in.m_val) {}
    GetDefault<T, def> &operator=(const GetDefault<T, def> &in) {
        m_val = in.m_val;
        return *this;
    }
    void clear() {
        m_val = def;
    }
    const T &get() const {
        return m_val;
    }
    void set(const T &ival) {
        m_val = ival;
    }
};

#endif // CHECKEDGET_H
