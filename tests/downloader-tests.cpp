
#include <fstream>

#include <templatious/FullPack.hpp>
#include <templatious/detail/DynamicVirtualMatchFunctor.hpp>
#include <boost/filesystem.hpp>
#include <io/AsyncDownloader.hpp>
#include <io/SafeListDownloader.hpp>
#include <io/Interval.hpp>
#include <io/RandomFileWriter.hpp>
#include <io/SafeListDownloaderFactory.hpp>
#include <model/AsyncSqlite.hpp>
#include <util/ScopeGuard.hpp>

#include <sqlite3.h>

#include "catch.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {
    bool ensureExpected(int64_t size,const char* buffer) {
        TEMPLATIOUS_0_TO_N(i,size) {
            if (buffer[i] != '7') {
                return false;
            }
        }
        return true;
    }

    StrongMsgPtr testDownloader() {
        typedef SafeLists::AsyncDownloader AD;
        static auto imitation = AD::createNew("imitation");
        return imitation;
    }

    std::vector<char> fileE_list() {
        std::vector<char> buf(424);
        buf[0] = 4; buf[1] = -102; buf[2] = 55; buf[3] = 55;
        buf[4] = 55; buf[5] = 18; buf[6] = -59; buf[7] = -99;
        buf[8] = 42; buf[9] = -33; buf[10] = 124; buf[11] = 92;
        buf[12] = 74; buf[13] = -9; buf[14] = -78; buf[15] = 55;
        buf[16] = 60; buf[17] = -112; buf[18] = 122; buf[19] = 117;
        buf[20] = 32; buf[21] = -125; buf[22] = 55; buf[23] = 53;
        buf[24] = 34; buf[25] = 26; buf[26] = -106; buf[27] = 24;
        buf[28] = 77; buf[29] = -63; buf[30] = -77; buf[31] = -83;
        buf[32] = 23; buf[33] = 0; buf[34] = 0; buf[35] = 0;
        buf[36] = 0; buf[37] = 0; buf[38] = 0; buf[39] = 0;
        buf[40] = 0; buf[41] = 0; buf[42] = 0; buf[43] = 0;
        buf[44] = 0; buf[45] = 0; buf[46] = 0; buf[47] = 0;
        buf[48] = -91; buf[49] = -86; buf[50] = 90; buf[51] = 0;
        buf[52] = 0; buf[53] = 0; buf[54] = 0; buf[55] = 0;
        buf[56] = 0; buf[57] = 0; buf[58] = 0; buf[59] = 0;
        buf[60] = 0; buf[61] = 0; buf[62] = 0; buf[63] = 0;
        buf[64] = -124; buf[65] = -77; buf[66] = 3; buf[67] = 0;
        buf[68] = 0; buf[69] = 0; buf[70] = 0; buf[71] = 0;
        buf[72] = -43; buf[73] = 93; buf[74] = 4; buf[75] = 0;
        buf[76] = 0; buf[77] = 0; buf[78] = 0; buf[79] = 0;
        buf[80] = -64; buf[81] = -81; buf[82] = 4; buf[83] = 0;
        buf[84] = 0; buf[85] = 0; buf[86] = 0; buf[87] = 0;
        buf[88] = -13; buf[89] = 47; buf[90] = 12; buf[91] = 0;
        buf[92] = 0; buf[93] = 0; buf[94] = 0; buf[95] = 0;
        buf[96] = -34; buf[97] = -127; buf[98] = 12; buf[99] = 0;
        buf[100] = 0; buf[101] = 0; buf[102] = 0; buf[103] = 0;
        buf[104] = -89; buf[105] = 4; buf[106] = 13; buf[107] = 0;
        buf[108] = 0; buf[109] = 0; buf[110] = 0; buf[111] = 0;
        buf[112] = -64; buf[113] = 112; buf[114] = 18; buf[115] = 0;
        buf[116] = 0; buf[117] = 0; buf[118] = 0; buf[119] = 0;
        buf[120] = -102; buf[121] = -128; buf[122] = 18; buf[123] = 0;
        buf[124] = 0; buf[125] = 0; buf[126] = 0; buf[127] = 0;
        buf[128] = 72; buf[129] = 68; buf[130] = 19; buf[131] = 0;
        buf[132] = 0; buf[133] = 0; buf[134] = 0; buf[135] = 0;
        buf[136] = 9; buf[137] = -34; buf[138] = 19; buf[139] = 0;
        buf[140] = 0; buf[141] = 0; buf[142] = 0; buf[143] = 0;
        buf[144] = -116; buf[145] = 24; buf[146] = 20; buf[147] = 0;
        buf[148] = 0; buf[149] = 0; buf[150] = 0; buf[151] = 0;
        buf[152] = 93; buf[153] = -34; buf[154] = 20; buf[155] = 0;
        buf[156] = 0; buf[157] = 0; buf[158] = 0; buf[159] = 0;
        buf[160] = 86; buf[161] = 125; buf[162] = 23; buf[163] = 0;
        buf[164] = 0; buf[165] = 0; buf[166] = 0; buf[167] = 0;
        buf[168] = -8; buf[169] = -122; buf[170] = 23; buf[171] = 0;
        buf[172] = 0; buf[173] = 0; buf[174] = 0; buf[175] = 0;
        buf[176] = 60; buf[177] = -53; buf[178] = 23; buf[179] = 0;
        buf[180] = 0; buf[181] = 0; buf[182] = 0; buf[183] = 0;
        buf[184] = 99; buf[185] = 126; buf[186] = 24; buf[187] = 0;
        buf[188] = 0; buf[189] = 0; buf[190] = 0; buf[191] = 0;
        buf[192] = -31; buf[193] = 83; buf[194] = 25; buf[195] = 0;
        buf[196] = 0; buf[197] = 0; buf[198] = 0; buf[199] = 0;
        buf[200] = 98; buf[201] = 102; buf[202] = 25; buf[203] = 0;
        buf[204] = 0; buf[205] = 0; buf[206] = 0; buf[207] = 0;
        buf[208] = -122; buf[209] = 110; buf[210] = 25; buf[211] = 0;
        buf[212] = 0; buf[213] = 0; buf[214] = 0; buf[215] = 0;
        buf[216] = 113; buf[217] = -48; buf[218] = 25; buf[219] = 0;
        buf[220] = 0; buf[221] = 0; buf[222] = 0; buf[223] = 0;
        buf[224] = 92; buf[225] = 34; buf[226] = 26; buf[227] = 0;
        buf[228] = 0; buf[229] = 0; buf[230] = 0; buf[231] = 0;
        buf[232] = 66; buf[233] = 93; buf[234] = 26; buf[235] = 0;
        buf[236] = 0; buf[237] = 0; buf[238] = 0; buf[239] = 0;
        buf[240] = -59; buf[241] = -105; buf[242] = 26; buf[243] = 0;
        buf[244] = 0; buf[245] = 0; buf[246] = 0; buf[247] = 0;
        buf[248] = -68; buf[249] = 63; buf[250] = 29; buf[251] = 0;
        buf[252] = 0; buf[253] = 0; buf[254] = 0; buf[255] = 0;
        buf[256] = 63; buf[257] = 122; buf[258] = 29; buf[259] = 0;
        buf[260] = 0; buf[261] = 0; buf[262] = 0; buf[263] = 0;
        buf[264] = -111; buf[265] = -122; buf[266] = 34; buf[267] = 0;
        buf[268] = 0; buf[269] = 0; buf[270] = 0; buf[271] = 0;
        buf[272] = 1; buf[273] = -101; buf[274] = 34; buf[275] = 0;
        buf[276] = 0; buf[277] = 0; buf[278] = 0; buf[279] = 0;
        buf[280] = -65; buf[281] = 8; buf[282] = 60; buf[283] = 0;
        buf[284] = 0; buf[285] = 0; buf[286] = 0; buf[287] = 0;
        buf[288] = -105; buf[289] = 26; buf[290] = 60; buf[291] = 0;
        buf[292] = 0; buf[293] = 0; buf[294] = 0; buf[295] = 0;
        buf[296] = -52; buf[297] = -89; buf[298] = 60; buf[299] = 0;
        buf[300] = 0; buf[301] = 0; buf[302] = 0; buf[303] = 0;
        buf[304] = 50; buf[305] = 14; buf[306] = 61; buf[307] = 0;
        buf[308] = 0; buf[309] = 0; buf[310] = 0; buf[311] = 0;
        buf[312] = 110; buf[313] = -119; buf[314] = 61; buf[315] = 0;
        buf[316] = 0; buf[317] = 0; buf[318] = 0; buf[319] = 0;
        buf[320] = 40; buf[321] = -18; buf[322] = 61; buf[323] = 0;
        buf[324] = 0; buf[325] = 0; buf[326] = 0; buf[327] = 0;
        buf[328] = 72; buf[329] = 124; buf[330] = 67; buf[331] = 0;
        buf[332] = 0; buf[333] = 0; buf[334] = 0; buf[335] = 0;
        buf[336] = 70; buf[337] = -86; buf[338] = 67; buf[339] = 0;
        buf[340] = 0; buf[341] = 0; buf[342] = 0; buf[343] = 0;
        buf[344] = 51; buf[345] = 40; buf[346] = 74; buf[347] = 0;
        buf[348] = 0; buf[349] = 0; buf[350] = 0; buf[351] = 0;
        buf[352] = -126; buf[353] = 43; buf[354] = 74; buf[355] = 0;
        buf[356] = 0; buf[357] = 0; buf[358] = 0; buf[359] = 0;
        buf[360] = 54; buf[361] = -19; buf[362] = 75; buf[363] = 0;
        buf[364] = 0; buf[365] = 0; buf[366] = 0; buf[367] = 0;
        buf[368] = 79; buf[369] = 77; buf[370] = 76; buf[371] = 0;
        buf[372] = 0; buf[373] = 0; buf[374] = 0; buf[375] = 0;
        buf[376] = -98; buf[377] = -7; buf[378] = 76; buf[379] = 0;
        buf[380] = 0; buf[381] = 0; buf[382] = 0; buf[383] = 0;
        buf[384] = -119; buf[385] = 75; buf[386] = 77; buf[387] = 0;
        buf[388] = 0; buf[389] = 0; buf[390] = 0; buf[391] = 0;
        buf[392] = -22; buf[393] = 29; buf[394] = 78; buf[395] = 0;
        buf[396] = 0; buf[397] = 0; buf[398] = 0; buf[399] = 0;
        buf[400] = 57; buf[401] = 73; buf[402] = 78; buf[403] = 0;
        buf[404] = 0; buf[405] = 0; buf[406] = 0; buf[407] = 0;
        buf[408] = 57; buf[409] = 0; buf[410] = 86; buf[411] = 0;
        buf[412] = 0; buf[413] = 0; buf[414] = 0; buf[415] = 0;
        buf[416] = 3; buf[417] = -111; buf[418] = 90; buf[419] = 0;
        buf[420] = 0; buf[421] = 0; buf[422] = 0; buf[423] = 0;
        return buf;
    }

    std::vector<char> fileD_list() {
        std::vector<char> buf(296);
        buf[0] = 72; buf[1] = -56; buf[2] = -56; buf[3] = -56;
        buf[4] = -63; buf[5] = -84; buf[6] = -77; buf[7] = -120;
        buf[8] = -92; buf[9] = 18; buf[10] = 55; buf[11] = 51;
        buf[12] = -83; buf[13] = 55; buf[14] = 55; buf[15] = -96;
        buf[16] = -63; buf[17] = -73; buf[18] = 55; buf[19] = -96;
        buf[20] = -125; buf[21] = 124; buf[22] = -54; buf[23] = -105;
        buf[24] = 55; buf[25] = 55; buf[26] = 54; buf[27] = 10;
        buf[28] = 51; buf[29] = -119; buf[30] = -84; buf[31] = 86;
        buf[32] = 15; buf[33] = 0; buf[34] = 0; buf[35] = 0;
        buf[36] = 0; buf[37] = 0; buf[38] = 0; buf[39] = 0;
        buf[40] = 0; buf[41] = 0; buf[42] = 0; buf[43] = 0;
        buf[44] = 0; buf[45] = 0; buf[46] = 0; buf[47] = 0;
        buf[48] = 83; buf[49] = 85; buf[50] = 37; buf[51] = 0;
        buf[52] = 0; buf[53] = 0; buf[54] = 0; buf[55] = 0;
        buf[56] = 0; buf[57] = 0; buf[58] = 0; buf[59] = 0;
        buf[60] = 0; buf[61] = 0; buf[62] = 0; buf[63] = 0;
        buf[64] = -20; buf[65] = -122; buf[66] = 6; buf[67] = 0;
        buf[68] = 0; buf[69] = 0; buf[70] = 0; buf[71] = 0;
        buf[72] = -39; buf[73] = 1; buf[74] = 7; buf[75] = 0;
        buf[76] = 0; buf[77] = 0; buf[78] = 0; buf[79] = 0;
        buf[80] = 63; buf[81] = 104; buf[82] = 7; buf[83] = 0;
        buf[84] = 0; buf[85] = 0; buf[86] = 0; buf[87] = 0;
        buf[88] = 91; buf[89] = -92; buf[90] = 7; buf[91] = 0;
        buf[92] = 0; buf[93] = 0; buf[94] = 0; buf[95] = 0;
        buf[96] = 70; buf[97] = -10; buf[98] = 7; buf[99] = 0;
        buf[100] = 0; buf[101] = 0; buf[102] = 0; buf[103] = 0;
        buf[104] = -17; buf[105] = 86; buf[106] = 8; buf[107] = 0;
        buf[108] = 0; buf[109] = 0; buf[110] = 0; buf[111] = 0;
        buf[112] = -89; buf[113] = -115; buf[114] = 8; buf[115] = 0;
        buf[116] = 0; buf[117] = 0; buf[118] = 0; buf[119] = 0;
        buf[120] = 42; buf[121] = 101; buf[122] = 10; buf[123] = 0;
        buf[124] = 0; buf[125] = 0; buf[126] = 0; buf[127] = 0;
        buf[128] = 0; buf[129] = -82; buf[130] = 10; buf[131] = 0;
        buf[132] = 0; buf[133] = 0; buf[134] = 0; buf[135] = 0;
        buf[136] = -126; buf[137] = -119; buf[138] = 12; buf[139] = 0;
        buf[140] = 0; buf[141] = 0; buf[142] = 0; buf[143] = 0;
        buf[144] = 83; buf[145] = 126; buf[146] = 14; buf[147] = 0;
        buf[148] = 0; buf[149] = 0; buf[150] = 0; buf[151] = 0;
        buf[152] = -97; buf[153] = -35; buf[154] = 21; buf[155] = 0;
        buf[156] = 0; buf[157] = 0; buf[158] = 0; buf[159] = 0;
        buf[160] = 34; buf[161] = 24; buf[162] = 22; buf[163] = 0;
        buf[164] = 0; buf[165] = 0; buf[166] = 0; buf[167] = 0;
        buf[168] = -6; buf[169] = 35; buf[170] = 22; buf[171] = 0;
        buf[172] = 0; buf[173] = 0; buf[174] = 0; buf[175] = 0;
        buf[176] = 96; buf[177] = -118; buf[178] = 22; buf[179] = 0;
        buf[180] = 0; buf[181] = 0; buf[182] = 0; buf[183] = 0;
        buf[184] = 64; buf[185] = -103; buf[186] = 22; buf[187] = 0;
        buf[188] = 0; buf[189] = 0; buf[190] = 0; buf[191] = 0;
        buf[192] = -10; buf[193] = 63; buf[194] = 23; buf[195] = 0;
        buf[196] = 0; buf[197] = 0; buf[198] = 0; buf[199] = 0;
        buf[200] = -20; buf[201] = -61; buf[202] = 25; buf[203] = 0;
        buf[204] = 0; buf[205] = 0; buf[206] = 0; buf[207] = 0;
        buf[208] = 118; buf[209] = 41; buf[210] = 28; buf[211] = 0;
        buf[212] = 0; buf[213] = 0; buf[214] = 0; buf[215] = 0;
        buf[216] = 51; buf[217] = 42; buf[218] = 28; buf[219] = 0;
        buf[220] = 0; buf[221] = 0; buf[222] = 0; buf[223] = 0;
        buf[224] = 68; buf[225] = 66; buf[226] = 28; buf[227] = 0;
        buf[228] = 0; buf[229] = 0; buf[230] = 0; buf[231] = 0;
        buf[232] = -76; buf[233] = 70; buf[234] = 28; buf[235] = 0;
        buf[236] = 0; buf[237] = 0; buf[238] = 0; buf[239] = 0;
        buf[240] = 41; buf[241] = 111; buf[242] = 28; buf[243] = 0;
        buf[244] = 0; buf[245] = 0; buf[246] = 0; buf[247] = 0;
        buf[248] = -19; buf[249] = -99; buf[250] = 28; buf[251] = 0;
        buf[252] = 0; buf[253] = 0; buf[254] = 0; buf[255] = 0;
        buf[256] = 83; buf[257] = 4; buf[258] = 29; buf[259] = 0;
        buf[260] = 0; buf[261] = 0; buf[262] = 0; buf[263] = 0;
        buf[264] = -29; buf[265] = 38; buf[266] = 29; buf[267] = 0;
        buf[268] = 0; buf[269] = 0; buf[270] = 0; buf[271] = 0;
        buf[272] = 73; buf[273] = -115; buf[274] = 29; buf[275] = 0;
        buf[276] = 0; buf[277] = 0; buf[278] = 0; buf[279] = 0;
        buf[280] = -43; buf[281] = -30; buf[282] = 29; buf[283] = 0;
        buf[284] = 0; buf[285] = 0; buf[286] = 0; buf[287] = 0;
        buf[288] = -106; buf[289] = 78; buf[290] = 37; buf[291] = 0;
        buf[292] = 0; buf[293] = 0; buf[294] = 0; buf[295] = 0;
        return buf;
    }
}

