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

#include <folly/String.h>
#include <folly/init/Init.h>
#include <gtest/gtest.h>
#include <string>
#include <unordered_set>
#include <vector>

#include "velox/expression/tests/FuzzerRunner.h"
#include "velox/functions/prestosql/registration/RegistrationFunctions.h"

DEFINE_int64(
    seed,
    123456,
    "Initial seed for random number generator "
    "(use it to reproduce previous results).");

DEFINE_string(
    only,
    "",
    "If specified, Fuzzer will only choose functions from "
    "this comma separated list of function names "
    "(e.g: --only \"split\" or --only \"substr,ltrim\").");

DEFINE_int32(steps, 10, "Number of expressions to generate.");

int main(int argc, char** argv) {
  facebook::velox::functions::prestosql::registerAllScalarFunctions();

  ::testing::InitGoogleTest(&argc, argv);

  // Calls common init functions in the necessary order, initializing
  // singletons, installing proper signal handlers for better debugging
  // experience, and initialize glog and gflags.
  folly::init(&argc, &argv);

  // TODO: List of the functions that at some point crash or fail and need to
  // be fixed before we can enable.
  std::unordered_set<std::string> skipFunctions = {
      "replace",
      // The pad functions cause the test to OOM. The 2nd arg is only bound by
      // the max value of int32_t, which leads to strings billions of characters
      // long.
      "lpad",
      "rpad",
      // Fuzzer and the underlying engine are confused about cardinality(HLL)
      // (since HLL is a user defined type), and end up trying to use
      // cardinality passing a VARBINARY (since HLL's implementation uses an
      // alias to VARBINARY).
      "cardinality",
  };
  return FuzzerRunner::run(FLAGS_only, FLAGS_steps, FLAGS_seed, skipFunctions);
}
