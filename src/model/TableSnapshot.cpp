
#include <cstring>
#include <templatious/FullPack.hpp>

#include "TableSnapshot.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {
    template <class T>
    const char* toConstChar(T&& t) {
        return reinterpret_cast<const char*>(std::addressof(t));
    }
}

TableSnapshot::TableSnapshot() : _bufSize(-1), _buffer(nullptr) {}

TableSnapshot::~TableSnapshot() {
    delete[] _buffer;
}

TableSnapshot::TableSnapshot(TableSnapshot&& other) :
    _bufSize(other._bufSize), _buffer(other._buffer)
{
    other._bufSize = -1;
    other._buffer = nullptr;
}

void TableSnapshot::traverse(const TraverseFunction& func) const {
    templatious::StaticBuffer< const char*, 32 > buf;
    templatious::StaticBuffer< int, 32 > bufInt;
    auto vHead = buf.getStaticVector();
    auto vHeadSize = bufInt.getStaticVector();
    int total = *reinterpret_cast<int*>(_buffer);
    int columns = *reinterpret_cast<int*>(_buffer + sizeof(int));

    char* headerStart = _buffer + 2*sizeof(int);
    TEMPLATIOUS_0_TO_N(i,columns) {
        int currSize = *reinterpret_cast<int*>(headerStart);
        headerStart += sizeof(currSize);
        SA::add(vHead,headerStart);
        SA::add(vHeadSize,currSize);
        headerStart += currSize;
    }
    char* bodyStart = headerStart;

    std::string strBufHead,strBufVal;

    TEMPLATIOUS_0_TO_N(i,total) {
        TEMPLATIOUS_0_TO_N(j,columns) {
            int currSize = *reinterpret_cast<int*>(bodyStart);
            bodyStart += sizeof(currSize);
            strBufHead.assign(vHead[j],vHead[j] + vHeadSize[j]);
            strBufVal.assign(bodyStart,bodyStart + currSize);
            if (!func(i,j,strBufVal.c_str(),strBufHead.c_str())) {
                return;
            }
            bodyStart += currSize;
        }
    }
}

TableSnapshotBuilder::TableSnapshotBuilder(int columns,const char** headerNames) :
    _columns(columns), _totalCount(0)
{
    _tmp.resize(_columns);
    // write record count right away, will be edited in the end
    _stream.write(toConstChar(_totalCount),sizeof(_totalCount));
    _stream.write(toConstChar(_columns),sizeof(_columns));

    std::string buf;
    TEMPLATIOUS_0_TO_N(i,columns) {
        buf = headerNames[i];
        int size = buf.size();
        _stream.write(toConstChar(size),sizeof(size));
        _stream.write(buf.c_str(),size);
    }

    SM::set("[EMPTY]",_tmp);
}

void TableSnapshotBuilder::writeCurrentRow() {
    TEMPLATIOUS_FOREACH(auto& i,_tmp) {
        int size = i.size();
        _stream.write(toConstChar(size),sizeof(size));
        _stream.write(i.c_str(),size);
    }
}

void TableSnapshotBuilder::commitRow() {
    writeCurrentRow();

    SM::set("[EMPTY]",_tmp);
    ++_totalCount;
}

void TableSnapshotBuilder::setValue(int column,const char* value) {
    assert( column < _columns && column >= 0 && "HUH?!?" );
    _tmp[column] = value;
}

TableSnapshot TableSnapshotBuilder::getSnapshot() {
    TableSnapshot outRes;

    _stream.seekg(0,std::ios::end);
    int res = _stream.tellg();
    _stream.seekg(0,std::ios::beg);

    std::unique_ptr< char[] > uptr(new char[res]);
    outRes._bufSize = res;
    outRes._buffer = uptr.release();

    auto string = _stream.str();
    _stream.read(outRes._buffer,outRes._bufSize);

    memcpy(outRes._buffer,
        toConstChar(_totalCount),
        sizeof(_totalCount));

    return outRes;
}