TEST_CASE("async_downloader_dummy","[async_downloader]") {
    auto downloader = testDownloader();
    typedef SafeLists::Interval Int;
    typedef SafeLists::IntervalList IntList;
    typedef SafeLists::AsyncDownloader AD;

    std::unique_ptr< std::promise<void> > prom( new std::promise<void> );
    auto future = prom->get_future();
    auto handler =
        std::make_shared< SafeLists::MessageableMatchFunctor >(
            SF::virtualMatchFunctorPtr(
                SF::virtualMatch< AD::OutDownloadFinished >(
                    [&](AD::OutDownloadFinished) {
                        prom->set_value();
                    }
                )
            )
        );

    std::unique_ptr< bool > success( new bool(true) );
    std::unique_ptr< IntList > toDownload(new IntList(Int(0,1024 * 1024))); // 1MB
    IntList copy = toDownload->clone();
    auto downloadJob = SF::vpackPtr<
        AD::ScheduleDownload,
        IntList,
        std::function< bool(const char*,int64_t,int64_t) >,
        std::weak_ptr< Messageable >
    >(
        nullptr,
        std::move(copy),
        [&](const char* buffer,int64_t start,int64_t end) {
            auto size = end - start;
            *success &= ensureExpected(size,buffer);
            toDownload->append(Int(start,end));
            return true;
        },
        handler
    );

    downloader->message(downloadJob);

    future.wait();

    *success &= toDownload->isFilled();

    REQUIRE( *success );
}

