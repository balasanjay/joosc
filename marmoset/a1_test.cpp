#include "marmoset/common_test.h"

namespace marmoset {

const static string kStdlib1 = "";

const static string kTest1 = "third_party/cs444/assignment_testcases/a1";

INSTANTIATE_TEST_CASE_P(MarmosetA1, CompilerSuccessTest,
    testing::ValuesIn(GetGoodInputs(
        kStdlib1, kTest1, CompilerStage::WEED)));
INSTANTIATE_TEST_CASE_P(MarmosetA1, CompilerFailureTest,
    testing::ValuesIn(GetBadInputs(
        kStdlib1, kTest1, CompilerStage::WEED)));

} // namespace marmoset
