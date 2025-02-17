/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include "velox/core/SimpleFunctionMetadata.h"

// Test for simple function type analysis.
namespace facebook::velox::core {
namespace {
class TypeAnalysisTest : public testing::Test {
 protected:
  template <typename... Args>
  void testHasGeneric(bool expecetd) {
    TypeAnalysisResults results;
    (TypeAnalysis<Args>().run(results), ...);
    ASSERT_EQ(expecetd, results.hasGeneric);
  }

  template <typename... Args>
  void testHasVariadic(bool expecetd) {
    TypeAnalysisResults results;
    (TypeAnalysis<Args>().run(results), ...);
    ASSERT_EQ(expecetd, results.hasVariadic);
  }

  template <typename... Args>
  void testHasVariadicOfGeneric(bool expecetd) {
    TypeAnalysisResults results;
    (TypeAnalysis<Args>().run(results), ...);
    ASSERT_EQ(expecetd, results.hasVariadicOfGeneric);
  }

  template <typename... Args>
  void testCountConcrete(size_t expecetd) {
    TypeAnalysisResults results;
    (TypeAnalysis<Args>().run(results), ...);
    ASSERT_EQ(expecetd, results.concreteCount);
  }

  template <typename... Args>
  void testStringType(const std::vector<std::string>& expected) {
    TypeAnalysisResults results;
    std::vector<std::string> types;

    (
        [&]() {
          // Clear string representation but keep other collected information to
          // accumulate.
          results.resetTypeString();
          TypeAnalysis<Args>().run(results);
          types.push_back(results.typeAsString());
        }(),
        ...);
    ASSERT_EQ(expected, types);
  }

  template <typename... Args>
  void testVariables(const std::set<std::string>& expected) {
    TypeAnalysisResults results;
    (TypeAnalysis<Args>().run(results), ...);
    ASSERT_EQ(expected, results.variables);
  }
};

TEST_F(TypeAnalysisTest, hasGeneric) {
  testHasGeneric<int32_t>(false);
  testHasGeneric<int32_t, int32_t>(false);
  testHasGeneric<Variadic<int32_t>>(false);
  testHasGeneric<Map<Array<int32_t>, Array<int32_t>>>(false);

  testHasGeneric<Map<Array<Generic<>>, Array<int32_t>>>(true);
  testHasGeneric<Map<Array<Generic<T1>>, Array<int32_t>>>(true);
  testHasGeneric<Map<Array<int32_t>, Generic<>>>(true);
  testHasGeneric<Variadic<Generic<>>>(true);
  testHasGeneric<Generic<>>(true);
  testHasGeneric<int32_t, Generic<>>(true);
  testHasGeneric<Generic<>, int32_t>(true);
}

TEST_F(TypeAnalysisTest, hasVariadic) {
  testHasVariadic<int32_t>(false);
  testHasVariadic<Map<Array<int32_t>, Array<int32_t>>>(false);
  testHasVariadic<Map<Array<int32_t>, Generic<>>>(false);
  testHasVariadic<int32_t, Array<int32_t>>(false);

  testHasVariadic<Variadic<int32_t>>(true);
  testHasVariadic<Variadic<Generic<>>>(true);
  testHasVariadic<Variadic<int64_t>, Array<int32_t>>(true);
  testHasVariadic<int32_t, Variadic<Array<int32_t>>>(true);
}

TEST_F(TypeAnalysisTest, hasVariadicOfGeneric) {
  testHasVariadicOfGeneric<int32_t>(false);
  testHasVariadicOfGeneric<Map<Array<int32_t>, Array<int32_t>>>(false);
  testHasVariadicOfGeneric<Map<Array<int32_t>, Generic<>>>(false);
  testHasVariadicOfGeneric<int32_t, Array<int32_t>>(false);
  testHasVariadicOfGeneric<Variadic<int32_t>>(false);
  testHasVariadicOfGeneric<Variadic<int64_t>, Array<int32_t>>(false);
  testHasVariadicOfGeneric<int32_t, Variadic<Array<int32_t>>>(false);
  testHasVariadicOfGeneric<Variadic<int32_t>, Generic<>>(false);
  testHasVariadicOfGeneric<Generic<>, Variadic<int32_t>>(false);

  testHasVariadicOfGeneric<Variadic<Generic<>>>(true);
  testHasVariadicOfGeneric<Variadic<Generic<>>, int32_t>(true);
  testHasVariadicOfGeneric<int32_t, Variadic<Array<Generic<>>>>(true);
  testHasVariadicOfGeneric<int32_t, Variadic<Map<int64_t, Array<Generic<T1>>>>>(
      true);
}

TEST_F(TypeAnalysisTest, countConcrete) {
  testCountConcrete<>(0);
  testCountConcrete<int32_t>(1);
  testCountConcrete<int32_t, int32_t>(2);
  testCountConcrete<int32_t, int32_t, double>(3);
  testCountConcrete<Generic<>>(0);
  testCountConcrete<Generic<T1>>(0);
  testCountConcrete<Variadic<Generic<>>>(0);
  testCountConcrete<Variadic<int32_t>>(1);
  testCountConcrete<Variadic<Array<Generic<>>>>(1);

  testCountConcrete<Map<Array<int32_t>, Array<int32_t>>>(5);
  testCountConcrete<Map<Array<int32_t>, Generic<>>>(3);
  testCountConcrete<int32_t, Array<int32_t>>(3);
  testCountConcrete<Variadic<int64_t>, Array<int32_t>>(3);
  testCountConcrete<int32_t, Variadic<Array<int32_t>>>(3);
  testCountConcrete<Variadic<int32_t>, Generic<>>(1);
  testCountConcrete<Generic<>, Variadic<int32_t>>(1);

  testCountConcrete<Variadic<Generic<>>>(0);
  testCountConcrete<Variadic<Generic<>>, int32_t>(1);
  testCountConcrete<int32_t, Variadic<Array<Generic<>>>>(2);
}

TEST_F(TypeAnalysisTest, testStringType) {
  testStringType<int32_t>({"integer"});
  testStringType<int64_t>({"bigint"});
  testStringType<double>({"double"});
  testStringType<float>({"real"});
  testStringType<Array<int32_t>>({"array(integer)"});
  testStringType<Generic<>>({"any"});
  testStringType<Generic<T1>>({"__user_T1"});
  testStringType<Map<Generic<>, int32_t>>({"map(any,integer)"});
  testStringType<Variadic<int32_t>>({"integer"});

  testStringType<int32_t, int64_t, Map<Array<int32_t>, Generic<T2>>>({
      "integer",
      "bigint",
      "map(array(integer),__user_T2)",
  });
}

TEST_F(TypeAnalysisTest, testVariables) {
  testVariables<int32_t>({});
  testVariables<Array<int32_t>>({});
  testVariables<Generic<>>({});
  testVariables<Generic<T1>>({"__user_T1"});
  testVariables<Map<Generic<>, int32_t>>({});
  testVariables<Variadic<int32_t>>({});
  testVariables<int32_t, Generic<T5>, Map<Array<int32_t>, Generic<T2>>>(
      {"__user_T2", "__user_T5"});
}

} // namespace
} // namespace facebook::velox::core