namespace {
    bool isFileGood(const char* path,int64_t size) {
        std::ifstream in(path,std::ios::binary);
        int64_t count = 0;
        char ch;
        while (in.get(ch)) {
            if (ch != '7')
                return false;
            ++count;
        }
        return count == size;
    }
}

TEST_CASE("safelist_downloader_example_download","[safelist_downloader]") {
    const char* dlPath = "downloadtest1";
    std::string dlPathAbs = dlPath;
    dlPathAbs += "/safelist_session";
    namespace fs = boost::filesystem;
    fs::remove_all(dlPath);
    fs::create_directory(dlPath);
    fs::copy_file("exampleData/dlsessions/2/safelist_session",dlPathAbs);

    typedef SafeLists::SafeListDownloader SLD;

    std::unique_ptr< std::promise<void> > promPtr(
        new std::promise<void>()
    );
    auto rawCopy = promPtr.get();
    auto future = promPtr->get_future();
    auto downloader = testDownloader();
    auto writer = SafeLists::RandomFileWriter::make();
    auto notifier = std::make_shared<
        SafeLists::MessageableMatchFunctor
    >(
        SF::virtualMatchFunctorPtr(
            SF::virtualMatch< SLD::OutDone >(
                [=](SLD::OutDone) {
                    rawCopy->set_value();
                }
            )
        )
    );

    auto handle =
        SLD::startNew(dlPathAbs.c_str(),writer,downloader,notifier);

    future.wait();
    auto finishWriting = SF::vpackPtrCustom<
        templatious::VPACK_WAIT,
        SafeLists::RandomFileWriter::WaitWrites
    >(nullptr);
    writer->message(finishWriting);
    finishWriting->wait();

    bool result = true;
    result &= isFileGood("downloadtest1/fileA",1048576);
    REQUIRE( result );
    result &= isFileGood("downloadtest1/fileB",2097152);
    REQUIRE( result );
    result &= isFileGood("downloadtest1/fileC",349525);
    REQUIRE( result );
    result &= isFileGood("downloadtest1/fileD",2446675);
    REQUIRE( result );
    result &= isFileGood("downloadtest1/fileE",5941925);
    REQUIRE( result );
    result &= isFileGood("downloadtest1/fldA/fileF",9437175);
    REQUIRE( result );
    result &= isFileGood("downloadtest1/fldA/fileG",77777);
    REQUIRE( result );
}

