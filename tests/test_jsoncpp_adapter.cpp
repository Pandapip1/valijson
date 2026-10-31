
#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include <valijson/adapters/jsoncpp_adapter.hpp>

class TestJsonCppAdapter : public testing::Test
{

};

TEST_F(TestJsonCppAdapter, BasicArrayIteration)
{
    const unsigned int numElements = 10;

    // Create a jsoncpp document that consists of an array of numbers
    Json::Value document(Json::arrayValue);
    for (unsigned int i = 0; i < numElements; i++) {
        document.append(Json::Value(i));
    }

    // Ensure that wrapping the document preserves the array and does not allow
    // it to be cast to other types
    valijson::adapters::JsonCppAdapter adapter(document);
#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW( adapter.getArray() );
    ASSERT_ANY_THROW( adapter.getBool() );
    ASSERT_ANY_THROW( adapter.getDouble() );
    ASSERT_ANY_THROW( adapter.getObject() );
    ASSERT_ANY_THROW( adapter.getString() );
#endif
    // Ensure that the array contains the expected number of elements
    EXPECT_EQ( numElements, adapter.getArray().size() );

    // Ensure that the elements are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::JsonCppAdapter value : adapter.getArray()) {
        ASSERT_TRUE( value.isNumber() );
        EXPECT_EQ( double(expectedValue), value.getNumber() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ(numElements, expectedValue);
}

TEST_F(TestJsonCppAdapter, BasicObjectIteration)
{
    const unsigned int numElements = 10;

    // Create a jsoncpp document that consists of an object that maps numeric
    // strings their corresponding numeric values
    Json::Value document(Json::objectValue);
    for (unsigned int i = 0; i < numElements; i++) {
        std::string name(std::to_string(i));
        document[name] = Json::Value(double(i));
    }

    // Ensure that wrapping the document preserves the object and does not
    // allow it to be cast to other types
    valijson::adapters::JsonCppAdapter adapter(document);
#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW( adapter.getObject() );
    ASSERT_ANY_THROW( adapter.getArray() );
    ASSERT_ANY_THROW( adapter.getBool() );
    ASSERT_ANY_THROW( adapter.getDouble() );
    ASSERT_ANY_THROW( adapter.getString() );
#endif

    // Ensure that the object contains the expected number of members
    EXPECT_EQ( numElements, adapter.getObject().size() );

    // Ensure that the members are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::JsonCppAdapter::ObjectMember member : adapter.getObject()) {
        ASSERT_TRUE( member.second.isNumber() );
        EXPECT_EQ( std::to_string(expectedValue), member.first );
        EXPECT_EQ( double(expectedValue), member.second.getDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ( numElements, expectedValue );
}

TEST_F(TestJsonCppAdapter, GetIntegerDoesNotThrowForLargeInt64Values)
{
    // asInt() is 32-bit and throws Json::LogicError for a value this large,
    // even though it fits in int64_t (the function's own contract).
    Json::Value document(static_cast<Json::Int64>(4000000000));
    valijson::adapters::JsonCppAdapter adapter(document);

    ASSERT_TRUE(adapter.isInteger());

    int64_t result = 0;
#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW(adapter.getInteger(result));
#endif
    ASSERT_TRUE(adapter.getInteger(result));
    EXPECT_EQ(4000000000, result);
}

TEST_F(TestJsonCppAdapter, IsIntegerFalseForValuesOutsideInt64Range)
{
    // isIntegral() only checks the value's stored type tag, not whether it
    // actually fits int64_t - a uintValue this large passes isIntegral() but
    // getInteger()'s asInt64() then throws, contradicting isInteger()==true.
    Json::Value document(static_cast<Json::UInt64>(10000000000000000000ULL));
    valijson::adapters::JsonCppAdapter adapter(document);

    ASSERT_FALSE(adapter.isInteger());

    int64_t result = 0;
#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW(adapter.getInteger(result));
#endif
    ASSERT_FALSE(adapter.getInteger(result));
}
