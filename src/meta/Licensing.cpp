
#include <cassert>
#include <iostream>
#include <fstream>
#include <regex>

#include <sodium.h>
#include <libscrypt.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <rapidjson/document.h>
#include <boost/filesystem.hpp>

#include <util/Semaphore.hpp>
#include <util/GenericShutdownGuard.hpp>
#include <util/ScopeGuard.hpp>
#include <util/Base64.hpp>
#include <util/ProgramArgs.hpp>

#include "Licensing.hpp"

TEMPLATIOUS_TRIPLET_STD;

typedef SafeLists::GracefulShutdownInterface GSI;
namespace rj = rapidjson;
namespace fs = boost::filesystem;

namespace SafeLists {

std::string getCurrentUserIdBase64() {
    // dummy implementation, matches play backend test
    return "4j5iMF/54gJW/c4bPAdN28+4YcIeSQj3QGvOv2xIZjU=";
}

std::string getCurrentUserPrivateKeyBase64() {
    // dummy implementation, matches play backend test
    return "y3hQUBVr/dBlrV/3MhgudnDC1t7tur2AondP/"
        "XJwZSXiPmIwX/niAlb9zhs8B03bz7hhwh5JCPdAa86/bEhmNQ==";
}

std::string getServerSignKey() {
    // debug mode, different for production
    return "V9EFZB6xeocuQGW6BYubxVQV8MPtlwKzQnR0+317/xA=";
}

std::string getServerUrl() {
    // for testing
    return "http://127.0.0.1:9000";
}

struct BufStruct {
    char* buf;
    int iter;
    int bufLen;
    int outRes;
};

int curlReadFunc(char* buffer,size_t size,size_t nitems,void* userdata) {
    BufStruct& bfr = *reinterpret_cast<BufStruct*>(userdata);

    assert( size == 1 && "You're kidding me..." );

    size_t written = 0;

    TEMPLATIOUS_0_TO_N(i,nitems) {
        if (bfr.iter < bfr.bufLen) {
            bfr.buf[bfr.iter++] = buffer[i];
            ++written;
        } else {
            bfr.outRes = -1;
            break;
        }
    }

    return written;
}

// TODO: query the pseudonymous server
// behind safe network tunnel.
int querySignature(const std::string& user,const char* thePath,char* out,int& buflen) {
    CURL* handle = ::curl_easy_init();
    auto clean = SCOPE_GUARD_LC(
        ::curl_easy_cleanup(handle);
    );

    BufStruct s;
    s.buf = out;
    s.iter = 0;
    s.bufLen = buflen;
    s.outRes = -2;

    std::string url = getServerUrl();
    url += thePath;
    url += user;

    ::curl_easy_setopt(handle,::CURLOPT_URL,url.c_str());
    ::curl_easy_setopt(handle,::CURLOPT_WRITEFUNCTION,&curlReadFunc);
    ::curl_easy_setopt(handle,::CURLOPT_WRITEDATA,&s);

    auto outRes = ::curl_easy_perform(handle);
    if (::CURLE_OK == outRes) {
        buflen = s.iter;
        s.outRes = 0;
    } else {
        printf("CURL ERROR: |%d|\n",outRes);
    }

    return s.outRes;
}

enum VerificationFailures {
    FORGED_FIRST_TIER = 101,
    MISSING_FIELDS_FIRST_TIER = 102,
    MISTYPED_FIELDS_FIRST_TIER = 103,

    INVALID_BLOB_JSON = 201,
    MISSING_FIELDS_SECOND_TIER = 202,
    MISTYPED_FIELDS_SECOND_TIER = 203,
    FORGED_SIGNATURE_SECOND_TIER = 204,

    INVALID_SERVER_PUB_KEY = 301,
    INVALID_SERVER_PUB_KEY_SIZE = 302,

    INVALID_CHALLENGE_JSON_SECOND_TIER = 401,
    MISSING_FIELDS_THIRD_TIER = 402,
    MISTYPED_FIELDS_THIRD_TIER = 403,
    FORGED_SIGNATURE_THIRD_TIER = 404,

    INVALID_FOURTH_TIER_JSON = 501,
    MISSING_FIELDS_FOURTH_TIER = 502,
    MISTYPED_FIELDS_FOURTH_TIER = 503,
    REFERRALS_DONT_MATCH = 504,
    CHALLENGE_SIGNATURE_FORGED = 505,

