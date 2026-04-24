#include <gtest/gtest.h>
#include "conversation.hpp"

using namespace agent;

TEST(ConversationTest, EmptyOnCreation) {
    Conversation conv;
    EXPECT_TRUE(conv.empty());
    EXPECT_EQ(conv.get_messages().size(), 0u);
}

TEST(ConversationTest, AddUserMessage) {
    Conversation conv;
    conv.add_user_msg("Hello");
    EXPECT_EQ(conv.get_messages().size(), 1u);
    EXPECT_EQ(conv.get_messages()[0].role, "user");
    EXPECT_EQ(conv.get_messages()[0].content, "Hello");
}

TEST(ConversationTest, AddAssistantMessage) {
    Conversation conv;
    conv.add_assistant_msg("Hi there");
    EXPECT_EQ(conv.get_messages()[0].role, "assistant");
    EXPECT_EQ(conv.get_messages()[0].content, "Hi there");
}

TEST(ConversationTest, AddToolResult) {
    Conversation conv;
    conv.add_tool_result("tool_123", "result data");
    EXPECT_EQ(conv.get_messages()[0].role, "tool");
    EXPECT_EQ(conv.get_messages()[0].tool_call_id, "tool_123");
    EXPECT_EQ(conv.get_messages()[0].content, "result data");
}

TEST(ConversationTest, MessageOrder) {
    Conversation conv;
    conv.add_user_msg("Q");
    conv.add_assistant_msg("A", "calc", "t1", json::object());
    conv.add_tool_result("t1", "42");

    auto msgs = conv.get_messages();
    EXPECT_EQ(msgs.size(), 3u);
    EXPECT_EQ(msgs[0].role, "user");
    EXPECT_EQ(msgs[1].role, "assistant");
    EXPECT_EQ(msgs[1].tool_name, "calc");
    EXPECT_EQ(msgs[2].role, "tool");
}

TEST(ConversationTest, Clear) {
    Conversation conv;
    conv.add_user_msg("test");
    conv.clear();
    EXPECT_TRUE(conv.empty());
}
