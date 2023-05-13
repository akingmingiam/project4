#include <iostream>
#include <cstring>
#include <atomic>
#include <mutex>

enum DataType
{
    INT = sizeof(int),
    DOUBLE = sizeof(double),
    CHAR = sizeof(char)
};

class ReferenceCounter
{
public:
    ReferenceCounter() : m_count(1) {}
    void incease();
    bool decrease();
    size_t getCount();

private:
    std::atomic_size_t m_count;
    std::mutex m_mutex;
};

class DataBlob
{
private:
    struct Data
    {
        void *data;
        ReferenceCounter *referenceCounter;
        size_t rows;
        size_t cols;
        size_t channels;
        DataType type;
        // size_t refCount;
        size_t length;

        Data(size_t r, size_t c, size_t ch, DataType type);
        Data(size_t r, size_t c, size_t ch, DataType type, void *data);
        ~Data();
        Data *clone();
    };

    Data *Blob_Data;

public:
    DataBlob(size_t r, size_t c, size_t ch, DataType type);             // 不含数据构造函数
    DataBlob(size_t r, size_t c, size_t ch, DataType type, void *data); // 含数据构造函数
    DataBlob(const DataBlob &other);                                    // 拷贝构造函数
    ~DataBlob();                                                        // 析构函数

    bool data_dimension(const DataBlob &other) const; // 判断Blob_Data尺寸，类型是否相等
    bool mul_dimension(const DataBlob &other) const;  // 判断是否能进行乘法运算
    DataBlob &operator=(const DataBlob &other);       // 赋值运算符
    bool operator==(const DataBlob &other) const;
    bool operator!=(const DataBlob &other) const;

    template <typename T>
    T *convertPtr(const void *ptr) const; // 转化指针类型
    template <typename T>
    T *convertPtr() const; // 转化本类的数据指针
    template <typename T>
    DataBlob add(const DataBlob &other) const;
    template <typename T>
    DataBlob minus(const DataBlob &other) const;
    template <typename T>
    DataBlob mul(const DataBlob &other) const;

    DataBlob operator+(const DataBlob &other) const;
    DataBlob operator-(const DataBlob &other) const;
    DataBlob operator*(const DataBlob &other) const;

    template <typename T>
    T at(size_t r, size_t c, size_t ch) const;
    template <typename T>
    void set(size_t r, size_t c, size_t ch, T value);
    void release();

    DataBlob clone() const;
};


void ReferenceCounter::incease()
{
    m_mutex.lock();
    ++m_count;
    m_mutex.unlock();
}

bool ReferenceCounter::decrease()
{
    m_mutex.lock();
    --m_count;
    bool result = (m_count == 0);
    m_mutex.unlock();
    return result;
}

size_t ReferenceCounter::getCount()
{
    m_mutex.lock();
    size_t count = m_count;
    m_mutex.unlock();
    return count;
}

// Data: 不含数据构造函数
DataBlob::Data::Data(size_t r, size_t c, size_t ch, DataType type)
{
    rows = r;
    cols = c;
    channels = ch;
    this->type = type;
    length = r * c * ch;
    data = new char[length * type];
    referenceCounter = new ReferenceCounter();
}

// Data: 含数据构造函数
DataBlob::Data::Data(size_t r, size_t c, size_t ch, DataType type, void *data)
{
    rows = r;
    cols = c;
    channels = ch;
    this->type = type;
    length = r * c * ch;
    this->data = data;
    referenceCounter = new ReferenceCounter();
}

DataBlob::Data::~Data()
{
    delete[] (char *)data;
    delete referenceCounter;
}

DataBlob::Data *DataBlob::Data::clone()
{
    Data *newData = new Data(rows, cols, channels, type);
    if (newData == nullptr)
    {
        throw std::invalid_argument("Data::clone(): Failed to allocate memory for new Data object.");
    }
    memcpy(newData->data, data, rows * cols * channels * type);
    return newData;
}

// 不含数据的构造函数
DataBlob::DataBlob(size_t r, size_t c, size_t ch, DataType type)
{
    if (r <= 0 || c <= 0 || ch <= 0)
    {
        std::cerr << "DataBlob: dimensions must be positive!" << std::endl;
        exit(EXIT_FAILURE);
    }
    Blob_Data = new Data(r, c, ch, type);
    if (Blob_Data == nullptr)
    {
        std::cerr << "DataBlob::DataBlob(): Failed to allocate memory for new Data object." << std::endl;
        exit(EXIT_FAILURE);
    }
}

