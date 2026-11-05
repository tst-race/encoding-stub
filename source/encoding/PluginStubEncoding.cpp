#include "PluginStubEncoding.h"

#include "base64.h"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <cctype>

const char *PluginStubEncoding::name = "stub";

namespace {
constexpr int32_t kMaxBytes = 1'000'000;
constexpr double kEncodingTimeSeconds = 0.25;
constexpr char kMimeType[] = "text/plain";
std::atomic<uint64_t> outputSequence{0};
}

PluginStubEncoding::PluginStubEncoding(IEncodingSdk *sdk, const PluginConfig &pluginConfig) :
    sdk(sdk),
    loggingDirectory(pluginConfig.loggingDirectory) {
    if (sdk == nullptr) {
        throw std::invalid_argument("PluginStubEncoding: sdk parameter is null");
    }
    const auto response = sdk->requestPluginUserInput(
        "save-encoded-output",
        "Write encoded outputs to the logging directory for offline analysis? (true/false; default false)",
        false);
    if (response.status == CM_OK) {
        saveEncodedOutputHandle = response.handle;
    }
    sdk->updateState(COMPONENT_STATE_STARTED);
}

ComponentStatus PluginStubEncoding::onUserInputReceived(RaceHandle handle, bool answered,
                                                        const std::string &response) {
    if (handle == NULL_RACE_HANDLE || handle != saveEncodedOutputHandle) {
        return COMPONENT_ERROR;
    }
    saveEncodedOutputHandle = NULL_RACE_HANDLE;
    if (!answered) {
        saveEncodedOutputs.store(false);
        return COMPONENT_OK;
    }
    std::string normalizedResponse = response;
    std::transform(normalizedResponse.begin(), normalizedResponse.end(),
                   normalizedResponse.begin(), [](unsigned char value) {
                       return static_cast<char>(std::tolower(value));
                   });
    if (normalizedResponse == "true" || normalizedResponse == "1") {
        saveEncodedOutputs.store(true);
        return COMPONENT_OK;
    }
    if (normalizedResponse == "false" || normalizedResponse == "0") {
        saveEncodedOutputs.store(false);
        return COMPONENT_OK;
    }
    saveEncodedOutputs.store(false);
    return COMPONENT_ERROR;
}

bool PluginStubEncoding::persistEncodedOutput(RaceHandle handle,
                                              const std::vector<uint8_t> &bytes) {
    if (!saveEncodedOutputs.load()) {
        return true;
    }
    if (loggingDirectory.empty()) {
        return false;
    }
    try {
        const auto outputDirectory = std::filesystem::path(loggingDirectory) / "encoding-outputs";
        std::filesystem::create_directories(outputDirectory);
        const auto sequence = outputSequence.fetch_add(1);
        const auto filename = "encoded-output-" + std::to_string(handle) + "-" +
                              std::to_string(sequence) + ".bin";
        std::ofstream output(outputDirectory / filename, std::ios::binary | std::ios::trunc);
        if (!output) {
            return false;
        }
        if (!bytes.empty()) {
            output.write(reinterpret_cast<const char *>(bytes.data()),
                         static_cast<std::streamsize>(bytes.size()));
        }
        output.flush();
        return output.good();
    } catch (const std::filesystem::filesystem_error &) {
        return false;
    }
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
    const std::vector<uint8_t> encodedBytes(encoded.begin(), encoded.end());
    if (!persistEncodedOutput(handle, encodedBytes)) {
        sdk->onBytesEncoded(handle, {}, ENCODE_FAILED);
        return COMPONENT_ERROR;
    }
    sdk->onBytesEncoded(handle, encodedBytes, ENCODE_OK);
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
                                   const std::string &, const PluginConfig &pluginConfig) {
    if (encoding != PluginStubEncoding::name || sdk == nullptr) {
        return nullptr;
    }
    return new PluginStubEncoding(sdk, pluginConfig);
}

void destroyEncoding(IEncodingComponent *component) {
    delete component;
}

const RaceVersionInfo raceVersion = RACE_VERSION;