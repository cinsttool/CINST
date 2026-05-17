#pragma once
#include <vector>
#include <string>
#include <stdio.h>
#include <functional>
#include <assert.h>

class RecordWriter;
template<typename T>
class RecordReader 
{
public:
    RecordReader(const std::string& filename, size_t buffer_size = 1<<10);
    ~RecordReader();

    RecordReader(const RecordReader&) = delete;
    RecordReader& operator=(const RecordReader&) = delete;

    void transform_local();

    void register_transformer(const std::function<void(T&)>& func);


private:
    T* _buffer;
    FILE* f;
    size_t _buffer_size;
    std::vector<std::function<void(T&)>> transformers;

};


template<typename T>
RecordReader<T>::RecordReader(const std::string& filepath, size_t buffer_size): _buffer_size(buffer_size)
{
    _buffer = new T[buffer_size];
    f = fopen(filepath.c_str(), "r+");
    assert(f != nullptr);
}

template<typename T>
RecordReader<T>::~RecordReader() 
{
    delete [] _buffer;
    fclose(f);
}


template <typename T>
inline void RecordReader<T>::transform_local()
{
    bool eof = false;
    while (!eof) 
    {
        int count = fread(_buffer, sizeof(T), _buffer_size, f);
        eof = feof(f);
        for (int i = 0; i < count; ++i) 
        {
            for (const auto& func: transformers)
            {
                func(_buffer[i]);
            }
        }
        fseek(f, -count * sizeof(T), SEEK_CUR);
        fwrite(_buffer, sizeof(T), count, f);
        fflush(f);
    }
}

template <typename T>
inline void RecordReader<T>::register_transformer(const std::function<void(T &)> &func)
{
    transformers.push_back(func);
}
