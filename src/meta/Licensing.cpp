
#include <cassert>
#include <iostream>
#include <sodium.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <rapidjson/document.h>

#include <util/Semaphore.hpp>
#include <util/GenericShutdownGuard.hpp>
#include <util/ScopeGuard.hpp>
#include <util/Base64.hpp>

#include "Licensing.hpp"

TEMPLATIOUS_TRIPLET_STD;

typedef SafeLists::GracefulShutdownInterface GSI;
namespace rj = rapidjson;

namespace SafeLists {

std::string getCurrentUserIdBase64() {
    // dummy implementation, matches play backend test
    return "4j5iMF/54gJW/c4bPAdN28+4YcIeSQj3QGvOv2xIZjU=";
}

std::string getServerSignKey() {
    // debug mode, different for production
    return "liz538kE+U7V6dzigrw00JN2H0nCR9strNyxoj9cpn8=";
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

int querySignature(const std::string& user,char* out,int& buflen) {
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
    url += "/getuser/";
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

    INVALID_SERVER_PUB_KEY = 301,
    INVALID_SERVER_PUB_KEY_SIZE = 302
};

bool verifySignature(
    const std::string& signature,
    const std::string& pubKeyBase64,
    const std::string& message)
{
    templatious::StaticBuffer<unsigned char,2*4096> uCharBuf;

    auto bufA = uCharBuf.getStaticVectorPre(4096);
    auto bufB = uCharBuf.getStaticVector();

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

    SA::clear(bufB);

    return true;
}

int challengeBlobVerification(
    const std::string& publicKey,
    const std::string& referral,
    const std::string& theBlob)
{

    templatious::StaticBuffer<unsigned char,4096> uCharBuf;
    templatious::StaticBuffer<char,4096> charBuf;

    auto ubuf = uCharBuf.getStaticVectorPre(uCharBuf.total_size);
    auto charVec = charBuf.getStaticVector();

    size_t outBufSize = SA::size(ubuf);

    auto srvKey = getServerSignKey();
    TEMPLATIOUS_0_TO_N(i,srvKey.size()) {
        SA::add(charVec,srvKey[i]);
    }

    int res =
        ::base64decode(
            charVec.rawBegin(),SA::size(charVec),
            ubuf.rawBegin(),&outBufSize);

    if (0 != res) {
        return VerificationFailures::INVALID_SERVER_PUB_KEY;
    }

    if (outBufSize != crypto_sign_PUBLICKEYBYTES) {
        return VerificationFailures::INVALID_SERVER_PUB_KEY_SIZE;
    }

    char pkBytes[crypto_sign_PUBLICKEYBYTES];
    ::memcpy(pkBytes,ubuf.rawBegin(),sizeof(pkBytes));

    SA::clear(ubuf);

    return 0;
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

    if (   !doc.HasMember("challengeanswer")
        || !doc.HasMember("answersignature"))
    {
        return VerificationFailures::MISSING_FIELDS_SECOND_TIER;
    }

    auto& challengeAnswer = doc["challengeanswer"];
    auto& answerSignature = doc["answersignature"];

    if (   !challengeAnswer.IsString()
        || answerSignature.IsString())
    {
        return VerificationFailures::MISTYPED_FIELDS_SECOND_TIER;
    }

    return 0;
}

int firstTierSignatureVerification(const std::string& theJson) {
    rj::Document doc;
    std::cout << "|" << theJson << "|" << std::endl;
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

int licenseReadOrRegisterRoutine() {
    char outAnswer[4096];
    int outLen = sizeof(outAnswer);
    int queryRes = querySignature(
        getCurrentUserIdBase64(),outAnswer,outLen);
    std::string theJson;
    if (0 == queryRes) {
        theJson.assign( outAnswer, outAnswer + outLen );
    } else {
        std::cout << "Could not query license info..." << std::endl;
        return 1;
    }
    return firstTierSignatureVerification(theJson);
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
            SF::virtualMatch< LD::IsExpired, bool >(
                [=](ANY_CONV,bool& res) {
                    // do something, read files or whatever
                    int result = licenseReadOrRegisterRoutine();
                    return res = result == 0;
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

