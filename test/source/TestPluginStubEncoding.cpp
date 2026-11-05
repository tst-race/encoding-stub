#include "PluginStubEncoding.h"

#include <MockEncodingSdk.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
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
        static uint64_t testSequence = 0;
        testDirectory = std::filesystem::temp_directory_path() /
                        ("encoding-stub-test-" + std::to_string(testSequence++));
        loggingDirectory = testDirectory / "logs";
        pluginConfig.loggingDirectory = loggingDirectory.string();
        ON_CALL(sdk, updateState(_)).WillByDefault(Return(sdkResponse()));
        ON_CALL(sdk, requestPluginUserInput(_, _, _))
            .WillByDefault(Return(ChannelResponse{CM_OK, 101}));
        ON_CALL(sdk, onBytesEncoded(_, _, _)).WillByDefault(Return(sdkResponse()));
        ON_CALL(sdk, onBytesDecoded(_, _, _)).WillByDefault(Return(sdkResponse()));
    }

    void TearDown() override { std::filesystem::remove_all(testDirectory); }

    PluginStubEncoding makeEncoding() { return PluginStubEncoding(&sdk, pluginConfig); }

    NiceMock<MockEncodingSdk> sdk;
    PluginConfig pluginConfig;
    std::filesystem::path testDirectory;
    std::filesystem::path loggingDirectory;
    const EncodingParameters params{"test-link", "text/plain", true, "{}"};
};
}

TEST_F(PluginStubEncodingTest, startsAndReportsConfiguredProperties) {
    EXPECT_CALL(sdk, requestPluginUserInput(
                         "save-encoded-output", _, false))
        .Times(1);
    EXPECT_CALL(sdk, updateState(COMPONENT_STATE_STARTED)).Times(1);
    auto encoding = makeEncoding();

    const auto properties = encoding.getEncodingProperties();
    EXPECT_EQ("text/plain", properties.type);
    EXPECT_GT(properties.encodingTime, 0.0);
    EXPECT_EQ(1'000'000, encoding.getEncodingPropertiesForParameters(params).maxBytes);
}

TEST_F(PluginStubEncodingTest, roundTripsEmptyAndAllByteValues) {
    auto encoding = makeEncoding();
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
    EXPECT_FALSE(std::filesystem::exists(loggingDirectory / "encoding-outputs"));
}

TEST_F(PluginStubEncodingTest, acceptsDeclaredMaximumAndRejectsLargerInput) {
    auto encoding = makeEncoding();
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
    auto encoding = makeEncoding();
    const std::vector<uint8_t> malformed{'!', '!', '!', '!'};
    EXPECT_CALL(sdk, onBytesDecoded(31, _, ENCODE_FAILED)).Times(1);
    EXPECT_EQ(COMPONENT_ERROR, encoding.decodeBytes(31, params, malformed));
}

TEST_F(PluginStubEncodingTest, persistsEncodedOutputOnlyWhenOptedIn) {
    auto encoding = makeEncoding();
    EXPECT_EQ(COMPONENT_OK, encoding.onUserInputReceived(101, true, "1"));

    const std::vector<uint8_t> input{'h', 'i'};
    std::vector<uint8_t> encoded;
    EXPECT_CALL(sdk, onBytesEncoded(41, _, ENCODE_OK))
        .WillOnce(DoAll(SaveArg<1>(&encoded), Return(sdkResponse())));
    EXPECT_EQ(COMPONENT_OK, encoding.encodeBytes(41, params, input));

    const auto outputDirectory = loggingDirectory / "encoding-outputs";
    ASSERT_TRUE(std::filesystem::is_directory(outputDirectory));
    std::vector<std::filesystem::path> outputs;
    for (const auto &entry : std::filesystem::directory_iterator(outputDirectory)) {
        outputs.push_back(entry.path());
    }
    ASSERT_EQ(1, outputs.size());
    std::ifstream output(outputs.front(), std::ios::binary);
    const std::vector<uint8_t> persisted{std::istreambuf_iterator<char>(output),
                                         std::istreambuf_iterator<char>()};
    EXPECT_EQ(encoded, persisted);
}