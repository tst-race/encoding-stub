#include "PluginStubEncoding.h"

#include <MockEncodingSdk.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SaveArg;

namespace {
ChannelResponse sdkResponse() { return {CM_OK, NULL_RACE_HANDLE}; }

class PluginStubEncodingTest : public ::testing::Test {
protected:
    void SetUp() override {
        ON_CALL(sdk, updateState(_)).WillByDefault(Return(sdkResponse()));
        ON_CALL(sdk, onBytesEncoded(_, _, _)).WillByDefault(Return(sdkResponse()));
        ON_CALL(sdk, onBytesDecoded(_, _, _)).WillByDefault(Return(sdkResponse()));
    }

    NiceMock<MockEncodingSdk> sdk;
    const EncodingParameters params{"test-link", "text/plain", true, "{}"};
};
}

TEST_F(PluginStubEncodingTest, startsAndReportsConfiguredProperties) {
    EXPECT_CALL(sdk, updateState(COMPONENT_STATE_STARTED)).Times(1);
    PluginStubEncoding encoding(&sdk);

    const auto properties = encoding.getEncodingProperties();
    EXPECT_EQ("text/plain", properties.type);
    EXPECT_GT(properties.encodingTime, 0.0);
    EXPECT_EQ(1'000'000, encoding.getEncodingPropertiesForParameters(params).maxBytes);
}

TEST_F(PluginStubEncodingTest, roundTripsEmptyAndAllByteValues) {
    PluginStubEncoding encoding(&sdk);
    const std::vector<uint8_t> empty;
    std::vector<uint8_t> emptyEncoded{1};
    EXPECT_CALL(sdk, onBytesEncoded(11, _, ENCODE_OK))
        .WillOnce(DoAll(SaveArg<1>(&emptyEncoded), Return(sdkResponse())));
    EXPECT_EQ(COMPONENT_OK, encoding.encodeBytes(11, params, empty));
    EXPECT_TRUE(emptyEncoded.empty());

    std::vector<uint8_t> emptyDecoded{1};
    EXPECT_CALL(sdk, onBytesDecoded(12, _, ENCODE_OK))
        .WillOnce(DoAll(SaveArg<1>(&emptyDecoded), Return(sdkResponse())));
    EXPECT_EQ(COMPONENT_OK, encoding.decodeBytes(12, params, emptyEncoded));
    EXPECT_TRUE(emptyDecoded.empty());

    std::vector<uint8_t> input;
    for (unsigned int value = 0; value < 256; ++value) {
        input.push_back(static_cast<uint8_t>(value));
    }
    std::vector<uint8_t> encoded;
    EXPECT_CALL(sdk, onBytesEncoded(13, _, ENCODE_OK))
        .WillOnce(DoAll(SaveArg<1>(&encoded), Return(sdkResponse())));
    EXPECT_EQ(COMPONENT_OK, encoding.encodeBytes(13, params, input));

    std::vector<uint8_t> decoded;
    EXPECT_CALL(sdk, onBytesDecoded(14, _, ENCODE_OK))
        .WillOnce(DoAll(SaveArg<1>(&decoded), Return(sdkResponse())));
    EXPECT_EQ(COMPONENT_OK, encoding.decodeBytes(14, params, encoded));
    EXPECT_EQ(input, decoded);
}

TEST_F(PluginStubEncodingTest, acceptsDeclaredMaximumAndRejectsLargerInput) {
    PluginStubEncoding encoding(&sdk);
    const std::vector<uint8_t> maximum(1'000'000, 0xA5);
    std::vector<uint8_t> encoded;
    EXPECT_CALL(sdk, onBytesEncoded(21, _, ENCODE_OK))
        .WillOnce(DoAll(SaveArg<1>(&encoded), Return(sdkResponse())));
    EXPECT_EQ(COMPONENT_OK, encoding.encodeBytes(21, params, maximum));
    EXPECT_EQ(4 * ((maximum.size() + 2) / 3), encoded.size());

    const std::vector<uint8_t> tooLarge(1'000'001, 0);
    EXPECT_CALL(sdk, onBytesEncoded(22, _, ENCODE_FAILED)).Times(1);
    EXPECT_EQ(COMPONENT_ERROR, encoding.encodeBytes(22, params, tooLarge));
}

TEST_F(PluginStubEncodingTest, malformedInputCallsFailureCallback) {
    PluginStubEncoding encoding(&sdk);
    const std::vector<uint8_t> malformed{'!', '!', '!', '!'};
    EXPECT_CALL(sdk, onBytesDecoded(31, _, ENCODE_FAILED)).Times(1);
    EXPECT_EQ(COMPONENT_ERROR, encoding.decodeBytes(31, params, malformed));
}