struct EnsureContTrack {
    int _row;
    bool _value;
};

int ensureContCallback(void* data,int count,char** values,char** header) {
    EnsureContTrack& track = *reinterpret_cast<EnsureContTrack*>(data);

    track._value &= count == 6;
    if (!track._value) return 1;

    ++track._row;

    char buf[64];
    sprintf(buf,"%d",track._row);

    char aLetter = 'a' + ((track._row - 1) / 26);
    char bLetter = 'a' + ((track._row - 1) % 26);

    track._value &= 0 == strcmp(values[0],buf);
    sprintf(buf,"%c%c.txt",aLetter,bLetter);
    track._value &= 0 == strcmp(values[1],buf);
    track._value &= 0 == strcmp(values[2],"7");
    track._value &= 0 == strcmp(values[3],"");
    track._value &= 0 == strcmp(values[4],"0");
    track._value &= 0 == strcmp(values[5],"10");

    return track._value ? 0 : 1;
}

int ensureContCallback2(void* data,int count,char** values,char** header) {
    EnsureContTrack& track = *reinterpret_cast<EnsureContTrack*>(data);

    track._value &= count == 3;
    if (!track._value) return 1;

    ++track._row;

    char buf[64];
    sprintf(buf,"%d",track._row);
    track._value &= 0 == strcmp(values[0],buf);
    track._value &= 0 == strcmp(values[1],"test");
    track._value &= 0 == strcmp(values[2],"0");

    return track._value ? 0 : 1;
}

