#pragma once
 
#include <assert.h>
#include <string.h>
 
class BitMap
{
public:
    //构造函数
    BitMap(const size_t & range) {
        assert(range >= 0);
        if (bits != nullptr) {
            delete[] bits;
        }
        count = range;
        size = range / 32 + 1;
        bits = new unsigned int[size];
    }
    //析构函数
    ~BitMap() {
        delete[] bits;
    }
    //初始化数据，把所有数据置0
    void init() {
        for (int i = 0; i < size; i++)
            bits[i] = 0;
    }
    //增加数据到位图
    void add(const size_t & num) {
        assert(count > num);
        int index = num / 32;
        int bit_index = num % 32;
        bits[index] |= 1 << bit_index;
    }
    //删除数据到位图
    void remove(const size_t & num){
        assert(count > num );
        int index = num / 32;
        int bit_index = num % 32;
        bits[index] &= ~(1 << bit_index);
    }
    //查找数据到位图
    bool find(const size_t & num) {
        assert(count > num);
        int index = num / 32;
        int bit_index = num % 32;
        return (bits[index] >> bit_index) & 1;
    }
//位图相关数据
private:
    unsigned int* bits=nullptr;
    int size;
    int count;
};