#ifndef RINGBUFFER_H
#define RINGBUFFER_H

template<class T>
class RingBuffer
{
private:
    T* buffer = nullptr;
    size_t length = 0;
    size_t read_index = 0;
    size_t write_index = 0;
    size_t readable_length = 0;
public:
    RingBuffer(size_t _length);
    ~RingBuffer();
    int write(T* _values, size_t _length);
    int read(T* _buffer, size_t _length);
    size_t readable();
    void reset();
    void flush();
};

template<class T>
inline RingBuffer<T>::RingBuffer(size_t _length)
{
    buffer = new T[_length];
    length = _length;
    read_index = 0;
    write_index = 0;
    readable_length = 0;
}

template<class T>
inline RingBuffer<T>::~RingBuffer()
{
    delete[] buffer;
}

template<class T>
inline int RingBuffer<T>::write(T* _values, size_t _length) {
    if (readable_length + _length > length) {
        return 0;
    }
    for (size_t i = 0; i < _length; i++) {
        buffer[(i + write_index) % length] = _values[i];
    }
    readable_length += _length;
    write_index = (write_index + _length) % length;
    return _length;
}

template<class T>
inline int RingBuffer<T>::read(T* _buffer, size_t _length) {
    size_t read_size = _length > readable_length ? readable_length : _length;
    for (size_t i = 0; i < read_size; i++) {
        _buffer[i] = buffer[(i + read_index) % length];
    }
    if (read_size != _length) {
        for (size_t i = read_size; i < _length; i++) {
            _buffer[i] = 0;
        }
    }
    readable_length -= read_size;
    read_index = (read_index + _length) % length;
    return _length;
}

template<class T>
inline size_t RingBuffer<T>::readable() {
    return readable_length;
}

template<class T>
inline void RingBuffer<T>::reset() {
    read_index = 0;
    write_index = 0;
    readable_length = 0;
}

template<class T>
inline void RingBuffer<T>::flush() {
    for (size_t i = 0; i < length; i++) {
        buffer[i] = (T)0;
    }
    reset();
}

#endif