bool ensureContentsOfExample2Session(const char* path) {
    sqlite3* sess = nullptr;
    sqlite3_open(path,&sess);
    if (nullptr == sess) {
        return false;
    }

    auto guard = SCOPE_GUARD_LC(
        sqlite3_close(sess);
    );

    EnsureContTrack t;
    t._row = 0;
    t._value = true;

    char* errMsg = nullptr;
    int res = sqlite3_exec(
        sess,
        "SELECT * FROM to_download;",
        &ensureContCallback,
        &t,
        &errMsg
    );

    if (res != SQLITE_OK || errMsg != nullptr) {
        return false;
    }

    t._value &= t._row == 676;
    t._row = 0;

    res = sqlite3_exec(
        sess,
        "SELECT * FROM mirrors;",
        &ensureContCallback2,
        &t,
        &errMsg
    );

    if (res != SQLITE_OK || errMsg != nullptr) {
        return false;
    }

    t._value &= t._row == 676;

    return t._value;
}

TEST_CASE("safelist_create_session","[safelist_downloader]") {
    const char* dlPath = "downloadtest1";
    std::string dlPathAbs = dlPath;
    dlPathAbs += "/safelist_session";
    namespace fs = boost::filesystem;
    fs::remove_all(dlPath);
    fs::create_directory(dlPath);

    using namespace SafeLists;

    auto asyncSqlite = AsyncSqlite::createNew(
        "exampleData/example2.safelist");

    std::unique_ptr< std::promise<void> > prPtr(
        new std::promise<void>
    );
    auto future = prPtr->get_future();
    auto rawPrPtr = prPtr.get();
    typedef SafeLists::SafeListDownloaderFactory SLDF;
    auto handler = std::make_shared< MessageableMatchFunctorWAsync >(
        SF::virtualMatchFunctorPtr(
            SF::virtualMatch< SLDF::OutCreateSessionDone >(
                [=](SLDF::OutCreateSessionDone) {
                    rawPrPtr->set_value();
                }
            )
        )
    );
    auto sldf = SLDF::createNew();
    auto msg = SF::vpack<
        SLDF::CreateSession,
        StrongMsgPtr,
        StrongMsgPtr,
        std::string
    >(
        nullptr,
        asyncSqlite,
        handler,
        dlPathAbs
    );
    sldf->message(msg);
    future.wait();

    bool res = ensureContentsOfExample2Session(dlPathAbs.c_str());
    REQUIRE( res );
}

