#pragma once

#include <IEncodingComponent.h>

#include <atomic>
#include <string>

class PluginStubEncoding final : public IEncodingComponent {
public:
    PluginStubEncoding(IEncodingSdk *sdk, const PluginConfig &pluginConfig);

    ComponentStatus onUserInputReceived(RaceHandle handle, bool answered,
                                        const std::string &response) override;
    EncodingProperties getEncodingProperties() override;
    SpecificEncodingProperties getEncodingPropertiesForParameters(
        const EncodingParameters &params) override;
    ComponentStatus encodeBytes(RaceHandle handle, const EncodingParameters &params,
                                const std::vector<uint8_t> &bytes) override;
    ComponentStatus decodeBytes(RaceHandle handle, const EncodingParameters &params,
                                const std::vector<uint8_t> &bytes) override;

    static const char *name;

private:
    bool persistEncodedOutput(RaceHandle handle, const std::vector<uint8_t> &bytes);

    IEncodingSdk *sdk;
    std::string loggingDirectory;
    RaceHandle saveEncodedOutputHandle{NULL_RACE_HANDLE};
    std::atomic<bool> saveEncodedOutputs{false};
};