    INVALID_FIFTH_TIER_JSON = 601,
    MISSING_FIELDS_FIFTH_TIER = 602,
    MISTYPED_FIELDS_FIFTH_TIER = 603,
    HASH_TOO_WEAK_FIFTH_TIER = 604,
    PUBLIC_KEY_REQUEST_FAIL_FIFTH_TIER = 605,
    BAD_SCRYPT_VERSION_FIFTH_TIER = 606,
    FORGED_USER_REQUEST_SIGNATURE_FIFTH_TIER = 607
};

enum TimespanErrors {
    INVALID_JSON_FIRST_TIER = 101,
    TE_MISSING_FIELDS_FIRST_TIER = 102,
    TE_MISTYPED_FIELDS_FIRST_TIER = 103,
    TE_INVALID_TIMESPAN = 104,
    TE_LICENSE_EXPIRED = 105,

    TE_INVALID_JSON_SECOND_TIER = 201,
    TE_MISSING_FIELDS_SECOND_TIER = 202,
    TE_MISTYPED_FIELDS_SECOND_TIER = 203,
    TE_FORGED_SIGNATURE_SECOND_TIER = 204,

    TE_INVALID_JSON_THIRD_TIER = 301,
    TE_MISSING_FIELDS_THIRD_TIER = 302,
    TE_MISTYPED_FIELDS_THIRD_TIER = 303,
    TE_MISMATCHED_USERS_THIRD_TIER = 304,
    TE_MISMATCHING_TIMESPANS_THIRD_TIER = 305
};

bool verifySignature(
    const std::string& signature,
    const std::string& pubKeyBase64,
    const std::string& message)
{
    const int CHUNK_SIZE = 4096;
    templatious::StaticBuffer<unsigned char,3*CHUNK_SIZE> uCharBuf;

    auto bufA = uCharBuf.getStaticVectorPre(CHUNK_SIZE);
    auto bufB = uCharBuf.getStaticVector(CHUNK_SIZE);
    auto bufC = uCharBuf.getStaticVectorPre(CHUNK_SIZE);

    TEMPLATIOUS_0_TO_N(i,signature.size()) {
        SA::add(bufB,signature[i]);
    }

    size_t outSize = SA::size(bufA);

    int decodeRes = ::base64decode(
        reinterpret_cast<char*>(bufB.rawBegin()),
        SA::size(bufB),
        bufA.rawBegin(),&outSize);

    if (0 != decodeRes) {
        return false;
    }

    if (crypto_sign_BYTES != outSize) {
        return false;
    }

    int msgLen = message.length();
    if (msgLen + crypto_sign_BYTES > CHUNK_SIZE) {
        return false;
    }

    SA::clear(bufB);

    TEMPLATIOUS_0_TO_N(i,pubKeyBase64.size()) {
        SA::add(bufB,pubKeyBase64[i]);
    }

    outSize = SA::size(bufC);
    decodeRes = ::base64decode(
        reinterpret_cast<char*>(bufB.rawBegin()),
        SA::size(bufB),
        bufC.rawBegin(),&outSize);

    if (0 != decodeRes) {
        return false;
    }

    if (crypto_sign_PUBLICKEYBYTES != outSize) {
        return false;
    }

    SA::clear(bufB);

    TEMPLATIOUS_0_TO_N(i,crypto_sign_BYTES) {
        SA::add(bufB,bufA[i]);
    }

    TEMPLATIOUS_0_TO_N(i,msgLen) {
        SA::add(bufB,message[i]);
    }

    // A might go overflow without
    // this when written by sodium...
    // but why should anyone care?
    while (!bufA.isFull()) {
        bufA.push('7');
    }

    unsigned long long cryptoOut = SA::size(bufA);
    int signRes = ::crypto_sign_open(
        bufA.rawBegin(),&cryptoOut,
        bufB.rawBegin(),SA::size(bufB),
        bufC.rawBegin()
    );

    return signRes == 0;
}

bool hexToBin(const char* str,std::vector< unsigned char >& out) {
    unsigned char outRes[32];
    size_t binLen = sizeof(outRes);
    int res = ::sodium_hex2bin(
        outRes,sizeof(outRes),str,strlen(str),nullptr,&binLen,nullptr);
    if (res != 0) {
        return false;
    }

    SA::clear(out);
    TEMPLATIOUS_0_TO_N(i,binLen) {
        SA::add(out,outRes[i]);
    }
    return true;
}

bool getScryptArgsForVersion(
    int version,
    unsigned char*& salt,
    int& saltLength,
    int& N,
    int& r,
    int& p,
    int& dkLen)
{
    if (version == 1) {
        const char* SCRYPT_SALT =
            "f1705f7a9f458f3af425d0e9349a63c0"
            "83a427fe4a8187d5c5edf526ef7f1359";
        static std::vector< unsigned char > scryptSaltV1Vec;
        static bool hex2BinV1Success = hexToBin(SCRYPT_SALT,scryptSaltV1Vec);
        assert( hex2BinV1Success == true );

        salt = scryptSaltV1Vec.data();
        saltLength = SA::size(scryptSaltV1Vec);
        // should match server values
        N = 1024;
        r = 8;
        p = 8;
        dkLen = 32;
        return true;
    }

    return false;
}

bool scrypt(const std::string& text,int version,std::string& out) {
    unsigned char* salt = nullptr;
    int saltLength,N,r,p,dkLen;
    saltLength = N = r = p = dkLen = 0;
    bool scryptRes = getScryptArgsForVersion(version,salt,saltLength,N,r,p,dkLen);
    if (!scryptRes) {
        return false;
    }
    std::vector< unsigned char > outRes(dkLen);
    ::libscrypt_scrypt(
        reinterpret_cast<unsigned const char*>(text.c_str()),text.size(),
        salt,saltLength,
        N,r,p,
        outRes.data(),SA::size(outRes));
    std::vector< char > hexOutArr(dkLen * 2 + 1);
    auto hexRes =
        ::sodium_bin2hex(
            hexOutArr.data(),
            SA::size(hexOutArr),
            outRes.data(),
            SA::size(outRes));
    assert( hexRes == hexOutArr.data() );
    out.clear();
    TEMPLATIOUS_FOREACH(auto& i,hexOutArr) {
        out += i;
    }
    return true;
}

std::string sha256(const std::string& text) {
    unsigned char out[crypto_hash_sha256_BYTES];
    ::crypto_hash_sha256(out,
        reinterpret_cast<const unsigned char*>(text.c_str()),text.size());
    char outStr[sizeof(out)*2 + 1];
    char* hex = ::sodium_bin2hex(outStr,sizeof(outStr),out,sizeof(out));
    assert( outStr == hex );
    return hex;
}

int byteStrength(unsigned char num) {
    if (num >= 128) {
        return 0;
    } else if (num >= 64) {
        return 1;
    } else if (num >= 32) {
        return 2;
    } else if (num >= 16) {
        return 3;
    } else if (num >= 8) {
        return 4;
    } else if (num >= 4) {
        return 5;
    } else if (num >= 2) {
        return 6;
    } else if (num >= 1) {
        return 7;
    } else {
        return 8;
    }
}

int hashStrength(const std::string& hash) {
    int strength = 0;
    unsigned char outRes[crypto_hash_sha256_BYTES];
    size_t binLen = 0;

    int res = ::sodium_hex2bin(
        outRes,sizeof(outRes),
        hash.c_str(),hash.size(),
        nullptr,&binLen,nullptr);

    assert( 0 == res );
    assert( binLen == crypto_hash_sha256_BYTES );
    TEMPLATIOUS_FOREACH(auto& i,outRes) {
        int thisAdd = byteStrength(i);
        strength += thisAdd;
        if (thisAdd < 8) {
            break;
        }
    }

    return strength;
}

int fifthTierJsonVerification(
    const std::string& publicKey,
    const std::string& challengeAnswer)
{
    rj::Document doc;
    doc.Parse(challengeAnswer.c_str());
    if (doc.HasParseError()) {
        return VerificationFailures
            ::INVALID_FIFTH_TIER_JSON;
    }

    if (   !doc.HasMember("challengestrength")
        || !doc.HasMember("challengeversion")
        || !doc.HasMember("encodedsalt")
        || !doc.HasMember("publickeyrequest")
        || !doc.HasMember("publickeyrequestsignature")
        || !doc.HasMember("useranswer")
        || !doc.HasMember("validuntil"))
    {
        return VerificationFailures
            ::MISSING_FIELDS_FIFTH_TIER;
    }

    auto& challengeStrength = doc["challengestrength"];
    auto& challengeVersion = doc["challengeversion"];
    auto& encodedSalt = doc["encodedsalt"];
    auto& publicKeyRequest = doc["publickeyrequest"];
    auto& publicKeyRequestSignature = doc["publickeyrequestsignature"];
    auto& userAnswer = doc["useranswer"];
    auto& validUntil = doc["validuntil"];

    if (   !challengeStrength.IsNumber()
        || !challengeVersion.IsNumber()
        || !encodedSalt.IsString()
        || !publicKeyRequest.IsString()
        || !publicKeyRequestSignature.IsString()
        || !userAnswer.IsString()
        || !validUntil.IsNumber())
    {
        return VerificationFailures
            ::MISTYPED_FIELDS_FIFTH_TIER;
    }

    int challengeVersionInt = challengeVersion.GetInt();

    std::string challengeTextHash;
    bool scryptSucc = scrypt(
        challengeAnswer,challengeVersionInt,challengeTextHash);
    if (!scryptSucc) {
        return VerificationFailures
            ::BAD_SCRYPT_VERSION_FIFTH_TIER;
    }

    int hashStrengthRes = hashStrength(challengeTextHash);

    int challengeStrengthNum = challengeStrength.GetInt();
    if (challengeStrengthNum > hashStrengthRes) {
        return VerificationFailures
            ::HASH_TOO_WEAK_FIFTH_TIER;
    }

    std::string expectedRequest = publicKey;
    expectedRequest += " I_WANT_TO_USE_SAFELISTS";

    if (expectedRequest != publicKeyRequest.GetString()) {
        return VerificationFailures
            ::PUBLIC_KEY_REQUEST_FAIL_FIFTH_TIER;
    }

    std::string userPubKeySiganture =
        publicKeyRequestSignature.GetString();


    bool internalSigVerification = verifySignature(
            userPubKeySiganture,publicKey,expectedRequest);
    if (!internalSigVerification) {
        return VerificationFailures
            ::FORGED_USER_REQUEST_SIGNATURE_FIFTH_TIER;
    }

    // BOOM, verified
    return 0;
}

int fourthTierJsonVerification(
    const std::string& publicKey,
    const std::string& referral,
    const std::string& theBlob)
{
    rj::Document doc;
    doc.Parse(theBlob.c_str());
    if (doc.HasParseError()) {
        return VerificationFailures
            ::INVALID_FOURTH_TIER_JSON;
    }

    if (   !doc.HasMember("challengetext")
        || !doc.HasMember("challengesignature")
        || !doc.HasMember("referralname"))
    {
        return VerificationFailures
            ::MISSING_FIELDS_FOURTH_TIER;
    }

    auto& challengeText = doc["challengetext"];
    auto& challengeSignature = doc["challengesignature"];
    auto& referralName = doc["referralname"];

    if (   !challengeText.IsString()
        || !challengeSignature.IsString()
        || !referralName.IsString())
    {
        return VerificationFailures
            ::MISTYPED_FIELDS_FOURTH_TIER;
    }

    if (referral != referralName.GetString()) {
        return VerificationFailures
            ::REFERRALS_DONT_MATCH;
    }

    std::string challengeTextSolved = challengeText.GetString();
    std::string challengeSignatureStr = challengeSignature.GetString();
    std::string serverPublicKey = getServerSignKey();

    std::regex replaceAnswer("\"useranswer\":\"[a-zA-Z0-9]+\"");
    std::string challengeTextNoSolution =
        std::regex_replace(
            challengeTextSolved,
            replaceAnswer,
            "\"useranswer\":\"\""
        );

    bool validSignature = verifySignature(
        challengeSignatureStr,serverPublicKey,challengeTextNoSolution);
    if (!validSignature) {
        return VerificationFailures
            ::CHALLENGE_SIGNATURE_FORGED;
    }

    return fifthTierJsonVerification(
        publicKey,challengeTextSolved);
}

int thirdTierChallengeVerification(
    const std::string& publicKey,
    const std::string& referral,
    const std::string& theBlob)
{
    rj::Document doc;
    doc.Parse(theBlob.c_str());

    if (doc.HasParseError()) {
        return VerificationFailures
            ::INVALID_CHALLENGE_JSON_SECOND_TIER;
    }

    if (   !doc.HasMember("challengeanswer")
        || !doc.HasMember("answersignature"))
    {
        return VerificationFailures
            ::MISSING_FIELDS_THIRD_TIER;
    }

    auto& challengeAnswer = doc["challengeanswer"];
    auto& answerSignature = doc["answersignature"];

    if (   !challengeAnswer.IsString()
        || !answerSignature.IsString())
    {
        return VerificationFailures
            ::MISTYPED_FIELDS_THIRD_TIER;
    }

    std::string answer = challengeAnswer.GetString();
    std::string signature = answerSignature.GetString();

    bool isGood = verifySignature(signature,publicKey,answer);
    if (!isGood) {
        return VerificationFailures
            ::FORGED_SIGNATURE_THIRD_TIER;
    }

    return fourthTierJsonVerification(
        publicKey,referral,answer);
}

int secondTierJsonValidation(
    const std::string& publicKey,
    const std::string& referral,
    const std::string& theBlob)
{
    rj::Document doc;
    doc.Parse(theBlob.c_str());

    if (doc.HasParseError()) {
        return VerificationFailures::INVALID_BLOB_JSON;
    }

    if (   !doc.HasMember("signedblob")
        || !doc.HasMember("signature"))
    {
        return VerificationFailures::MISSING_FIELDS_SECOND_TIER;
    }

    auto& challengeAnswer = doc["signedblob"];
    auto& answerSignature = doc["signature"];

    if (   !challengeAnswer.IsString()
        || !answerSignature.IsString())
    {
        return VerificationFailures::MISTYPED_FIELDS_SECOND_TIER;
    }

    std::string challengeAnswerStr = challengeAnswer.GetString();
    std::string answerSignatureStr = answerSignature.GetString();
    std::string serverPublicKey = getServerSignKey();

    bool isValidSignature = verifySignature(
        answerSignatureStr,serverPublicKey,challengeAnswerStr);

    if (!isValidSignature) {
        return VerificationFailures::FORGED_SIGNATURE_SECOND_TIER;
    }

    return thirdTierChallengeVerification(
        publicKey,referral,challengeAnswerStr);
}

int firstTierSignatureVerification(const std::string& theJson) {
    rj::Document doc;
    doc.Parse(theJson.c_str());

    if (doc.HasParseError()) {
        return VerificationFailures(FORGED_FIRST_TIER);
    }

    if (   !doc.HasMember("userkey")
        || !doc.HasMember("referral")
        || !doc.HasMember("challengeblob"))
    {
        return VerificationFailures::MISSING_FIELDS_FIRST_TIER;
    }

    auto& userkey = doc["userkey"];
    auto& referral = doc["referral"];
    auto& blob = doc["challengeblob"];

    if (   !userkey.IsString()
        || !referral.IsString()
        || !blob.IsString())
    {
        return VerificationFailures::MISTYPED_FIELDS_FIRST_TIER;
    }

    std::string userKeyStr(userkey.GetString());
    std::string referralStr(referral.GetString());
    std::string blobStr(blob.GetString());

    return secondTierJsonValidation(userKeyStr,referralStr,blobStr);
}

int checkUserTimespanValidityThirdTier(
    int64_t from,int64_t to,
    const std::string& theJson,const std::string& userkey)
{
    rj::Document doc;
    doc.Parse(theJson.c_str());

    if (doc.HasParseError()) {
        return TimespanErrors::TE_INVALID_JSON_THIRD_TIER;
    }

    if (   !doc.HasMember("user")
        || !doc.HasMember("from")
        || !doc.HasMember("to"))
    {
        return TimespanErrors::TE_MISSING_FIELDS_THIRD_TIER;
    }

    auto& user = doc["user"];
    auto& fromJson = doc["from"];
    auto& toJson = doc["to"];

    if (   !user.IsString()
        || !fromJson.IsNumber()
        || !toJson.IsNumber())
    {
        return TimespanErrors::TE_MISTYPED_FIELDS_THIRD_TIER;
    }

    int64_t fromJsNum = fromJson.GetInt64();
    int64_t toJsNum = toJson.GetInt64();
    std::string userStr = user.GetString();

    if (userkey != userStr) {
        return TimespanErrors::TE_MISMATCHED_USERS_THIRD_TIER;
    }

    if (from != fromJsNum || to != toJsNum) {
        return TimespanErrors::TE_MISMATCHING_TIMESPANS_THIRD_TIER;
    }

    return 0;
}

int checkUserTimespanValiditySecondTier(
    int64_t from,int64_t to,
    const std::string& signature,
    const std::string& userkey)
{
    rj::Document doc;
    doc.Parse(signature.c_str());

    if (doc.HasParseError()) {
        return TimespanErrors::TE_INVALID_JSON_SECOND_TIER;
    }

    if (   !doc.HasMember("signedspan")
        || !doc.HasMember("signature"))
    {
        return TimespanErrors::TE_MISSING_FIELDS_SECOND_TIER;
    }

    auto& signedSpan = doc["signedspan"];
    auto& signatureInner = doc["signature"];

    if (   !signedSpan.IsString()
        || !signatureInner.IsString())
    {
        return TimespanErrors::TE_MISTYPED_FIELDS_SECOND_TIER;
    }

    std::string sSpan = signedSpan.GetString();
    std::string sSig = signatureInner.GetString();
    std::string serverPublicKey = getServerSignKey();

    bool isGood = SafeLists::verifySignature(
        sSig,serverPublicKey,sSpan);

    if (!isGood) {
        return TimespanErrors::TE_FORGED_SIGNATURE_SECOND_TIER;
    }

    return checkUserTimespanValidityThirdTier(from,to,sSpan,userkey);
}

int checkUserTimespanValidityFirstTier(
    const std::string& json,
    const std::string& userid)
{
    rj::Document doc;
    doc.Parse(json.c_str());

    if (doc.HasParseError()) {
        return TimespanErrors::INVALID_JSON_FIRST_TIER;
    }

    if (   !doc.HasMember("from")
        || !doc.HasMember("to")
        || !doc.HasMember("signature"))
    {
        return TimespanErrors::TE_MISSING_FIELDS_FIRST_TIER;
    }

    auto& from = doc["from"];
    auto& to = doc["to"];
    auto& signature = doc["signature"];

    if (   !from.IsNumber()
        || !to.IsNumber())
    {
        return TimespanErrors::TE_MISTYPED_FIELDS_FIRST_TIER;
    }

    int64_t fromNum = from.GetInt64();
    int64_t toNum = to.GetInt64();
    if (   (fromNum == toNum && fromNum != -1)
        || fromNum > toNum)
    {
        return TimespanErrors::TE_INVALID_TIMESPAN;
    }

    auto currtime = std::time(nullptr);
    // license expiry date
    if (!(fromNum <= currtime && toNum >= currtime)) {
        return TimespanErrors::TE_LICENSE_EXPIRED;
    }

    std::string signatureStr = signature.GetString();

    return checkUserTimespanValiditySecondTier(
        fromNum,toNum,signatureStr,userid);
}

std::string executablePath() {
    auto programArgs = SafeLists::getGlobalProgramArgs();
    fs::path selfpath = programArgs[0];
    selfpath = fs::absolute(selfpath.remove_filename());
    return selfpath.string();
}

std::string localLicensePath(const std::string& pubKey) {
    // since key is base64 and
    // can contain slashes just get sha256...
    std::string pubKeyHash = "license-" + sha256(pubKey);
    pubKeyHash += ".json";
    std::string absFilePath = executablePath();
    absFilePath += "/userdata/licensecache/";
    absFilePath += pubKeyHash;
    return absFilePath;
}

std::string localTimespanPath(const std::string& pubKey) {
    // since key is base64 and
    // can contain slashes just get sha256...
    std::string pubKeyHash = "subscription-" + sha256(pubKey);
    pubKeyHash += ".json";
    std::string absFilePath = executablePath();
    absFilePath += "/userdata/licensecache/";
    absFilePath += pubKeyHash;
    return absFilePath;
}

int localGetLicense(const std::string& pubKey,std::string& out) {
    std::string absFilePath = localLicensePath(pubKey);
    std::ifstream file(absFilePath.c_str());
    if (!file.is_open()) {
        return 1; // 1 - could not open file
    }

    std::string license;
    char single;
    while (file.get(single)) {
        license += single;
    }

    out = license;
    return 0;
}

int serverGetLicense(const std::string& pubKey,std::string& out) {
    // should fit
    std::vector<char> buf(1024 * 16);

    int bufLen = SA::size(buf);
    int res = querySignature(
        pubKey,"/getuser/",buf.data(),bufLen);

    if (res != 0) {
        return 1;
    }

    assert( bufLen > 0 && bufLen < SA::size(buf) );

    out.assign(buf.data(),buf.data() + bufLen);
    return 0;
}

int localGetTimespan(const std::string& pubKey,std::string& out) {
    std::string absFilePath = localTimespanPath(pubKey);
    std::ifstream file(absFilePath.c_str());
    if (!file.is_open()) {
        return 1; // 1 - could not open file
    }

    std::string license;
    char single;
    while (file.get(single)) {
        license += single;
    }

    out = license;
    return 0;
}

int serverGetTimespan(const std::string& pubKey,std::string& out) {
    // should fit
    std::vector<char> buf(1024 * 16);

    int bufLen = SA::size(buf);
    int res = querySignature(
        pubKey,"/getrelevantsubscription/",buf.data(),bufLen);

    if (res != 0) {
        return 1;
    }

    assert( bufLen > 0 && bufLen < SA::size(buf) );

    out.assign(buf.data(),buf.data() + bufLen);
    return 0;
}

int localStoreLicense(
    const std::string& pubKey,
    const std::string& value)
{
    std::string path = localLicensePath(pubKey);

    fs::path boostPath(path.c_str());
    auto parent = boostPath.parent_path();
    fs::create_directories(parent);

    auto file = ::fopen(path.c_str(),"w");
    if (nullptr == file) {
        // could not open file
        return 1;
    }

    auto closeGuard = SCOPE_GUARD_LC(
        ::fclose(file);
    );

    ::fwrite(value.c_str(),1,value.size(),file);

    return 0;
}

int localDeleteLicense(const std::string& pubKey) {
    std::string path = localLicensePath(pubKey);
    bool succeeded = fs::remove(path.c_str());
    return succeeded ? 0 : 1;
}

int localDeleteSpan(const std::string& pubKey) {
    std::string path = localTimespanPath(pubKey);
    bool succeeded = fs::remove(path.c_str());
    return succeeded ? 0 : 1;
}

void regUserRoutine(const std::string& name,const StrongMsgPtr& toNotify) {
}

struct LicenseDaemonDummyImpl : public Messageable {
    LicenseDaemonDummyImpl(const LicenseDaemonDummyImpl&) = delete;
    LicenseDaemonDummyImpl(LicenseDaemonDummyImpl&&) = delete;

