#include <string>

#include <gtest/gtest.h>

#include <valijson/adapters/qvariant_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

class TestQtVariantAdapter : public testing::Test
{

};

TEST_F(TestQtVariantAdapter, BasicArrayIteration)
{
    const unsigned int numElements = 10;

    // Create a QVariant document that consists of an array of numbers
    QVariantList array;
    for (unsigned int i = 0; i < numElements; i++) {
        QVariant value(static_cast<double>(i));
        array.append(value);
    }
    QVariant document(array);

    // Ensure that wrapping the document preserves the array and does not allow
    // it to be cast to other types
    valijson::adapters::QtVariantAdapter adapter(document);
    ASSERT_NO_THROW( adapter.getArray() );
    ASSERT_ANY_THROW( adapter.getBool() );
    ASSERT_ANY_THROW( adapter.getDouble() );
    ASSERT_ANY_THROW( adapter.getObject() );
    ASSERT_ANY_THROW( adapter.getString() );

    // Ensure that the array contains the expected number of elements
    EXPECT_EQ( numElements, adapter.getArray().size() );

    // Ensure that the elements are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::QtVariantAdapter value : adapter.getArray()) {
        ASSERT_TRUE( value.isNumber() );
        EXPECT_EQ( double(expectedValue), value.getDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ(numElements, expectedValue);
}

TEST_F(TestQtVariantAdapter, BasicObjectIteration)
{
    const unsigned int numElements = 10;

    // Create a QVariant document that consists of an object that maps numeric
    // strings their corresponding numeric values
    QVariantMap object;
    for (unsigned int i = 0; i < numElements; i++) {
        QString name(QString::number(i));
        object[name] = QVariant(static_cast<double>(i));
    }
    QVariant document(object);

    // Ensure that wrapping the document preserves the object and does not
    // allow it to be cast to other types
    valijson::adapters::QtVariantAdapter adapter(document);
    ASSERT_NO_THROW( adapter.getObject() );
    ASSERT_ANY_THROW( adapter.getArray() );
    ASSERT_ANY_THROW( adapter.getBool() );
    ASSERT_ANY_THROW( adapter.getDouble() );
    ASSERT_ANY_THROW( adapter.getString() );

    // Ensure that the object contains the expected number of members
    EXPECT_EQ( numElements, adapter.getObject().size() );

    // Ensure that the members are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::QtVariantAdapter::ObjectMember member : adapter.getObject()) {
        ASSERT_TRUE( member.second.isNumber() );
        EXPECT_EQ( std::to_string(expectedValue), member.first );
        EXPECT_EQ( double(expectedValue), member.second.getDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ( numElements, expectedValue );
}

TEST_F(TestQtVariantAdapter, GetIntegerFailsForUnsignedValuesOutsideInt64Range)
{
    // QVariant::toLongLong()'s "ok" output doesn't catch an unsigned value
    // too large for int64_t - it previously returned ok=true with the value
    // silently reinterpreted as negative instead of failing.
    QVariant document(static_cast<qulonglong>(10000000000000000000ULL));
    valijson::adapters::QtVariantAdapter adapter(document);

    ASSERT_TRUE( adapter.isInteger() );

    int64_t result = 0;
    ASSERT_FALSE( adapter.getInteger(result) );
}

TEST_F(TestQtVariantAdapter, EnumComparisonDoesNotThrowForValuesOutsideInt64Range)
{
    // isInteger() stays true for this value by design, so EnumConstraint
    // reaches BasicAdapter::equalTo()'s numeric comparison path, which
    // previously called the throwing getNumber()/asInteger() accessors
    // directly - dying via an uncaught throw (VALIJSON_USE_EXCEPTIONS=1) or
    // abort() (VALIJSON_USE_EXCEPTIONS=0). Neither should happen now.
    using namespace valijson;

    QVariantMap schemaMap;
    schemaMap["enum"] = QVariantList{5};
    QVariant schemaJson(schemaMap);
    Schema schema;
    SchemaParser parser;
    valijson::adapters::QtVariantAdapter schemaAdapter(schemaJson);
    parser.populateSchema(schemaAdapter, schema);

    QVariant document(static_cast<qulonglong>(10000000000000000000ULL));
    valijson::adapters::QtVariantAdapter adapter(document);

    Validator strictValidator(Validator::kStrongTypes);
    EXPECT_FALSE( strictValidator.validate(schema, adapter, nullptr) );

    Validator weakValidator(Validator::kWeakTypes);
    EXPECT_FALSE( weakValidator.validate(schema, adapter, nullptr) );
}

TEST_F(TestQtVariantAdapter, MaximumComparisonDoesNotThrowForValuesOutsideInt64Range)
{
    // MaximumConstraint/MinimumConstraint had the same unfixed asDouble()
    // call as EnumConstraint above, reached via maybeDouble() (also
    // permissive for this value), so they died the same way.
    using namespace valijson;

    QVariantMap schemaMap;
    schemaMap["maximum"] = 10;
    QVariant schemaJson(schemaMap);
    Schema schema;
    SchemaParser parser;
    valijson::adapters::QtVariantAdapter schemaAdapter(schemaJson);
    parser.populateSchema(schemaAdapter, schema);

    QVariant document(static_cast<qulonglong>(10000000000000000000ULL));
    valijson::adapters::QtVariantAdapter adapter(document);

    Validator strictValidator(Validator::kStrongTypes);
    EXPECT_FALSE( strictValidator.validate(schema, adapter, nullptr) );

    Validator weakValidator(Validator::kWeakTypes);
    EXPECT_FALSE( weakValidator.validate(schema, adapter, nullptr) );
}

TEST_F(TestQtVariantAdapter, GetIntegerStillWorksForSmallUnsignedValues)
{
    QVariant document(static_cast<qulonglong>(5ULL));
    valijson::adapters::QtVariantAdapter adapter(document);

    ASSERT_TRUE( adapter.isInteger() );

    int64_t result = 0;
    ASSERT_TRUE( adapter.getInteger(result) );
    EXPECT_EQ( int64_t(5), result );
}
