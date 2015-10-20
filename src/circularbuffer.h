#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <type_traits>

// this assumes std::is_trivially_copyable<T>
template<typename T>
class MyCircularBuffer
{
private:
    size_t m_head;
    size_t m_tail;
    size_t m_maxlen;
    T *m_buf;
    // forbid
    MyCircularBuffer();
    MyCircularBuffer(const MyCircularBuffer &);
    MyCircularBuffer &operator=(const MyCircularBuffer &in);
private:
    size_t next_after(size_t n) const
    {
        if(n >= m_maxlen - 1) {
            return 0;
        }
        else {
            return n + 1;
        }
    }
    T *pointer_to(size_t i)
    {
        return &(m_buf[i]);
    }
    void *vpointer_to(size_t i)
    {
        return &(m_buf[i]);
    }
    static_assert(std::is_trivially_copyable<T>::value, "Bad type used - not copyable");
public:
    MyCircularBuffer(size_t len)
        : m_head(0)
        , m_tail(0)
        , m_maxlen(len)
        , m_buf((T *)calloc(m_maxlen, sizeof(T)))
    {
        if(len < 1) {
            PROGRAMMERERROR("WTF");
        }

        clear();
    }
    ~MyCircularBuffer()
    {
        free(m_buf);
        m_buf = NULL;
    }
    bool isEmpty() const
    {
        return m_head == m_tail;
    }
    bool isFull() const
    {
        return next_after(m_head) == m_tail;
    }
    void clear()
    {
        m_head = 0;
        m_tail = 0;
    }
    // this needs to be fast
    void append(const T *pelem)
    {
        if(isFull()) {
            m_tail = next_after(m_tail);
        }

        memcpy(vpointer_to(m_head), pelem, sizeof(T));
        m_head = next_after(m_head);
    }
    bool pop(T *pelem)
    {
        // if the head isn't ahead of the tail, we don't have any characters
        if(isEmpty()) {
            return false;    // quit with an error
        }

        memcpy(pelem, vpointer_to(m_tail), sizeof(T));
        // memset(vpointer_to(m_tail), 0, sizeof(T));  // clear the data (optional)

        m_tail = next_after(m_tail);

        return true;
    }
    void get_range_1(T const **pptr, size_t *sz)
    {
        if(isEmpty()) {
            *pptr = NULL;
            *sz = 0;
            return;
        }

        if(m_tail < m_head) {
            *pptr = pointer_to(m_tail);
            *sz = m_head - m_tail;
        }
        else {
            *pptr = pointer_to(m_tail);
            *sz = m_maxlen - m_tail;
        }
    }
    void get_range_2(T const **pptr, size_t *sz)
    {
        if(isEmpty()) {
            *pptr = NULL;
            *sz = 0;
            return;
        }

        if(m_tail < m_head) {
            *pptr = NULL;
            *sz = 0;
        }
        else {
            *pptr = pointer_to(0);
            *sz = m_head;
        }
    }
};

#endif // CIRCULAR_BUFFER_H