// 含数据的构造函数
DataBlob::DataBlob(size_t r, size_t c, size_t ch, DataType type, void *data)
{
    if (r <= 0 || c <= 0 || ch <= 0)
    {
        std::cerr << "DataBlob: dimensions must be positive!" << std::endl;
        exit(EXIT_FAILURE);
    }
    Blob_Data = new Data(r, c, ch, type, data);
    if (Blob_Data == nullptr)
    {
        std::cerr << "DataBlob::DataBlob(): Failed to allocate memory for new Data object." << std::endl;
        exit(EXIT_FAILURE);
    }
}

// 拷贝构造函数
DataBlob::DataBlob(const DataBlob &other)
{
    Blob_Data = other.Blob_Data;
    Blob_Data->referenceCounter->incease();
    std::cout << "The copy constructor is used" << std::endl;
}
// 析构函数
DataBlob::~DataBlob()
{
    release();
    std::cout << "The destructor of DataBlob is used" << std::endl;
}

// 判断Blob_Data尺寸，类型是否相等
bool DataBlob::data_dimension(const DataBlob &other) const
{
    if (Blob_Data->rows != other.Blob_Data->rows || Blob_Data->cols != other.Blob_Data->cols ||
        Blob_Data->channels != other.Blob_Data->channels || Blob_Data->type != other.Blob_Data->type)
    {
        return false;
    }
    else
    {
        return true;
    }
}

// 判断是否能进行乘法运算
bool DataBlob::mul_dimension(const DataBlob &other) const
{
    if (Blob_Data->cols != other.Blob_Data->rows || Blob_Data->channels != other.Blob_Data->channels ||
        Blob_Data->type != other.Blob_Data->type)
    {
        return false;
    }
    else
    {
        return true;
    }
}

// 赋值运算符
DataBlob &DataBlob::operator=(const DataBlob &other)
{
    if (this != &other)
    {
        release();
        Blob_Data = other.Blob_Data;
        Blob_Data->referenceCounter->incease();
    }
    std::cout << "The assignment operator is used" << std::endl;
    return *this;
}

bool DataBlob::operator==(const DataBlob &other) const
{
    if (!data_dimension(other))
    {
        return false;
    }
    return std::memcmp(Blob_Data->data, other.Blob_Data->data, Blob_Data->length * other.Blob_Data->type) == 0;
}

bool DataBlob::operator!=(const DataBlob &other) const
{
    return !(*this == other);
}

// 转化指针类型
template <typename T>
T *DataBlob::convertPtr(const void *ptr) const
{
    if (ptr == nullptr)
    {
        throw std::invalid_argument("Null pointer");
    }
    T *converted = static_cast<T *>(const_cast<void *>(ptr));
    if (converted == nullptr)
    {
        std::cerr << "fall to convert ptr!" << std::endl;
        exit(EXIT_FAILURE);
    }
    else
    {
        return converted;
    }
}

// 转化本类的数据指针类型
template <typename T>
T *DataBlob::convertPtr() const
{
    if (Blob_Data == nullptr || Blob_Data->data == nullptr)
    {
        throw std::invalid_argument("Null pointer");
    }
    T *converted = static_cast<T *>(const_cast<void *>(Blob_Data->data));
    if (converted == nullptr)
    {
        std::cerr << "fall to convert ptr!" << std::endl;
        exit(EXIT_FAILURE);
    }
    else
    {
        return converted;
    }
}

template <typename T>
DataBlob DataBlob::add(const DataBlob &other) const
{
    T *data_this = convertPtr<T>();
    T *data_other = other.convertPtr<T>();
    T *result_data = new T[Blob_Data->length];
    for (size_t i = 0; i < Blob_Data->length; i++)
    {
        result_data[i] = data_this[i] + data_other[i];
    }
    return DataBlob(Blob_Data->rows, Blob_Data->cols, Blob_Data->channels, Blob_Data->type, result_data);
}

template <typename T>
DataBlob DataBlob::minus(const DataBlob &other) const
{
    T *data_this = convertPtr<T>();
    T *data_other = other.convertPtr<T>();
    T *result_data = new T[Blob_Data->length];
    for (size_t i = 0; i < Blob_Data->length; i++)
    {
        result_data[i] = data_this[i] - data_other[i];
    }
    return DataBlob(Blob_Data->rows, Blob_Data->cols, Blob_Data->channels, Blob_Data->type, result_data);
}