    // this is for sending message across threads
    void message(const std::shared_ptr< templatious::VirtualPack >& msg) override {
        _cache.enqueue(msg);
        _sem.notify();
    }

    // this is for sending stack allocated (faster)
    // if we know we're on the same thread as GUI
    void message(templatious::VirtualPack& msg) override {
        assert( false && "Only async messages in license daemon." );
    }

    static std::shared_ptr< LicenseDaemonDummyImpl > makeNew() {
        // perfect spot
        ::sodium_init();

        std::shared_ptr< LicenseDaemonDummyImpl > result(
            new LicenseDaemonDummyImpl()
        );
        result->_myself = result;
        std::thread(
            [=]() {
                result->mainLoop();
            }
        ).detach();
        return result;
    }
private:
    void mainLoop() {
        auto sendMsg = SCOPE_GUARD_LC(
            auto locked = _notifyExit.lock();
            if (nullptr != locked) {
                auto msg = SF::vpack<
                    GenericShutdownGuard::SetFuture >(nullptr);
                locked->message(msg);
            }
        );

        while (_keepGoing) {
            _sem.wait();
            _cache.process(
                [=](templatious::VirtualPack& pack) {
                    _handler->tryMatch(pack);
                }
            );
        }
    }

    LicenseDaemonDummyImpl() :
        _handler(genHandler()), _keepGoing(true) {}

