#include "base/error.h"
#include "lexer/lexer.h"
#include "parser/ast.h"
#include "third_party/gtest/gtest.h"
#include "weeder/weeder.h"

using base::ErrorList;
using base::FileSet;
using base::PosRange;
using lexer::StripSkippableTokens;
using lexer::FindUnsupportedTokens;
using lexer::Token;
using parser::Parse;
using parser::Program;

namespace weeder {

class CompilerSuccessTest : public testing::TestWithParam<string> {};

TEST_P(CompilerSuccessTest, ShouldCompile) {
  FileSet::Builder builder = FileSet::Builder().AddDiskFile(GetParam());

  FileSet *fs;
  ErrorList errors;
  vector<vector<Token>> tokens;

  ASSERT_TRUE(builder.Build(&fs, &errors));
  unique_ptr<FileSet> fs_deleter(fs);

  LexJoosFiles(fs, &tokens, &errors);
  ASSERT_TRUE(errors.Size() == 0);

  vector<vector<Token>> filtered_tokens;
  StripSkippableTokens(tokens, &filtered_tokens);

  FindUnsupportedTokens(fs, filtered_tokens, &errors);
  ASSERT_TRUE(errors.Size() == 0);

  unique_ptr<Program> prog = Parse(fs, filtered_tokens, &errors);
  if (errors.Size() != 0) {
    errors.PrintTo(&std::cerr, base::OutputOptions::kUserOutput);
    ASSERT_TRUE(false);
  }
  ASSERT_TRUE(prog != nullptr);

  WeedProgram(fs, prog.get(), &errors);
  if (errors.Size() != 0) {
    errors.PrintTo(&std::cerr, base::OutputOptions::kUserOutput);
    ASSERT_TRUE(false);
  }
  ASSERT_TRUE(prog != nullptr);
}

class CompilerFailureTest : public testing::TestWithParam<string> {};

TEST_P(CompilerFailureTest, ShouldNotCompile) {
  FileSet::Builder builder = FileSet::Builder().AddDiskFile(GetParam());

  FileSet *fs;
  ErrorList errors;
  vector<vector<Token>> tokens;

  ASSERT_TRUE(builder.Build(&fs, &errors));
  unique_ptr<FileSet> fs_deleter(fs);

  LexJoosFiles(fs, &tokens, &errors);
  if (errors.IsFatal()) {
    return;
  }

  vector<vector<Token>> filtered_tokens;
  StripSkippableTokens(tokens, &filtered_tokens);

  FindUnsupportedTokens(fs, filtered_tokens, &errors);
  if (errors.IsFatal()) {
    return;
  }

  unique_ptr<Program> prog = Parse(fs, filtered_tokens, &errors);
  if (errors.IsFatal()) {
    return;
  }

  WeedProgram(fs, prog.get(), &errors);
  if (errors.IsFatal()) {
    return;
  }

  // Shouldn't have made it here.
  ASSERT_TRUE(false);
}

vector<string> SuccessFiles();
vector<string> FailureFiles();

INSTANTIATE_TEST_CASE_P(MarmosetTests, CompilerSuccessTest, testing::ValuesIn(SuccessFiles()));
INSTANTIATE_TEST_CASE_P(MarmosetTests, CompilerFailureTest, testing::ValuesIn(FailureFiles()));

// find third_party/cs444/assignment_testcases/a1 -name "*.java" -not -name "*Je*.java".
vector<string> SuccessFiles() {
  vector<string> files;
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_01.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_1_AmbiguousName_AccessResultFromMethod.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_1_Cast_Complement.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_1_Cast_MultipleCastOfSameValue_1.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_1_Cast_MultipleCastOfSameValue_2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_1_Cast_MultipleCastOfSameValue_3.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_1_Cast_MultipleReferenceArray.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_1_Escapes_3DigitOctalAndDigit.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_1_Instanceof_InLazyExp.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_1_Instanceof_OfAdditiveExpression.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_1_Instanceof_OfCastExpression.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_1_IntRange_NegativeInt.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_ArrayCreateAndIndex.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_BigInt.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_CharCast.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_CharCharInit1.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_CharCharInit2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_EscapeEscape.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_ForUpdate_ClassCreation.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_IntArrayDecl1.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_IntArrayDecl2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_IntCast.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_IntCharInit.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_IntInit.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_IntRange_MinNegativeInt.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_IsThisACast.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_NamedTypeArray.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeByteCast.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeCharCast.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeIntCast1.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeIntCast2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeOneByteByteCast.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeOneByteCharCast.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeOneByteIntCast.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeOneByteShortCast.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeShortCast.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_SimpleTypeArray.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_SmallInt.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_abstractclass.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_abstractmethodwithoutbody.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_arbitrarylocaldeclaration.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_arithmeticoperations.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_assignment.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_assignmentExp.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_barminusfoo.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_char.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_char_escape.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_char_escape2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_char_escape3.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_charadd.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_charliterals.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_classinstance.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp1.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp3.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp4.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp5.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp6.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp7.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp8.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp9.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_comparisonoperations.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_concat_in_binop.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_constructorWithSameNameAsMethod.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_constructorbodycast.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_constructorparameter.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_eagerbooleanoperations.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_evalMethodInvocationFromParExp.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_exp.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_extends.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_externalcall.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_finalclass.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_finalclass2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_forAllwaysInit.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_forAlwaysInitAsWhile.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_forMethodInit.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_forMethodUpdate.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_forMethodUpdate2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_forWithoutExp.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_forWithoutInit.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_forWithoutUpdate.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_for_no_short_if.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_forbodycast.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_forifstatements1.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_forifstatements2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_forifstatements3.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_forinfor.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_forinitcast.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_forupdatecast.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_hello_comment.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_if.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_ifThenElse.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_if_then.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_if_then_for.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_implements.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_integerFun.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_integerFun1.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_integerFun3.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_intliterals.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_intminusfoo.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_lazybooleanoperations.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_maxint_comment.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_minuschar.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_minusminusminus.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_negativeintcast3.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_newobject.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_nonemptyconstructor.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_nullinstanceof1.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_nullliteral.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_octal_escape.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_octal_escape2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_octal_escape3.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_octal_escape4.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_octal_escape5.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_primitivecasts.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_protected.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_protectedfields.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_publicclasses.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_publicconstructors.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_publicfields.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_publicmethods.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_staticmethoddeclaration.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_stringliteralinvoke.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_stringliterals.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1_weird_chars.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1w_Interface.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1w_RestrictedNative.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J1w_StaticField.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J2_staticFieldDecl.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/J2_staticfielddeclaration.java");
  return files;
}

// find third_party/cs444/assignment_testcases/a1 -name "*Je*.java".
vector<string> FailureFiles() {
  vector<string> files;
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_Circularity_1.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_Circularity_2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_Circularity_3.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_Circularity_4_Rhoshaped.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_ClosestMatch_Array.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_ClosestMatch_Constructor_NoClosestMatch_This.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_IncDec_Final_ArrayLengthDec.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_IncDec_Final_ArrayLengthInc.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_IncDec_Final_PostDec.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_IncDec_Final_PostInc.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_IncDec_Final_PreDec.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_IncDec_Final_PreInc.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_IncDec_StringPostDec.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_IncDec_StringPostInc.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_IncDec_StringPreDec.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_IncDec_StringPreInc.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_MultiArrayCreation_Assign_1.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_MultiArrayCreation_Null.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_StaticThis_ArgumentToSuper.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_StaticThis_ArgumentToThis.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_SuperThis_InvalidSuperParameter.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_SuperThis_InvalidThisParameter.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_Throw_NoThrows.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_Throw_NotSubclass.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_Throw_SimpleType.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_Throw_ThrowsNotSuperclass.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_16_Throws_This.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_17_Unreachable_AfterThrow.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_17_Unreachable_AfterThrowInConditional.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_AbstractClass_AbstractConstructor.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_AbstractClass_Final.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_AbstractMethodCannotBeFinal.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_AbstractMethod_Body.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_AbstractMethod_EmptyBody.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_AbstractMethod_Final.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_AbstractMethod_Static.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Access_PrivateLocal.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Access_ProtectedLocal.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Access_PublicLocal.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Array_Data.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Array_Data_Empty.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Array_OnVariableNameInDecl.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_CastToArrayLvalue.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Cast_DoubleParenthese.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Cast_Expression.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Cast_LeftHandSideOfAssignment_1.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Cast_LeftHandSideOfAssignment_2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Cast_NoParenthesis.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Cast_NonstaticField.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Cast_ToMethodInvoke.java");
  // files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_ClassDeclaration_WrongFileName.java");
  // files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_ClassDeclaration_WrongFileName_Dot.foo.java");
  // files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_ClassDeclaration_WrongFileName_Suffix.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_ClassInstantiation_InstantiateSimpleType.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_ClassInstantiation_InstantiateSimpleValue.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Declarations_MultipleVars.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Declarations_MultipleVars_Fields.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Escapes_1DigitOctal_1.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Escapes_1DigitOctal_2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Escapes_1DigitOctal_3.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Escapes_1DigitOctal_4.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Escapes_2DigitOctal_1.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Escapes_2DigitOctal_2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Escapes_2DigitOctal_3.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Escapes_3DigitOctal_1.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Escapes_3DigitOctal_2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Escapes_3DigitOctal_3.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Extends_NamedTypeArray.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Escapes_NonExistingEscape.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Extends_SimpleType.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Extends_SimpleTypeArray.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Extends_Value.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_FinalField_NoInitializer.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_For_DeclarationInUpdate.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_For_MultipleDeclarationsInInit.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_For_MultipleUpdates.java");
  // files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_For_NotAStatementInUpdate.java");
  // files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_For_PrimaryExpInInit.java");
  // files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_For_PrimaryExpInUpdate.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_For_StatementInInit.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Formals_Final.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Formals_Initializer_Constructor.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Formals_Initializer_Method.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Identifiers_Goto.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Identifiers_Private.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Implements_NamedTypeArray.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Implements_SimpleType.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Implements_SimpleTypeArray.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Implements_Value.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_IncDec_IncDecNotLvalue.java");
  // files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_IncDec_Parenthesized.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_InstanceInitializers.java");
  // files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_InstanceOf_Null.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_InstanceOf_Primitive.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_InstanceOf_Void.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_IntRange_MinusTooBigInt.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_IntRange_PlusTooBigInt.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_IntRange_TooBigInt.java");
  // files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_IntRange_TooBigIntNegated.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_IntRange_TooBigInt_InInitializer.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Interface_ConstructorAbstract.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Interface_ConstructorBody.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Interface_Field.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Interface_FinalMethod.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Interface_MethodBody.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Interface_NoBody.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Interface_StaticMethod.java");
  // files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Interface_WrongFileName.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_JoosTypes_Double.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_JoosTypes_Float.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_JoosTypes_Long.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_LabeledStatements.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Literals_Class.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Literals_Exponential.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Literals_Float.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Literals_Hex.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Literals_Long.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Literals_Octal.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Locals_Final.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Methods_MissingAccessModifier.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Methods_NonAbstractNoBody.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Methods_StaticFinal.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_MultiArrayCreation_Assign_2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_MultiArrayCreation_MissingDimension_1.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_MultiArrayCreation_MissingDimension_2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_MultiArrayCreation_MissingDimension_4.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_MultiArrayCreation_NoType.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_MultiArrayTypes_Dimensions.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NegIntTooLow.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_AssignmentOperations_BitwiseAnd.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_AssignmentOperations_BitwiseOr.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_AssignmentOperations_BitwiseXOR.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_AssignmentOperations_Divide.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_AssignmentOperations_Minus.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_AssignmentOperations_Multiply.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_AssignmentOperations_Plus.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_AssignmentOperations_Remainder.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_AssignmentOperations_ShiftLeft.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_AssignmentOperations_SignShiftRight.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_AssignmentOperations_ZeroShiftRight.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_BitShift_Left.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_BitShift_SignRight.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_BitShift_ZeroRight.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_Bitwise_Negation.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_Break.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_Choice.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_Continue.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_DoWhile.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_ExpressionSequence.java");
  // files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_MultipleTypesPrFile.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_NestedTypes.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_PrivateFields.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_PrivateMethods.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_StaticInitializers.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_Strictftp.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_SuperMethodCall.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_Switch.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_Synchronized.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_SynchronizedStatement.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_Transient.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_UnaryPlus.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_Unicode.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_NonJoosConstructs_Volatile.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_PackagePrivate_Class.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_PackagePrivate_Field.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_PackagePrivate_Method.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_SuperThis_SuperAfterBlock.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_SuperThis_SuperAfterStatement.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_SuperThis_SuperInBlock.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_SuperThis_SuperInMethod.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_SuperThis_SuperThis.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_SuperThis_ThisAfterStatement.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_SuperThis_ThisInMethod.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_SuperThis_TwoSuperCalls.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Throw_NotExpression.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Throws_Array.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Throws_SimpleType.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_Throws_Void.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_VoidType_ArrayCreation.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_VoidType_ArrayDeclaration.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_VoidType_Cast.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_VoidType_Field.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_VoidType_Formals.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_VoidType_Local.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_1_VoidType_VoidMethod.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_6_Assignable_Instanceof_SimpleTypeOfSimpleType.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_6_InstanceOf_Primitive_1.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_6_InstanceOf_Primitive_2.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_Native.java");
  files.push_back("third_party/cs444/assignment_testcases/a1/Je_Throws.java");
  return files;
}

} // namespace weeder