template <typename T>
DataBlob DataBlob::mul(const DataBlob &other) const
{
    T *data_this = convertPtr<T>();
    T *data_other = other.convertPtr<T>();
    T *result_data = new T[Blob_Data->length];
    size_t r_left = Blob_Data->rows;
    size_t c_left = Blob_Data->cols;
    size_t c_right = other.Blob_Data->cols;
    size_t num_ch = Blob_Data->channels;
    size_t loc_result = 0;
    for (size_t ch = 0; ch < num_ch; ch++)
    {
        for (size_t r = 0; r < r_left; r++)
        {
            for (size_t c = 0; c < c_right; c++)
            {
                T result_rc = 0;
                size_t loc_left = r * c_left;
                size_t loc_right = c;
                for (size_t i = 0; i < c_left; i++)
                {
                    result_rc = result_rc + data_this[loc_left] * data_other[loc_right];
                    loc_left = loc_left + 1;
                    loc_right = loc_right + c_right;
                }
                result_data[loc_result] = result_rc;
                loc_result = loc_result + 1;
            }
        }
    }
    return DataBlob(Blob_Data->rows, Blob_Data->cols, Blob_Data->channels, Blob_Data->type, result_data);
}

DataBlob DataBlob::operator+(const DataBlob &other) const
{
    if (!data_dimension(other))
    {
        std::cerr << "The size of two DataBlobs doesn't match." << std::endl;
        exit(EXIT_FAILURE);
    }
    switch (Blob_Data->type)
    {
    case DataType::INT:
    {
        return add<int>(other);
        break;
    }
    case DataType::DOUBLE:
    {
        return add<double>(other);
        break;
    }
    case DataType::CHAR:
    {
        return add<char>(other);
        break;
    }
    default:
        throw std::invalid_argument("Invalid data type!");
    }
}

DataBlob DataBlob::operator-(const DataBlob &other) const
{
    if (!data_dimension(other))
    {
        std::cerr << "The size of two DataBlobs doesn't match." << std::endl;
        exit(EXIT_FAILURE);
    }
    switch (Blob_Data->type)
    {
    case DataType::INT:
    {
        return minus<int>(other);
        break;
    }
    case DataType::DOUBLE:
    {
        return minus<double>(other);
        break;
    }
    case DataType::CHAR:
    {
        return minus<char>(other);
        break;
    }
    default:
        throw std::invalid_argument("Invalid data type!");
    }
}

DataBlob DataBlob::operator*(const DataBlob &other) const
{
    if (!mul_dimension(other))
    {
        std::cerr << "The size of two DataBlobs doesn't match." << std::endl;
        exit(EXIT_FAILURE);
    }
    switch (Blob_Data->type)
    {
    case DataType::INT:
    {
        return mul<int>(other);
        break;
    }
    case DataType::DOUBLE:
    {
        return mul<double>(other);
        break;
    }
    case DataType::CHAR:
    {
        return mul<char>(other);
        break;
    }
    default:
        throw std::invalid_argument("Invalid data type for matrix multiplication!");
    }
}

template <typename T>
T DataBlob::at(size_t r, size_t c, size_t ch) const
{
    if (r < 0 || r >= Blob_Data->rows || c < 0 || c >= Blob_Data->cols || ch < 0 || ch >= Blob_Data->channels)
    {
        std::cerr << "DataBlob::at(): Index out of range!" << std::endl;
        exit(EXIT_FAILURE);
    }
    return *((T *)((char *)Blob_Data->data + (ch * Blob_Data->rows * Blob_Data->cols + (r * Blob_Data->cols + c)) * Blob_Data->type));
}

template <typename T>
void DataBlob::set(size_t r, size_t c, size_t ch, T value)
{
    if (r < 0 || r >= Blob_Data->rows || c < 0 || c >= Blob_Data->cols || ch < 0 || ch >= Blob_Data->channels)
    {
        std::cerr << "DataBlob::set(): Index out of range!" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (Blob_Data->referenceCounter->getCount() > 1)
    {
        if (Blob_Data == nullptr)
        {
            std::cerr << "DataBlob::set(): Blob_Data is nullptr." << std::endl;
            exit(EXIT_FAILURE);
        }
        Data *newData = Blob_Data->clone();
        Blob_Data->referenceCounter->decrease();
        Blob_Data = newData;
    }
    *((T *)Blob_Data->data + (r * Blob_Data->cols + c) + ch * Blob_Data->cols * Blob_Data->rows) = value;
}

void DataBlob::release()
{
    if (Blob_Data->referenceCounter->decrease())
    {
        delete Blob_Data;
        Blob_Data = nullptr; // 在多线程中更加安全
    }
}

DataBlob DataBlob::clone() const
{
    DataBlob newBlob(Blob_Data->rows, Blob_Data->cols, Blob_Data->channels, Blob_Data->type);
    memcpy(newBlob.Blob_Data->data, Blob_Data->data, Blob_Data->rows * Blob_Data->cols * Blob_Data->channels * Blob_Data->type);
    return newBlob;
}