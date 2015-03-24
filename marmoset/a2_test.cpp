#include "marmoset/common_test.h"

namespace marmoset {

const static string kStdlib2 = "third_party/cs444/stdlib/2.0";

const static string kTest2 = "third_party/cs444/assignment_testcases/a2";

INSTANTIATE_TEST_CASE_P(MarmosetA2, CompilerSuccessTest,
    testing::ValuesIn(GetGoodInputs(
        kStdlib2, kTest2, CompilerStage::TYPE_CHECK)));
INSTANTIATE_TEST_CASE_P(MarmosetA2, CompilerFailureTest,
    testing::ValuesIn(GetBadInputs(
        kStdlib2, kTest2, CompilerStage::TYPE_CHECK)));

} // namespace marmoset
