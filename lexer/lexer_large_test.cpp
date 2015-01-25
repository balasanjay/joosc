#include "lexer/lexer.h"
#include "base/error.h"
#include "third_party/gtest/gtest.h"

using base::ErrorList;
using base::FileSet;
using base::PosRange;

namespace lexer {


void AppendFiles(vector<string>* out);





TEST(LexerLargeTest, All) {
  vector<string> files;
  AppendFiles(&files);

  FileSet::Builder builder = FileSet::Builder();
  for (const auto& file : files) {
    builder.AddDiskFile(file);
  }

  FileSet *fs;
  ErrorList errors;
  vector<vector<Token>> tokens;

  ASSERT_TRUE(builder.Build(&fs, &errors));
  unique_ptr<FileSet> fs_deleter(fs);

  LexJoosFiles(fs, &tokens, &errors);
  EXPECT_FALSE(errors.IsFatal());
}

void AppendFiles(vector<string>* out) {
  // find third_party/cs444/assignment_testcases/a1 -name "*.java" -not -name "*Je*.java".
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_01.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_1_AmbiguousName_AccessResultFromMethod.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_1_Cast_Complement.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_1_Cast_MultipleCastOfSameValue_1.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_1_Cast_MultipleCastOfSameValue_2.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_1_Cast_MultipleCastOfSameValue_3.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_1_Cast_MultipleReferenceArray.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_1_Escapes_3DigitOctalAndDigit.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_1_Instanceof_InLazyExp.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_1_Instanceof_OfAdditiveExpression.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_1_Instanceof_OfCastExpression.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_1_IntRange_NegativeInt.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_ArrayCreateAndIndex.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_BigInt.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_CharCast.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_CharCharInit1.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_CharCharInit2.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_EscapeEscape.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_ForUpdate_ClassCreation.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_IntArrayDecl1.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_IntArrayDecl2.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_IntCast.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_IntCharInit.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_IntInit.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_IntRange_MinNegativeInt.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_IsThisACast.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_NamedTypeArray.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeByteCast.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeCharCast.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeIntCast1.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeIntCast2.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeOneByteByteCast.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeOneByteCharCast.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeOneByteIntCast.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeOneByteShortCast.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_NegativeShortCast.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_SimpleTypeArray.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_SmallInt.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_abstractclass.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_abstractmethodwithoutbody.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_arbitrarylocaldeclaration.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_arithmeticoperations.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_assignment.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_assignmentExp.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_barminusfoo.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_char.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_char_escape.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_char_escape2.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_char_escape3.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_charadd.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_charliterals.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_classinstance.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp1.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp2.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp3.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp4.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp5.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp6.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp7.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp8.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_commentsInExp9.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_comparisonoperations.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_concat_in_binop.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_constructorWithSameNameAsMethod.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_constructorbodycast.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_constructorparameter.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_eagerbooleanoperations.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_evalMethodInvocationFromParExp.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_exp.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_extends.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_externalcall.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_finalclass.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_finalclass2.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_forAllwaysInit.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_forAlwaysInitAsWhile.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_forMethodInit.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_forMethodUpdate.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_forMethodUpdate2.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_forWithoutExp.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_forWithoutInit.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_forWithoutUpdate.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_for_no_short_if.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_forbodycast.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_forifstatements1.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_forifstatements2.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_forifstatements3.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_forinfor.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_forinitcast.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_forupdatecast.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_hello_comment.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_if.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_ifThenElse.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_if_then.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_if_then_for.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_implements.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_integerFun.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_integerFun1.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_integerFun3.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_intliterals.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_intminusfoo.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_lazybooleanoperations.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_maxint_comment.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_minuschar.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_minusminusminus.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_negativeintcast3.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_newobject.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_nonemptyconstructor.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_nullinstanceof1.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_nullliteral.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_octal_escape.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_octal_escape2.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_octal_escape3.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_octal_escape4.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_octal_escape5.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_primitivecasts.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_protected.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_protectedfields.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_publicclasses.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_publicconstructors.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_publicfields.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_publicmethods.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_staticmethoddeclaration.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_stringliteralinvoke.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_stringliterals.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1_weird_chars.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1w_Interface.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1w_RestrictedNative.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J1w_StaticField.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J2_staticFieldDecl.java");
  out->push_back("third_party/cs444/assignment_testcases/a1/J2_staticfielddeclaration.java");
}

} // namespace lexer
