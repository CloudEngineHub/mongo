/**
 *    Copyright (C) 2018-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#include "mongo/db/matcher/matcher_type_set.h"

#include "mongo/base/error_codes.h"
#include "mongo/base/status.h"
#include "mongo/bson/bsonmisc.h"
#include "mongo/bson/bsonobj.h"
#include "mongo/bson/bsonobjbuilder.h"
#include "mongo/bson/json.h"
#include "mongo/platform/decimal128.h"
#include "mongo/unittest/unittest.h"

#include <cmath>
#include <limits>
#include <string>

namespace mongo {
namespace {

TEST(MatcherTypeSetTest, ParseFromStringAliasesCanParseNumberAlias) {
    auto result = MatcherTypeSet::fromStringAliases({"number"}, findBSONTypeAlias);
    ASSERT_OK(result.getStatus());
    ASSERT_TRUE(result.getValue().allNumbers);
    ASSERT_EQ(result.getValue().bsonTypes.size(), 0u);
}

TEST(MatcherTypeSetTest, ParseFromStringAliasesCanParseLongAlias) {
    auto result = MatcherTypeSet::fromStringAliases({"long"}, findBSONTypeAlias);
    ASSERT_OK(result.getStatus());
    ASSERT_FALSE(result.getValue().allNumbers);
    ASSERT_EQ(result.getValue().bsonTypes.size(), 1u);
    ASSERT_TRUE(result.getValue().hasType(BSONType::numberLong));
}

TEST(MatcherTypeSetTest, ParseFromStringAliasesCanParseMultipleTypes) {
    auto result =
        MatcherTypeSet::fromStringAliases({"number", "object", "string"}, findBSONTypeAlias);
    ASSERT_OK(result.getStatus());
    ASSERT_TRUE(result.getValue().allNumbers);
    ASSERT_EQ(result.getValue().bsonTypes.size(), 2u);
    ASSERT_TRUE(result.getValue().hasType(BSONType::object));
    ASSERT_TRUE(result.getValue().hasType(BSONType::string));
}

TEST(MatcherTypeSetTest, ParseFromStringAliasesCanParseEmptySet) {
    auto result = MatcherTypeSet::fromStringAliases({}, findBSONTypeAlias);
    ASSERT_OK(result.getStatus());
    ASSERT_TRUE(result.getValue().isEmpty());
}

TEST(MatcherTypeSetTest, ParseFromStringFailsToParseUnknownAlias) {
    auto result = MatcherTypeSet::fromStringAliases({"long", "unknown"}, findBSONTypeAlias);
    ASSERT_NOT_OK(result.getStatus());
}

TEST(MatcherTypeSetTest, ParseFromElementCanParseNumberAlias) {
    auto obj = BSON("" << "number");
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_OK(result.getStatus());
    ASSERT_TRUE(result.getValue().allNumbers);
    ASSERT_TRUE(result.getValue().bsonTypes.empty());
}

TEST(MatcherTypeSetTest, ParseFromElementCanParseLongAlias) {
    auto obj = BSON("" << "long");
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_OK(result.getStatus());
    ASSERT_FALSE(result.getValue().allNumbers);
    ASSERT_EQ(result.getValue().bsonTypes.size(), 1u);
    ASSERT_TRUE(result.getValue().hasType(BSONType::numberLong));
}

TEST(MatcherTypeSetTest, ParseFromElementFailsToParseUnknownAlias) {
    auto obj = BSON("" << "unknown");
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());
}

TEST(MatcherTypeSetTest, ParseFromElementFailsToParseWrongElementType) {
    auto obj = BSON("" << BSON("" << ""));
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());

    obj = fromjson("{'': null}");
    result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());
}

TEST(MatcherTypeSetTest, ParseFromElementFailsToParseUnknownBSONType) {
    auto obj = BSON("" << 99);
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());
}

TEST(MatcherTypeSetTest, ParseFromElementFailsToParseEOOTypeCode) {
    auto obj = BSON("" << 0);
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());
    ASSERT_EQ(result.getStatus().code(), ErrorCodes::BadValue);
    ASSERT_EQ(result.getStatus().reason(),
              "Invalid numerical type code: 0. Instead use {$exists:false}.");
}

TEST(MatcherTypeSetTest, ParseFromElementFailsToParseEOOTypeName) {
    auto obj = BSON("" << "missing");
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());
    ASSERT_EQ(result.getStatus().code(), ErrorCodes::BadValue);
    ASSERT_EQ(result.getStatus().reason(),
              "'missing' is not a legal type name. "
              "To query for non-existence of a field, use {$exists:false}.");
}

TEST(MatcherTypeSetTest, ParseFromElementCanParseRoundDoubleTypeCode) {
    auto obj = BSON("" << 2.0);
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_OK(result.getStatus());
    ASSERT_FALSE(result.getValue().allNumbers);
    ASSERT_EQ(result.getValue().bsonTypes.size(), 1u);
    ASSERT_TRUE(result.getValue().hasType(BSONType::string));
}

TEST(MatcherTypeSetTest, ParseFailsWhenElementIsNonRoundDoubleTypeCode) {
    auto obj = BSON("" << 2.5);
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());
}

TEST(MatcherTypeSetTest, ParseFromElementCanParseRoundDecimalTypeCode) {
    auto obj = BSON("" << Decimal128(2));
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_OK(result.getStatus());
    ASSERT_FALSE(result.getValue().allNumbers);
    ASSERT_EQ(result.getValue().bsonTypes.size(), 1U);
    ASSERT_TRUE(result.getValue().hasType(BSONType::string));
}

TEST(MatcherTypeSetTest, ParseFailsWhenElementIsNonRoundDecimalTypeCode) {
    auto obj = BSON("" << Decimal128(2.5));
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());
}

TEST(MatcherTypeSetTest, ParseFailsWhenDoubleElementIsTooPositiveForInteger) {
    double doubleTooLarge = scalbn(1, std::numeric_limits<long long>::digits);
    auto obj = BSON("" << doubleTooLarge);
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());
}

TEST(MatcherTypeSetTest, ParseFailsWhenDoubleElementIsTooNegativeForInteger) {
    double doubleTooNegative = std::numeric_limits<double>::lowest();
    auto obj = BSON("" << doubleTooNegative);
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());
}

TEST(MatcherTypeSetTest, ParseFailsWhenDoubleElementIsNaN) {
    auto obj = BSON("" << std::nan(""));
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());
}

TEST(MatcherTypeSetTest, ParseFailsWhenDoubleElementIsInfinite) {
    auto obj = BSON("" << std::numeric_limits<double>::infinity());
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());
}

TEST(MatcherTypeSetTest, ParseFailsWhenDecimalElementIsTooPositiveForInteger) {
    auto obj = BSON("" << Decimal128(static_cast<int64_t>(std::numeric_limits<int>::max()) + 1));
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());
}

TEST(MatcherTypeSetTest, ParseFailsWhenDecimalElementIsTooNegativeForInteger) {
    auto obj = BSON("" << Decimal128(static_cast<int64_t>(std::numeric_limits<int>::min()) - 1));
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());
}

TEST(MatcherTypeSetTest, ParseFailsWhenDecimalElementIsNaN) {
    auto obj = BSON("" << Decimal128::kPositiveNaN);
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());
}

TEST(MatcherTypeSetTest, ParseFailsWhenDecimalElementIsInfinite) {
    auto obj = BSON("" << Decimal128::kPositiveInfinity);
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());
}

TEST(MatcherTypeSetTest, ParseFromElementFailsWhenArrayHasUnknownType) {
    auto obj = BSON("" << BSON_ARRAY("long" << "unknown"));
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());
}

TEST(MatcherTypeSetTest, ParseFailsWhenArrayElementIsNotStringOrNumber) {
    auto obj = BSON("" << BSON_ARRAY("long" << BSONObj()));
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_NOT_OK(result.getStatus());
}

TEST(MatcherTypeSetTest, ParseFromElementCanParseEmptyArray) {
    auto obj = BSON("" << BSONArray());
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_OK(result.getStatus());
    ASSERT_TRUE(result.getValue().isEmpty());
}

TEST(MatcherTypeSetTest, ParseFromElementCanParseArrayWithSingleType) {
    auto obj = BSON("" << BSON_ARRAY("string"));
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_OK(result.getStatus());
    ASSERT_FALSE(result.getValue().allNumbers);
    ASSERT_EQ(result.getValue().bsonTypes.size(), 1u);
    ASSERT_TRUE(result.getValue().hasType(BSONType::string));
}

TEST(MatcherTypeSetTest, ParseFromElementCanParseArrayWithMultipleTypes) {
    auto obj = BSON("" << BSON_ARRAY("string" << 3 << "number"));
    auto result = MatcherTypeSet::parse(obj.firstElement());
    ASSERT_OK(result.getStatus());
    ASSERT_TRUE(result.getValue().allNumbers);
    ASSERT_EQ(result.getValue().bsonTypes.size(), 2u);
    ASSERT_TRUE(result.getValue().hasType(BSONType::string));
    ASSERT_TRUE(result.getValue().hasType(BSONType::object));
}

TEST(MatcherTypeSetTest, IsSingleTypeTrueForTypeNumber) {
    MatcherTypeSet typeSet;
    typeSet.allNumbers = true;
    ASSERT_TRUE(typeSet.isSingleType());
}

TEST(MatcherTypeSetTest, IsSingleTypeTrueForSingleBSONType) {
    MatcherTypeSet typeSet{BSONType::numberLong};
    ASSERT_TRUE(typeSet.isSingleType());
}

TEST(MatcherTypeSetTest, IsSingleTypeFalseForEmpty) {
    MatcherTypeSet typeSet;
    ASSERT_FALSE(typeSet.isSingleType());
}

TEST(MatcherTypeSetTest, IsSingleTypeFalseForMultipleTypes) {
    MatcherTypeSet typeSet{BSONType::numberLong};
    typeSet.allNumbers = true;
    ASSERT_FALSE(typeSet.isSingleType());
}

TEST(MatcherTypeSetTest, IsEmptyTrueForEmpty) {
    MatcherTypeSet typeSet;
    ASSERT_TRUE(typeSet.isEmpty());
}

TEST(MatcherTypeSetTest, IsEmptyFalseForAllNumbers) {
    MatcherTypeSet typeSet;
    typeSet.allNumbers = true;
    ASSERT_FALSE(typeSet.isEmpty());
}

TEST(MatcherTypeSetTest, IsEmptyFalseForBSONTypeNumberLong) {
    MatcherTypeSet typeSet{BSONType::numberLong};
    ASSERT_FALSE(typeSet.isEmpty());
}

TEST(MatcherTypeSetTest, HasTypeTrueForNumericalTypesWhenSetHasAllNumbersType) {
    MatcherTypeSet typeSet;
    typeSet.allNumbers = true;
    ASSERT_TRUE(typeSet.hasType(BSONType::numberInt));
    ASSERT_TRUE(typeSet.hasType(BSONType::numberLong));
    ASSERT_TRUE(typeSet.hasType(BSONType::numberDouble));
    ASSERT_TRUE(typeSet.hasType(BSONType::numberDecimal));
}

TEST(MatcherTypeSetTest, HasTypeTrueForSetOfTypes) {
    MatcherTypeSet typeSet;
    typeSet.bsonTypes.insert(BSONType::numberLong);
    typeSet.bsonTypes.insert(BSONType::string);
    ASSERT_FALSE(typeSet.hasType(BSONType::numberInt));
    ASSERT_TRUE(typeSet.hasType(BSONType::numberLong));
    ASSERT_FALSE(typeSet.hasType(BSONType::numberDouble));
    ASSERT_TRUE(typeSet.hasType(BSONType::string));
}

}  // namespace
}  // namespace mongo
