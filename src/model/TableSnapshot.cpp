
#include <templatious/FullPack.hpp>

#include "TableSnapshot.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {
    template <class T>
    const char* toConstChar(T&& t) {
        return reinterpret_cast<const char*>(std::addressof(t));
    }
}

TableSnapshotBuilder::TableSnapshotBuilder(int columns,char** headerNames) :
    _columns(columns), _totalCount(0)
{
    _tmp.resize(_columns);
    // write record count right away, will be edited in the end
    _stream.write(toConstChar(_totalCount),sizeof(_totalCount));
    _stream.write(toConstChar(_columns),sizeof(_columns));

    std::string buf;
    TEMPLATIOUS_0_TO_N(i,columns) {
        buf = headerNames[i];
        auto size = buf.size();
        _stream.write(toConstChar(size),sizeof(size));
        _stream.write(buf.c_str(),size);
    }
}

void TableSnapshotBuilder::writeCurrentRow() {
    TEMPLATIOUS_FOREACH(auto& i,_tmp) {
        auto size = i.size();
        _stream.write(toConstChar(size),sizeof(size));
        _stream.write(i.c_str(),sizeof(size));
    }
}

void TableSnapshotBuilder::newRow() {
    if (_totalCount > 0) {
        writeCurrentRow();
    }
    SM::set("[EMPTY]",_tmp);
}
