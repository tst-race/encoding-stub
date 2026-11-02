#pragma once

#include <IEncodingComponent.h>

class PluginStubEncoding final : public IEncodingComponent {
public:
    explicit PluginStubEncoding(IEncodingSdk *sdk);

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
    IEncodingSdk *sdk;
};