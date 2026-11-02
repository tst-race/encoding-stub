#include "PluginStubEncoding.h"

#include "base64.h"

#include <stdexcept>

const char *PluginStubEncoding::name = "stub";

namespace {
constexpr int32_t kMaxBytes = 1'000'000;
constexpr double kEncodingTimeSeconds = 0.25;
constexpr char kMimeType[] = "text/plain";
}

PluginStubEncoding::PluginStubEncoding(IEncodingSdk *sdk) : sdk(sdk) {
    if (sdk == nullptr) {
        throw std::invalid_argument("PluginStubEncoding: sdk parameter is null");
    }
    sdk->updateState(COMPONENT_STATE_STARTED);
}

ComponentStatus PluginStubEncoding::onUserInputReceived(RaceHandle, bool,
                                                        const std::string &) {
    return COMPONENT_OK;
}

EncodingProperties PluginStubEncoding::getEncodingProperties() {
    return {kEncodingTimeSeconds, kMimeType};
}

SpecificEncodingProperties PluginStubEncoding::getEncodingPropertiesForParameters(
    const EncodingParameters &) {
    return {kMaxBytes};
}

ComponentStatus PluginStubEncoding::encodeBytes(RaceHandle handle,
                                                const EncodingParameters &,
                                                const std::vector<uint8_t> &bytes) {
    if (bytes.size() > static_cast<size_t>(kMaxBytes)) {
        sdk->onBytesEncoded(handle, {}, ENCODE_FAILED);
        return COMPONENT_ERROR;
    }
    const std::string encoded = base64::encode(bytes);
    sdk->onBytesEncoded(handle, {encoded.begin(), encoded.end()}, ENCODE_OK);
    return COMPONENT_OK;
}

ComponentStatus PluginStubEncoding::decodeBytes(RaceHandle handle,
                                                const EncodingParameters &,
                                                const std::vector<uint8_t> &bytes) {
    try {
        const std::string encoded(bytes.begin(), bytes.end());
        std::vector<uint8_t> decoded = base64::decode(encoded);
        if (decoded.size() > static_cast<size_t>(kMaxBytes)) {
            throw std::invalid_argument("decoded payload exceeds maxBytes");
        }
        sdk->onBytesDecoded(handle, decoded, ENCODE_OK);
        return COMPONENT_OK;
    } catch (const std::invalid_argument &) {
        sdk->onBytesDecoded(handle, {}, ENCODE_FAILED);
        return COMPONENT_ERROR;
    }
}

IEncodingComponent *createEncoding(const std::string &encoding, IEncodingSdk *sdk,
                                   const std::string &, const PluginConfig &) {
    if (encoding != PluginStubEncoding::name || sdk == nullptr) {
        return nullptr;
    }
    return new PluginStubEncoding(sdk);
}

void destroyEncoding(IEncodingComponent *component) {
    delete component;
}

const RaceVersionInfo raceVersion = RACE_VERSION;