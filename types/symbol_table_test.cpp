#include "types/symbol_table.h"

#include "lexer/lexer.h"
#include "third_party/gtest/gtest.h"

using base::ErrorList;
using base::PosRange;
using ast::TypeId;
using ast::kVarUnassigned;
using lexer::Token;

#define EXPECT_ERRS(msg) EXPECT_EQ(msg, testing::PrintToString(errors_))
#define EXPECT_NO_ERRS() EXPECT_EQ(0, errors_.Size())

namespace types {

class SymbolTableTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    fs_.reset();
    errors_.Clear();
  }

  virtual void TearDown() {
    fs_.reset();
    errors_.Clear();
  }

  void MakeSymbolTable(const vector<VariableInfo>& paramInfos) {
    // Create file set.
    base::FileSet* fs = nullptr;
    ASSERT_TRUE(base::FileSet::Builder().AddStringFile("Foo.java", "").Build(
        &fs, &errors_));
    fs_.reset(fs);
    symbs_.reset(new SymbolTable(fs_.get(), paramInfos, &errors_));
  }

  base::ErrorList errors_;
  uptr<base::FileSet> fs_;
  uptr<SymbolTable> symbs_;
};

TEST_F(SymbolTableTest, ParamLookupWorks) {
  TypeId fooTid{100, 0};
  TypeId barTid{101, 2};
  MakeSymbolTable({
      {fooTid, "foo", PosRange(0, 0, 1)},
      {barTid, "bar", PosRange(0, 1, 2)}});

  auto varInfo = symbs_->ResolveLocal("foo", PosRange(0, 3, 4), &errors_);
  EXPECT_NO_ERRS();
  EXPECT_EQ(fooTid, varInfo.first);
  EXPECT_NE(kVarUnassigned, varInfo.second);

  varInfo = symbs_->ResolveLocal("bar", PosRange(0, 5, 6), &errors_);
  EXPECT_NO_ERRS();
  EXPECT_EQ(barTid, varInfo.first);
  EXPECT_NE(kVarUnassigned, varInfo.second);
}

TEST_F(SymbolTableTest, LocalVarWorks) {
  TypeId tid1{100, 0};
  TypeId tid2{101, 2};
  MakeSymbolTable({});

  VarDeclGuard(symbs_.get(), tid1, "foo", PosRange(0, 0, 1), &errors_);
  VarDeclGuard(symbs_.get(), tid2, "bar", PosRange(0, 1, 2), &errors_);
  EXPECT_NO_ERRS();

  auto varInfo = symbs_->ResolveLocal("foo", PosRange(0, 3, 4), &errors_);
  EXPECT_EQ(tid1, varInfo.first);
  EXPECT_NE(kVarUnassigned, varInfo.second);
  EXPECT_NO_ERRS();

  varInfo = symbs_->ResolveLocal("bar", PosRange(0, 5, 6), &errors_);
  EXPECT_NO_ERRS();
  EXPECT_EQ(tid2, varInfo.first);
  EXPECT_NE(kVarUnassigned, varInfo.second);
}

TEST_F(SymbolTableTest, UndefinedError) {
  MakeSymbolTable({});
  auto result = symbs_->ResolveLocal("foo", PosRange(0, 5, 9), &errors_);
  EXPECT_ERRS("UndefinedReferenceError(0:5-9)\n");
  EXPECT_EQ(TypeId::kUnassigned, result.first);
  EXPECT_EQ(kVarUnassigned, result.second);
}

TEST_F(SymbolTableTest, SimpleScope) {
  TypeId tid{101, 2};
  MakeSymbolTable({});

  string name = "foo";
  symbs_->ResolveLocal(name, PosRange(0, 0, 1), &errors_);
  EXPECT_ERRS("UndefinedReferenceError(0:0)\n");
  errors_.Clear();

  symbs_->EnterScope();
  VarDeclGuard(symbs_.get(), tid, name, PosRange(0, 1, 2), &errors_);
  auto result = symbs_->ResolveLocal(name, PosRange(0, 0, 1), &errors_);
  EXPECT_NO_ERRS();
  EXPECT_EQ(tid, result.first);
  EXPECT_NE(kVarUnassigned, result.second);

  symbs_->LeaveScope();
  symbs_->ResolveLocal(name, PosRange(0, 2, 3), &errors_);
  EXPECT_ERRS("UndefinedReferenceError(0:2)\n");
}

TEST_F(SymbolTableTest, LocalVarShadowsParam) {
  TypeId paramTid{100, 0};
  TypeId localTid{101, 2};
  MakeSymbolTable({{paramTid, "foo", PosRange(0, 0, 5)}});

  symbs_->EnterScope();
  VarDeclGuard(symbs_.get(), localTid, "foo", PosRange(0, 5, 10), &errors_);
  EXPECT_ERRS("foo: [0:5-10,0:0-5,]\n");

  // We expect a lookup to NOT emit a new error because the previous error
  // would cause this name to be blacklisted.
  auto varInfo = symbs_->ResolveLocal("foo", PosRange(0, 10, 15), &errors_);
  EXPECT_ERRS("foo: [0:5-10,0:0-5,]\n");
  EXPECT_EQ(paramTid, varInfo.first);
  EXPECT_NE(kVarUnassigned, varInfo.second);
}

TEST_F(SymbolTableTest, LocalVarDuplicateDef) {
  TypeId tid{100, 0};
  MakeSymbolTable({});

  VarDeclGuard(symbs_.get(), tid, "foo", PosRange(0, 1, 2), &errors_);
  EXPECT_NO_ERRS();

  symbs_->EnterScope();
  VarDeclGuard(symbs_.get(), tid, "foo", PosRange(0, 2, 3), &errors_);
  EXPECT_ERRS("foo: [0:2,0:1,]\n");
}

TEST_F(SymbolTableTest, LocalVarNonOverlappingScopes) {
  TypeId tid{100, 0};
  MakeSymbolTable({});

  symbs_->EnterScope();
  VarDeclGuard(symbs_.get(), tid, "foo", PosRange(0, 1, 2), &errors_);
  EXPECT_NO_ERRS();
  symbs_->LeaveScope();

  symbs_->EnterScope();
  VarDeclGuard(symbs_.get(), tid, "foo", PosRange(0, 2, 3), &errors_);
  EXPECT_NO_ERRS();
  symbs_->LeaveScope();
}

TEST_F(SymbolTableTest, InitializerReferencingOtherVar) {
  TypeId tid{100, 0};
  MakeSymbolTable({{tid, "foo", PosRange(0, 0, 1)}});

  symbs_->DeclareLocalStart(tid, "bar", PosRange(0, 1, 2), &errors_);
  EXPECT_NO_ERRS();
  symbs_->ResolveLocal("foo", PosRange(0, 4, 5), &errors_);
  EXPECT_NO_ERRS();
  symbs_->DeclareLocalEnd();
}

TEST_F(SymbolTableTest, InitializerReferencingOwnVar) {
  TypeId tid{100, 0};
  MakeSymbolTable({});

  symbs_->DeclareLocalStart(tid, "foo", PosRange(0, 1, 2), &errors_);
  EXPECT_NO_ERRS();
  symbs_->ResolveLocal("foo", PosRange(0, 4, 5), &errors_);
  EXPECT_ERRS("VariableInitializerSelfReferenceError(0:4)\n");
  symbs_->DeclareLocalEnd();
}

}  // namespace types
