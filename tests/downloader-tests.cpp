
#include <fstream>

#include <templatious/FullPack.hpp>
#include <templatious/detail/DynamicVirtualMatchFunctor.hpp>
#include <boost/filesystem.hpp>
#include <io/AsyncDownloader.hpp>
#include <io/SafeListDownloader.hpp>
#include <io/SafeListDownloaderInternal.hpp>
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

    void writeList(const char* path,const std::vector<char>& list) {
        auto file = fopen(path,"w");
        auto g = SCOPE_GUARD_LC(
            fclose(file);
        );

        fwrite(list.data(),SA::size(list),1,file);
    }

    std::vector<char> readFile(const char* path) {
        std::vector<char> res;
        res.reserve(1024 * 1024 * 8);
        std::ifstream file(path,std::ios::binary);
        assert( file.is_open() );
        char c;
        while (file.get(c)) {
            SA::add(res,c);
        }
        return res;
    }

    void writeUniform(const char* path,int64_t size,char c) {
        std::ofstream ofs(path,std::ios::binary);
        TEMPLATIOUS_REPEAT(size) {
            ofs.put(c);
        }
    }

    bool fileIntervalTest(
        const char* path,
        char full,char empty,
        const SafeLists::IntervalList& lst)
    {
        auto file = readFile(path);
        bool isGood = true;

        if (lst.range().size() != SA::size(file)) {
            return false;
        }

        lst.traverseFilled(
            [&](const SafeLists::Interval& i) {
                auto seq = SF::seqL<int64_t>(i.start(),i.end());
                TEMPLATIOUS_FOREACH(auto i,seq) {
                    bool curr = file[i] == full;
                    isGood &= curr;
                }
                return true;
            }
        );

        lst.traverseEmpty(
            [&](const SafeLists::Interval& i) {
                auto seq = SF::seqL<int64_t>(i.start(),i.end());
                TEMPLATIOUS_FOREACH(auto i,seq) {
                    bool curr = file[i] == empty;
                    isGood &= curr;
                }
                return true;
            }
        );

        return isGood;
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

    std::vector<char> fileB_ilist() {
        std::vector<char> buf(312);
        buf[0] = -56; buf[1] = -56; buf[2] = -56; buf[3] = -52;
        buf[4] = 119; buf[5] = 91; buf[6] = -5; buf[7] = -83;
        buf[8] = 55; buf[9] = 55; buf[10] = 36; buf[11] = -25;
        buf[12] = -123; buf[13] = 55; buf[14] = 55; buf[15] = -96;
        buf[16] = -38; buf[17] = 55; buf[18] = -95; buf[19] = -18;
        buf[20] = 65; buf[21] = -77; buf[22] = -120; buf[23] = -52;
        buf[24] = 124; buf[25] = 74; buf[26] = -120; buf[27] = -56;
        buf[28] = -56; buf[29] = 77; buf[30] = -56; buf[31] = -56;
        buf[32] = 16; buf[33] = 0; buf[34] = 0; buf[35] = 0;
        buf[36] = 0; buf[37] = 0; buf[38] = 0; buf[39] = 0;
        buf[40] = 0; buf[41] = 0; buf[42] = 0; buf[43] = 0;
        buf[44] = 0; buf[45] = 0; buf[46] = 0; buf[47] = 0;
        buf[48] = 0; buf[49] = 0; buf[50] = 32; buf[51] = 0;
        buf[52] = 0; buf[53] = 0; buf[54] = 0; buf[55] = 0;
        buf[56] = 0; buf[57] = 0; buf[58] = 0; buf[59] = 0;
        buf[60] = 0; buf[61] = 0; buf[62] = 0; buf[63] = 0;
        buf[64] = -53; buf[65] = 28; buf[66] = 4; buf[67] = 0;
        buf[68] = 0; buf[69] = 0; buf[70] = 0; buf[71] = 0;
        buf[72] = 81; buf[73] = 66; buf[74] = 4; buf[75] = 0;
        buf[76] = 0; buf[77] = 0; buf[78] = 0; buf[79] = 0;
        buf[80] = -26; buf[81] = -128; buf[82] = 4; buf[83] = 0;
        buf[84] = 0; buf[85] = 0; buf[86] = 0; buf[87] = 0;
        buf[88] = -62; buf[89] = -60; buf[90] = 4; buf[91] = 0;
        buf[92] = 0; buf[93] = 0; buf[94] = 0; buf[95] = 0;
        buf[96] = 56; buf[97] = -32; buf[98] = 4; buf[99] = 0;
        buf[100] = 0; buf[101] = 0; buf[102] = 0; buf[103] = 0;
        buf[104] = 96; buf[105] = -26; buf[106] = 4; buf[107] = 0;
        buf[108] = 0; buf[109] = 0; buf[110] = 0; buf[111] = 0;
        buf[112] = 75; buf[113] = 56; buf[114] = 5; buf[115] = 0;
        buf[116] = 0; buf[117] = 0; buf[118] = 0; buf[119] = 0;
        buf[120] = -60; buf[121] = -71; buf[122] = 5; buf[123] = 0;
        buf[124] = 0; buf[125] = 0; buf[126] = 0; buf[127] = 0;
        buf[128] = -127; buf[129] = -119; buf[130] = 6; buf[131] = 0;
        buf[132] = 0; buf[133] = 0; buf[134] = 0; buf[135] = 0;
        buf[136] = 39; buf[137] = -114; buf[138] = 6; buf[139] = 0;
        buf[140] = 0; buf[141] = 0; buf[142] = 0; buf[143] = 0;
        buf[144] = -114; buf[145] = -12; buf[146] = 6; buf[147] = 0;
        buf[148] = 0; buf[149] = 0; buf[150] = 0; buf[151] = 0;
        buf[152] = 44; buf[153] = 33; buf[154] = 7; buf[155] = 0;
        buf[156] = 0; buf[157] = 0; buf[158] = 0; buf[159] = 0;
        buf[160] = 75; buf[161] = 69; buf[162] = 7; buf[163] = 0;
        buf[164] = 0; buf[165] = 0; buf[166] = 0; buf[167] = 0;
        buf[168] = 40; buf[169] = 75; buf[170] = 7; buf[171] = 0;
        buf[172] = 0; buf[173] = 0; buf[174] = 0; buf[175] = 0;
        buf[176] = -57; buf[177] = -91; buf[178] = 12; buf[179] = 0;
        buf[180] = 0; buf[181] = 0; buf[182] = 0; buf[183] = 0;
        buf[184] = 42; buf[185] = 30; buf[186] = 16; buf[187] = 0;
        buf[188] = 0; buf[189] = 0; buf[190] = 0; buf[191] = 0;
        buf[192] = -107; buf[193] = 124; buf[194] = 16; buf[195] = 0;
        buf[196] = 0; buf[197] = 0; buf[198] = 0; buf[199] = 0;
        buf[200] = 85; buf[201] = -94; buf[202] = 16; buf[203] = 0;
        buf[204] = 0; buf[205] = 0; buf[206] = 0; buf[207] = 0;
        buf[208] = -69; buf[209] = 8; buf[210] = 17; buf[211] = 0;
        buf[212] = 0; buf[213] = 0; buf[214] = 0; buf[215] = 0;
        buf[216] = -94; buf[217] = -50; buf[218] = 18; buf[219] = 0;
        buf[220] = 0; buf[221] = 0; buf[222] = 0; buf[223] = 0;
        buf[224] = 51; buf[225] = -73; buf[226] = 19; buf[227] = 0;
        buf[228] = 0; buf[229] = 0; buf[230] = 0; buf[231] = 0;
        buf[232] = 27; buf[233] = 126; buf[234] = 20; buf[235] = 0;
        buf[236] = 0; buf[237] = 0; buf[238] = 0; buf[239] = 0;
        buf[240] = -23; buf[241] = -9; buf[242] = 22; buf[243] = 0;
        buf[244] = 0; buf[245] = 0; buf[246] = 0; buf[247] = 0;
        buf[248] = -55; buf[249] = 101; buf[250] = 23; buf[251] = 0;
        buf[252] = 0; buf[253] = 0; buf[254] = 0; buf[255] = 0;
        buf[256] = -102; buf[257] = 104; buf[258] = 23; buf[259] = 0;
        buf[260] = 0; buf[261] = 0; buf[262] = 0; buf[263] = 0;
        buf[264] = 46; buf[265] = -22; buf[266] = 23; buf[267] = 0;
        buf[268] = 0; buf[269] = 0; buf[270] = 0; buf[271] = 0;
        buf[272] = 94; buf[273] = 27; buf[274] = 24; buf[275] = 0;
        buf[276] = 0; buf[277] = 0; buf[278] = 0; buf[279] = 0;
        buf[280] = 118; buf[281] = -3; buf[282] = 24; buf[283] = 0;
        buf[284] = 0; buf[285] = 0; buf[286] = 0; buf[287] = 0;
        buf[288] = -85; buf[289] = 100; buf[290] = 26; buf[291] = 0;
        buf[292] = 0; buf[293] = 0; buf[294] = 0; buf[295] = 0;
        buf[296] = 42; buf[297] = 102; buf[298] = 27; buf[299] = 0;
        buf[300] = 0; buf[301] = 0; buf[302] = 0; buf[303] = 0;
        buf[304] = 0; buf[305] = 0; buf[306] = 32; buf[307] = 0;
        buf[308] = 0; buf[309] = 0; buf[310] = 0; buf[311] = 0;
        return buf;
    }

    std::vector<char> fileF_list() {
        std::vector<char> buf(872);
        buf[0] = -26; buf[1] = 7; buf[2] = -82; buf[3] = 101;
        buf[4] = -21; buf[5] = -46; buf[6] = -20; buf[7] = 95;
        buf[8] = 126; buf[9] = -50; buf[10] = 54; buf[11] = 29;
        buf[12] = 31; buf[13] = -3; buf[14] = 89; buf[15] = 124;
        buf[16] = -126; buf[17] = 9; buf[18] = 11; buf[19] = 7;
        buf[20] = 124; buf[21] = -10; buf[22] = -110; buf[23] = -18;
        buf[24] = 69; buf[25] = 64; buf[26] = -10; buf[27] = 57;
        buf[28] = 70; buf[29] = 89; buf[30] = 75; buf[31] = -121;
        buf[32] = 51; buf[33] = 0; buf[34] = 0; buf[35] = 0;
        buf[36] = 0; buf[37] = 0; buf[38] = 0; buf[39] = 0;
        buf[40] = 0; buf[41] = 0; buf[42] = 0; buf[43] = 0;
        buf[44] = 0; buf[45] = 0; buf[46] = 0; buf[47] = 0;
        buf[48] = -9; buf[49] = -1; buf[50] = -113; buf[51] = 0;
        buf[52] = 0; buf[53] = 0; buf[54] = 0; buf[55] = 0;
        buf[56] = 0; buf[57] = 0; buf[58] = 0; buf[59] = 0;
        buf[60] = 0; buf[61] = 0; buf[62] = 0; buf[63] = 0;
        buf[64] = 57; buf[65] = -119; buf[66] = 1; buf[67] = 0;
        buf[68] = 0; buf[69] = 0; buf[70] = 0; buf[71] = 0;
        buf[72] = 121; buf[73] = -115; buf[74] = 1; buf[75] = 0;
        buf[76] = 0; buf[77] = 0; buf[78] = 0; buf[79] = 0;
        buf[80] = 78; buf[81] = 73; buf[82] = 2; buf[83] = 0;
        buf[84] = 0; buf[85] = 0; buf[86] = 0; buf[87] = 0;
        buf[88] = -112; buf[89] = -74; buf[90] = 4; buf[91] = 0;
        buf[92] = 0; buf[93] = 0; buf[94] = 0; buf[95] = 0;
        buf[96] = -80; buf[97] = -28; buf[98] = 4; buf[99] = 0;
        buf[100] = 0; buf[101] = 0; buf[102] = 0; buf[103] = 0;
        buf[104] = 53; buf[105] = 93; buf[106] = 6; buf[107] = 0;
        buf[108] = 0; buf[109] = 0; buf[110] = 0; buf[111] = 0;
        buf[112] = 122; buf[113] = -95; buf[114] = 6; buf[115] = 0;
        buf[116] = 0; buf[117] = 0; buf[118] = 0; buf[119] = 0;
        buf[120] = 2; buf[121] = -37; buf[122] = 9; buf[123] = 0;
        buf[124] = 0; buf[125] = 0; buf[126] = 0; buf[127] = 0;
        buf[128] = -123; buf[129] = 21; buf[130] = 10; buf[131] = 0;
        buf[132] = 0; buf[133] = 0; buf[134] = 0; buf[135] = 0;
        buf[136] = -53; buf[137] = -73; buf[138] = 11; buf[139] = 0;
        buf[140] = 0; buf[141] = 0; buf[142] = 0; buf[143] = 0;
        buf[144] = -72; buf[145] = 9; buf[146] = 12; buf[147] = 0;
        buf[148] = 0; buf[149] = 0; buf[150] = 0; buf[151] = 0;
        buf[152] = -40; buf[153] = -120; buf[154] = 12; buf[155] = 0;
        buf[156] = 0; buf[157] = 0; buf[158] = 0; buf[159] = 0;
        buf[160] = 91; buf[161] = -61; buf[162] = 12; buf[163] = 0;
        buf[164] = 0; buf[165] = 0; buf[166] = 0; buf[167] = 0;
        buf[168] = -15; buf[169] = 123; buf[170] = 13; buf[171] = 0;
        buf[172] = 0; buf[173] = 0; buf[174] = 0; buf[175] = 0;
        buf[176] = 43; buf[177] = -67; buf[178] = 13; buf[179] = 0;
        buf[180] = 0; buf[181] = 0; buf[182] = 0; buf[183] = 0;
        buf[184] = 13; buf[185] = 23; buf[186] = 16; buf[187] = 0;
        buf[188] = 0; buf[189] = 0; buf[190] = 0; buf[191] = 0;
        buf[192] = 71; buf[193] = 52; buf[194] = 16; buf[195] = 0;
        buf[196] = 0; buf[197] = 0; buf[198] = 0; buf[199] = 0;
        buf[200] = -108; buf[201] = -17; buf[202] = 18; buf[203] = 0;
        buf[204] = 0; buf[205] = 0; buf[206] = 0; buf[207] = 0;
        buf[208] = 127; buf[209] = 65; buf[210] = 19; buf[211] = 0;
        buf[212] = 0; buf[213] = 0; buf[214] = 0; buf[215] = 0;
        buf[216] = 41; buf[217] = 69; buf[218] = 21; buf[219] = 0;
        buf[220] = 0; buf[221] = 0; buf[222] = 0; buf[223] = 0;
        buf[224] = -112; buf[225] = -85; buf[226] = 21; buf[227] = 0;
        buf[228] = 0; buf[229] = 0; buf[230] = 0; buf[231] = 0;
        buf[232] = -119; buf[233] = -19; buf[234] = 22; buf[235] = 0;
        buf[236] = 0; buf[237] = 0; buf[238] = 0; buf[239] = 0;
        buf[240] = 44; buf[241] = 63; buf[242] = 23; buf[243] = 0;
        buf[244] = 0; buf[245] = 0; buf[246] = 0; buf[247] = 0;
        buf[248] = 14; buf[249] = 118; buf[250] = 25; buf[251] = 0;
        buf[252] = 0; buf[253] = 0; buf[254] = 0; buf[255] = 0;
        buf[256] = -5; buf[257] = -57; buf[258] = 25; buf[259] = 0;
        buf[260] = 0; buf[261] = 0; buf[262] = 0; buf[263] = 0;
        buf[264] = 111; buf[265] = -44; buf[266] = 46; buf[267] = 0;
        buf[268] = 0; buf[269] = 0; buf[270] = 0; buf[271] = 0;
        buf[272] = -86; buf[273] = -31; buf[274] = 46; buf[275] = 0;
        buf[276] = 0; buf[277] = 0; buf[278] = 0; buf[279] = 0;
        buf[280] = 121; buf[281] = 113; buf[282] = 56; buf[283] = 0;
        buf[284] = 0; buf[285] = 0; buf[286] = 0; buf[287] = 0;
        buf[288] = -31; buf[289] = 77; buf[290] = 59; buf[291] = 0;
        buf[292] = 0; buf[293] = 0; buf[294] = 0; buf[295] = 0;
        buf[296] = -37; buf[297] = -107; buf[298] = 65; buf[299] = 0;
        buf[300] = 0; buf[301] = 0; buf[302] = 0; buf[303] = 0;
        buf[304] = 66; buf[305] = -4; buf[306] = 65; buf[307] = 0;
        buf[308] = 0; buf[309] = 0; buf[310] = 0; buf[311] = 0;
        buf[312] = -68; buf[313] = 49; buf[314] = 67; buf[315] = 0;
        buf[316] = 0; buf[317] = 0; buf[318] = 0; buf[319] = 0;
        buf[320] = 63; buf[321] = 108; buf[322] = 67; buf[323] = 0;
        buf[324] = 0; buf[325] = 0; buf[326] = 0; buf[327] = 0;
        buf[328] = 112; buf[329] = 27; buf[330] = 69; buf[331] = 0;
        buf[332] = 0; buf[333] = 0; buf[334] = 0; buf[335] = 0;
        buf[336] = 93; buf[337] = 109; buf[338] = 69; buf[339] = 0;
        buf[340] = 0; buf[341] = 0; buf[342] = 0; buf[343] = 0;
        buf[344] = -87; buf[345] = 24; buf[346] = 70; buf[347] = 0;
        buf[348] = 0; buf[349] = 0; buf[350] = 0; buf[351] = 0;
        buf[352] = 16; buf[353] = 127; buf[354] = 70; buf[355] = 0;
        buf[356] = 0; buf[357] = 0; buf[358] = 0; buf[359] = 0;
        buf[360] = 45; buf[361] = 122; buf[362] = 71; buf[363] = 0;
        buf[364] = 0; buf[365] = 0; buf[366] = 0; buf[367] = 0;
        buf[368] = 26; buf[369] = -52; buf[370] = 71; buf[371] = 0;
        buf[372] = 0; buf[373] = 0; buf[374] = 0; buf[375] = 0;
        buf[376] = 32; buf[377] = -38; buf[378] = 75; buf[379] = 0;
        buf[380] = 0; buf[381] = 0; buf[382] = 0; buf[383] = 0;
        buf[384] = 101; buf[385] = 30; buf[386] = 76; buf[387] = 0;
        buf[388] = 0; buf[389] = 0; buf[390] = 0; buf[391] = 0;
        buf[392] = -117; buf[393] = -65; buf[394] = 77; buf[395] = 0;
        buf[396] = 0; buf[397] = 0; buf[398] = 0; buf[399] = 0;
        buf[400] = -67; buf[401] = 15; buf[402] = 78; buf[403] = 0;
        buf[404] = 0; buf[405] = 0; buf[406] = 0; buf[407] = 0;
        buf[408] = -3; buf[409] = -48; buf[410] = 78; buf[411] = 0;
        buf[412] = 0; buf[413] = 0; buf[414] = 0; buf[415] = 0;
        buf[416] = 99; buf[417] = 55; buf[418] = 79; buf[419] = 0;
        buf[420] = 0; buf[421] = 0; buf[422] = 0; buf[423] = 0;
        buf[424] = -37; buf[425] = 122; buf[426] = 79; buf[427] = 0;
        buf[428] = 0; buf[429] = 0; buf[430] = 0; buf[431] = 0;
        buf[432] = -56; buf[433] = -116; buf[434] = 79; buf[435] = 0;
        buf[436] = 0; buf[437] = 0; buf[438] = 0; buf[439] = 0;
        buf[440] = -54; buf[441] = -14; buf[442] = 79; buf[443] = 0;
        buf[444] = 0; buf[445] = 0; buf[446] = 0; buf[447] = 0;
        buf[448] = 106; buf[449] = 37; buf[450] = 80; buf[451] = 0;
        buf[452] = 0; buf[453] = 0; buf[454] = 0; buf[455] = 0;
        buf[456] = 44; buf[457] = 124; buf[458] = 98; buf[459] = 0;
        buf[460] = 0; buf[461] = 0; buf[462] = 0; buf[463] = 0;
        buf[464] = -109; buf[465] = -30; buf[466] = 98; buf[467] = 0;
        buf[468] = 0; buf[469] = 0; buf[470] = 0; buf[471] = 0;
        buf[472] = 53; buf[473] = 52; buf[474] = 102; buf[475] = 0;
        buf[476] = 0; buf[477] = 0; buf[478] = 0; buf[479] = 0;
        buf[480] = 122; buf[481] = 120; buf[482] = 102; buf[483] = 0;
        buf[484] = 0; buf[485] = 0; buf[486] = 0; buf[487] = 0;
        buf[488] = -3; buf[489] = -113; buf[490] = 107; buf[491] = 0;
        buf[492] = 0; buf[493] = 0; buf[494] = 0; buf[495] = 0;
        buf[496] = -24; buf[497] = -31; buf[498] = 107; buf[499] = 0;
        buf[500] = 0; buf[501] = 0; buf[502] = 0; buf[503] = 0;
        buf[504] = 66; buf[505] = 117; buf[506] = 111; buf[507] = 0;
        buf[508] = 0; buf[509] = 0; buf[510] = 0; buf[511] = 0;
        buf[512] = -88; buf[513] = -37; buf[514] = 111; buf[515] = 0;
        buf[516] = 0; buf[517] = 0; buf[518] = 0; buf[519] = 0;
        buf[520] = 57; buf[521] = 52; buf[522] = 115; buf[523] = 0;
        buf[524] = 0; buf[525] = 0; buf[526] = 0; buf[527] = 0;
        buf[528] = -97; buf[529] = -102; buf[530] = 115; buf[531] = 0;
        buf[532] = 0; buf[533] = 0; buf[534] = 0; buf[535] = 0;
        buf[536] = -25; buf[537] = 85; buf[538] = 116; buf[539] = 0;
        buf[540] = 0; buf[541] = 0; buf[542] = 0; buf[543] = 0;
        buf[544] = -1; buf[545] = -126; buf[546] = 116; buf[547] = 0;
        buf[548] = 0; buf[549] = 0; buf[550] = 0; buf[551] = 0;
        buf[552] = -33; buf[553] = -85; buf[554] = 119; buf[555] = 0;
        buf[556] = 0; buf[557] = 0; buf[558] = 0; buf[559] = 0;
        buf[560] = 36; buf[561] = -16; buf[562] = 119; buf[563] = 0;
        buf[564] = 0; buf[565] = 0; buf[566] = 0; buf[567] = 0;
        buf[568] = 9; buf[569] = 68; buf[570] = 121; buf[571] = 0;
        buf[572] = 0; buf[573] = 0; buf[574] = 0; buf[575] = 0;
        buf[576] = 78; buf[577] = -120; buf[578] = 121; buf[579] = 0;
        buf[580] = 0; buf[581] = 0; buf[582] = 0; buf[583] = 0;
        buf[584] = 106; buf[585] = -11; buf[586] = 123; buf[587] = 0;
        buf[588] = 0; buf[589] = 0; buf[590] = 0; buf[591] = 0;
        buf[592] = 85; buf[593] = 71; buf[594] = 124; buf[595] = 0;
        buf[596] = 0; buf[597] = 0; buf[598] = 0; buf[599] = 0;
        buf[600] = 104; buf[601] = 88; buf[602] = 125; buf[603] = 0;
        buf[604] = 0; buf[605] = 0; buf[606] = 0; buf[607] = 0;
        buf[608] = 85; buf[609] = -86; buf[610] = 125; buf[611] = 0;
        buf[612] = 0; buf[613] = 0; buf[614] = 0; buf[615] = 0;
        buf[616] = 95; buf[617] = 21; buf[618] = -127; buf[619] = 0;
        buf[620] = 0; buf[621] = 0; buf[622] = 0; buf[623] = 0;
        buf[624] = 98; buf[625] = 2; buf[626] = -126; buf[627] = 0;
        buf[628] = 0; buf[629] = 0; buf[630] = 0; buf[631] = 0;
        buf[632] = 19; buf[633] = 91; buf[634] = -126; buf[635] = 0;
        buf[636] = 0; buf[637] = 0; buf[638] = 0; buf[639] = 0;
        buf[640] = 26; buf[641] = -39; buf[642] = -124; buf[643] = 0;
        buf[644] = 0; buf[645] = 0; buf[646] = 0; buf[647] = 0;
        buf[648] = 2; buf[649] = -121; buf[650] = -122; buf[651] = 0;
        buf[652] = 0; buf[653] = 0; buf[654] = 0; buf[655] = 0;
        buf[656] = -17; buf[657] = -40; buf[658] = -122; buf[659] = 0;
        buf[660] = 0; buf[661] = 0; buf[662] = 0; buf[663] = 0;
        buf[664] = -107; buf[665] = 6; buf[666] = -121; buf[667] = 0;
        buf[668] = 0; buf[669] = 0; buf[670] = 0; buf[671] = 0;
        buf[672] = 35; buf[673] = 54; buf[674] = -121; buf[675] = 0;
        buf[676] = 0; buf[677] = 0; buf[678] = 0; buf[679] = 0;
        buf[680] = 47; buf[681] = 67; buf[682] = -121; buf[683] = 0;
        buf[684] = 0; buf[685] = 0; buf[686] = 0; buf[687] = 0;
        buf[688] = -107; buf[689] = -87; buf[690] = -121; buf[691] = 0;
        buf[692] = 0; buf[693] = 0; buf[694] = 0; buf[695] = 0;
        buf[696] = 85; buf[697] = -79; buf[698] = -121; buf[699] = 0;
        buf[700] = 0; buf[701] = 0; buf[702] = 0; buf[703] = 0;
        buf[704] = 7; buf[705] = -45; buf[706] = -121; buf[707] = 0;
        buf[708] = 0; buf[709] = 0; buf[710] = 0; buf[711] = 0;
        buf[712] = 25; buf[713] = 53; buf[714] = -120; buf[715] = 0;
        buf[716] = 0; buf[717] = 0; buf[718] = 0; buf[719] = 0;
        buf[720] = -107; buf[721] = 95; buf[722] = -120; buf[723] = 0;
        buf[724] = 0; buf[725] = 0; buf[726] = 0; buf[727] = 0;
        buf[728] = 41; buf[729] = 107; buf[730] = -120; buf[731] = 0;
        buf[732] = 0; buf[733] = 0; buf[734] = 0; buf[735] = 0;
        buf[736] = -113; buf[737] = -47; buf[738] = -120; buf[739] = 0;
        buf[740] = 0; buf[741] = 0; buf[742] = 0; buf[743] = 0;
        buf[744] = -116; buf[745] = 93; buf[746] = -117; buf[747] = 0;
        buf[748] = 0; buf[749] = 0; buf[750] = 0; buf[751] = 0;
        buf[752] = 119; buf[753] = -81; buf[754] = -117; buf[755] = 0;
        buf[756] = 0; buf[757] = 0; buf[758] = 0; buf[759] = 0;
        buf[760] = -115; buf[761] = 27; buf[762] = -116; buf[763] = 0;
        buf[764] = 0; buf[765] = 0; buf[766] = 0; buf[767] = 0;
        buf[768] = -43; buf[769] = 100; buf[770] = -116; buf[771] = 0;
        buf[772] = 0; buf[773] = 0; buf[774] = 0; buf[775] = 0;
        buf[776] = 73; buf[777] = -118; buf[778] = -116; buf[779] = 0;
        buf[780] = 0; buf[781] = 0; buf[782] = 0; buf[783] = 0;
        buf[784] = 54; buf[785] = -36; buf[786] = -116; buf[787] = 0;
        buf[788] = 0; buf[789] = 0; buf[790] = 0; buf[791] = 0;
        buf[792] = -28; buf[793] = -33; buf[794] = -116; buf[795] = 0;
        buf[796] = 0; buf[797] = 0; buf[798] = 0; buf[799] = 0;
        buf[800] = -28; buf[801] = 14; buf[802] = -115; buf[803] = 0;
        buf[804] = 0; buf[805] = 0; buf[806] = 0; buf[807] = 0;
        buf[808] = -16; buf[809] = 21; buf[810] = -115; buf[811] = 0;
        buf[812] = 0; buf[813] = 0; buf[814] = 0; buf[815] = 0;
        buf[816] = -37; buf[817] = 103; buf[818] = -115; buf[819] = 0;
        buf[820] = 0; buf[821] = 0; buf[822] = 0; buf[823] = 0;
        buf[824] = -117; buf[825] = -89; buf[826] = -115; buf[827] = 0;
        buf[828] = 0; buf[829] = 0; buf[830] = 0; buf[831] = 0;
        buf[832] = 6; buf[833] = 84; buf[834] = -114; buf[835] = 0;
        buf[836] = 0; buf[837] = 0; buf[838] = 0; buf[839] = 0;
        buf[840] = -34; buf[841] = -81; buf[842] = -114; buf[843] = 0;
        buf[844] = 0; buf[845] = 0; buf[846] = 0; buf[847] = 0;
        buf[848] = 68; buf[849] = 22; buf[850] = -113; buf[851] = 0;
        buf[852] = 0; buf[853] = 0; buf[854] = 0; buf[855] = 0;
        buf[856] = -60; buf[857] = 117; buf[858] = -113; buf[859] = 0;
        buf[860] = 0; buf[861] = 0; buf[862] = 0; buf[863] = 0;
        buf[864] = -9; buf[865] = -1; buf[866] = -113; buf[867] = 0;
        buf[868] = 0; buf[869] = 0; buf[870] = 0; buf[871] = 0;
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
    auto sldf = SLDF::createNew(testDownloader(),
        SafeLists::RandomFileWriter::make());
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

TEST_CASE("safelist_partial_download_fragments","[safelist_downloader]") {
    const char* dlPath = "downloadtest1";
    std::string dlPathAbs = dlPath;
    dlPathAbs += "/safelist_session";
    namespace fs = boost::filesystem;
    fs::remove_all(dlPath);
    fs::create_directory(dlPath);
    fs::create_directory("downloadtest1/fldA");
    fs::copy_file("exampleData/dlsessions/2/safelist_session",dlPathAbs);

    {
        sqlite3* sess = nullptr;
        auto out = sqlite3_open(dlPathAbs.c_str(),&sess);
        auto guard = SCOPE_GUARD_LC( sqlite3_close(sess); );
        sqlite3_exec(
            sess,
            "UPDATE to_download SET status=2;"
            " UPDATE to_download SET status=1 WHERE id=2 OR id=4 OR id=5 OR id=6;", nullptr,
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

    SafeLists::DumbHash256 bHash;
    SafeLists::DumbHash256 dHash;
    SafeLists::DumbHash256 eHash;
    SafeLists::DumbHash256 fHash;

    writeList("downloadtest1/fileB.ilist",fileB_ilist());
    writeList("downloadtest1/fileD.ilist",fileD_list());
    writeList("downloadtest1/fileE.ilist.tmp",fileE_list());
    writeList("downloadtest1/fldA/fileF.ilist.tmp",fileF_list());
    writeList("downloadtest1/fldA/fileF.ilist",fileF_list());

    auto bList = SafeLists::readIListAndHash("downloadtest1/fileB.ilist",bHash);
    auto dList = SafeLists::readIListAndHash("downloadtest1/fileD.ilist",dHash);
    auto eList = SafeLists::readIListAndHash("downloadtest1/fileE.ilist.tmp",eHash);
    auto fList = SafeLists::readIListAndHash("downloadtest1/fldA/fileF.ilist",fHash);

    writeUniform("downloadtest1/fileB",2097152,7);
    writeUniform("downloadtest1/fileD",2446675,7);
    writeUniform("downloadtest1/fileE",5941925,7);
    writeUniform("downloadtest1/fldA/fileF",9437175,7);

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
    result &= !fs::exists("downloadtest1/fileA");
    REQUIRE( result );
    result &= fileIntervalTest("downloadtest1/fileB",7,'7',bList);
    REQUIRE( result );
    result &= !fs::exists("downloadtest1/fileC");
    REQUIRE( result );
    result &= fileIntervalTest("downloadtest1/fileD",7,'7',dList);
    REQUIRE( result );
    result &= fileIntervalTest("downloadtest1/fileE",7,'7',eList);
    REQUIRE( result );
    result &= fileIntervalTest("downloadtest1/fldA/fileF",7,'7',fList);
    REQUIRE( result );
    result &= !fs::exists("downloadtest1/fldA/fileG");
    REQUIRE( result );
}