    VmfPtr genHandler() {
        typedef LicenseDaemon LD;
        return SF::virtualMatchFunctorPtr(
            SF::virtualMatch< LD::GetCurrentUserId, std::string >(
                [=](ANY_CONV,std::string& pubKey) {
                    // TODO: query from safe app launcher.
                    pubKey = getCurrentUserIdBase64();
                }
            ),
            SF::virtualMatch< LD::GetLocalRecord, const std::string, std::string, int >(
                [=](ANY_CONV,const std::string& pubKey,std::string& outlicense,int& outErrCode) {
                    outErrCode = localGetLicense(pubKey,outlicense);
                }
            ),
            SF::virtualMatch< LD::GetServerRecord, const std::string, std::string, int >(
                [=](ANY_CONV,const std::string& pubKey,std::string& outlicense,int& outErrCode) {
                    outErrCode = serverGetLicense(pubKey,outlicense);
                }
            ),
            SF::virtualMatch< LD::GetLocalTimespan, const std::string, std::string, int >(
                [=](ANY_CONV,const std::string& pubKey,std::string& outlicense,int& outErrCode) {
                    outErrCode = localGetTimespan(pubKey,outlicense);
                }
            ),
            SF::virtualMatch< LD::GetServerTimespan, const std::string, std::string, int >(
                [=](ANY_CONV,const std::string& pubKey,std::string& outlicense,int& outErrCode) {
                    outErrCode = serverGetTimespan(pubKey,outlicense);
                }
            ),
            SF::virtualMatch< LD::StoreLocalLicense, const std::string, const std::string, int >(
                [=](ANY_CONV,const std::string& pubKey,const std::string& contents,int& outErrCode) {
                    outErrCode = localStoreLicense(pubKey,contents);
                }
            ),
            SF::virtualMatch< LD::DeleteLocalLicense, const std::string, int >(
                [=](ANY_CONV,const std::string& pubKey,int& outErrCode) {
                    outErrCode = localDeleteLicense(pubKey);
                }
            ),
            SF::virtualMatch< LD::DeleteLocalSpan, const std::string, int >(
                [=](ANY_CONV,const std::string& pubKey,int& outErrCode) {
                    outErrCode = localDeleteSpan(pubKey);
                }
            ),
            SF::virtualMatch< LD::UserRecordValidity, const std::string, int >(
                [=](ANY_CONV,const std::string& json,int& outRes) {
                    outRes = firstTierSignatureVerification(json);
                }
            ),
            SF::virtualMatch< LD::TimeSpanValidity, const std::string, const std::string, int >(
                [=](ANY_CONV,const std::string& json,const std::string& user,int& outRes) {
                    outRes = checkUserTimespanValidityFirstTier(json,user);
                }
            ),
            SF::virtualMatch< LD::RegisterUser, const std::string, const StrongMsgPtr >(
                [=](ANY_CONV,const std::string& name,const StrongMsgPtr& toNotify) {
                    regUserRoutine(name,toNotify);
                }
            ),
            SF::virtualMatch< GSI::InRegisterItself, StrongMsgPtr >(
                [=](ANY_CONV,StrongMsgPtr& ptr) {
                    auto handler = std::make_shared< GenericShutdownGuard >();
                    handler->setMaster(_myself);
                    auto msg = SF::vpackPtr< GSI::OutRegisterItself, StrongMsgPtr >(
                        nullptr, handler
                    );
                    assert( nullptr == _notifyExit.lock() );
                    _notifyExit = handler;
                    ptr->message(msg);
                }
            ),
            SF::virtualMatch< GenericShutdownGuard::ShutdownTarget >(
                [=](GenericShutdownGuard::ShutdownTarget) {
                    this->_keepGoing = false;
                }
            )
        );
    }

    MessageCache _cache;
    StackOverflow::Semaphore _sem;
    VmfPtr _handler;
    bool _keepGoing;
    WeakMsgPtr _myself;
    WeakMsgPtr _notifyExit;
};

StrongMsgPtr LicenseDaemon::getDaemon() {
    static StrongMsgPtr theObject =
        LicenseDaemonDummyImpl::makeNew();
    return theObject;
}

}