TEST_CASE("safelist_partial_download","[safelist_downloader]") {
    const char* dlPath = "downloadtest1";
    std::string dlPathAbs = dlPath;
    dlPathAbs += "/safelist_session";
    namespace fs = boost::filesystem;
    fs::remove_all(dlPath);
    fs::create_directory(dlPath);
    fs::copy_file("exampleData/dlsessions/2/safelist_session",dlPathAbs);

    {
        sqlite3* sess = nullptr;
        auto out = sqlite3_open(dlPathAbs.c_str(),&sess);
        auto guard = SCOPE_GUARD_LC( sqlite3_close(sess); );
        sqlite3_exec(
            sess,
            "UPDATE to_download SET status=2;"
            " UPDATE to_download SET status=1 WHERE id=1 OR id=7;", nullptr,
            nullptr,
            nullptr
        );
    }

    typedef SafeLists::SafeListDownloader SLD;

    std::unique_ptr< std::promise<void> > promPtr(
        new std::promise<void>()
    );
    auto rawCopy = promPtr.get();
    auto future = promPtr->get_future();
    auto downloader = testDownloader();
    auto writer = SafeLists::RandomFileWriter::make();
    auto notifier = std::make_shared<
        SafeLists::MessageableMatchFunctor
    >(
        SF::virtualMatchFunctorPtr(
            SF::virtualMatch< SLD::OutDone >(
                [=](SLD::OutDone) {
                    rawCopy->set_value();
                }
            )
        )
    );

    auto handle =
        SLD::startNew(dlPathAbs.c_str(),writer,downloader,notifier);

    future.wait();
    auto finishWriting = SF::vpackPtrCustom<
        templatious::VPACK_WAIT,
        SafeLists::RandomFileWriter::WaitWrites
    >(nullptr);
    writer->message(finishWriting);
    finishWriting->wait();

    bool result = true;
    result &= isFileGood("downloadtest1/fileA",1048576);
    REQUIRE( result );
    result &= !fs::exists("downloadtest1/fileB");
    REQUIRE( result );
    result &= !fs::exists("downloadtest1/fileC");
    REQUIRE( result );
    result &= !fs::exists("downloadtest1/fileD");
    REQUIRE( result );
    result &= !fs::exists("downloadtest1/fileE");
    REQUIRE( result );
    result &= !fs::exists("downloadtest1/fldA/fileF");
    REQUIRE( result );
    result &= isFileGood("downloadtest1/fldA/fileG",77777);
    REQUIRE( result );
}
