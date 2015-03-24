#include "marmoset/common_test.h"

namespace marmoset {

const static string kStdlib3 = "third_party/cs444/stdlib/3.0";

const static string kTest3 = "third_party/cs444/assignment_testcases/a3";

INSTANTIATE_TEST_CASE_P(MarmosetA3, CompilerSuccessTest,
    testing::ValuesIn(GetGoodInputs(
        kStdlib3, kTest3, CompilerStage::TYPE_CHECK)));
INSTANTIATE_TEST_CASE_P(MarmosetA3, CompilerFailureTest,
    testing::ValuesIn(GetBadInputs(
        kStdlib3, kTest3, CompilerStage::TYPE_CHECK)));

} // namespace marmoset
