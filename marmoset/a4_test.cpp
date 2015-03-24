#include "marmoset/common_test.h"

namespace marmoset {

const static string kStdlib4 = "third_party/cs444/stdlib/4.0";

const static string kTest4 = "third_party/cs444/assignment_testcases/a4";

INSTANTIATE_TEST_CASE_P(MarmosetA4, CompilerSuccessTest,
    testing::ValuesIn(GetGoodInputs(
        kStdlib4, kTest4, CompilerStage::TYPE_CHECK)));
INSTANTIATE_TEST_CASE_P(MarmosetA4, CompilerFailureTest,
    testing::ValuesIn(GetBadInputs(
        kStdlib4, kTest4, CompilerStage::TYPE_CHECK)));

} // namespace marmoset
