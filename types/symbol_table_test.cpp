#include "types/symbol_table.h"

#include "lexer/lexer.h"
#include "third_party/gtest/gtest.h"

using base::ErrorList;
using base::PosRange;
using ast::TypeId;
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

  void MakeSymbolTable(const TypeIdList& paramTids, const vector<string>& paramNames, const vector<base::PosRange>& ranges) {
    // Create file set.
    base::FileSet* fs = nullptr;
    ASSERT_TRUE(base::FileSet::Builder().AddStringFile("Foo.java", "").Build(
        &fs, &errors_));
    fs_.reset(fs);
    symbs_.reset(new SymbolTable(fs_.get(), paramTids, paramNames, ranges));
  }

  base::ErrorList errors_;
  uptr<base::FileSet> fs_;
  uptr<SymbolTable> symbs_;
};

TEST_F(SymbolTableTest, ParamLookupWorks) {
  TypeId fooTid{100, 0};
  TypeId barTid{101, 2};
  MakeSymbolTable(
      {fooTid, barTid},
      {"foo", "bar"},
      {PosRange(0, 0, 1), PosRange(0, 1, 2)});

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
  MakeSymbolTable({}, {}, {});

  symbs_->DeclareLocal(tid1, "foo", PosRange(0, 0, 1), &errors_);
  symbs_->DeclareLocal(tid2, "bar", PosRange(0, 1, 2), &errors_);
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
  MakeSymbolTable({}, {}, {});
  auto result = symbs_->ResolveLocal("foo", PosRange(0, 5, 9), &errors_);
  EXPECT_ERRS("UndefinedReferenceError(0:5-9)\n");
  EXPECT_EQ(TypeId::kUnassigned, result.first);
  EXPECT_EQ(kVarUnassigned, result.second);
}

TEST_F(SymbolTableTest, SimpleScope) {
  TypeId tid{101, 2};
  MakeSymbolTable({}, {}, {});

  string name = "foo";
  symbs_->ResolveLocal(name, PosRange(0, 0, 1), &errors_);
  EXPECT_ERRS("UndefinedReferenceError(0:0)\n");
  errors_.Clear();

  symbs_->EnterScope();
  symbs_->DeclareLocal(tid, name, PosRange(0, 1, 2), &errors_);
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
  MakeSymbolTable({paramTid}, {"foo"}, {PosRange(0, 0, 1)});

  symbs_->EnterScope();
  symbs_->DeclareLocal(localTid, "foo", PosRange(0, 1, 2), &errors_);
  EXPECT_NO_ERRS();

  auto varInfo = symbs_->ResolveLocal("foo", PosRange(0, 3, 4), &errors_);
  EXPECT_NO_ERRS();
  EXPECT_EQ(localTid, varInfo.first);
  EXPECT_NE(kVarUnassigned, varInfo.second);

  symbs_->LeaveScope();
  varInfo = symbs_->ResolveLocal("foo", PosRange(0, 4, 5), &errors_);
  EXPECT_NO_ERRS();
  EXPECT_EQ(paramTid, varInfo.first);
  EXPECT_NE(kVarUnassigned, varInfo.second);
}

TEST_F(SymbolTableTest, LocalVarDuplicateDef) {
  TypeId tid{100, 0};
  MakeSymbolTable({}, {}, {});

  symbs_->DeclareLocal(tid, "foo", PosRange(0, 1, 2), &errors_);
  EXPECT_NO_ERRS();

  symbs_->EnterScope();
  symbs_->DeclareLocal(tid, "foo", PosRange(0, 2, 3), &errors_);
  EXPECT_ERRS("DuplicateVarDeclError(0:2,0:1)\n");
}

TEST_F(SymbolTableTest, LocalVarNonOverlappingScopes) {
  TypeId tid{100, 0};
  MakeSymbolTable({tid}, {"foo"}, {PosRange(0, 0, 1)});

  symbs_->EnterScope();
  symbs_->DeclareLocal(tid, "foo", PosRange(0, 1, 2), &errors_);
  EXPECT_NO_ERRS();
  symbs_->LeaveScope();

  symbs_->EnterScope();
  symbs_->DeclareLocal(tid, "foo", PosRange(0, 2, 3), &errors_);
  EXPECT_NO_ERRS();
  symbs_->LeaveScope();
}

}  // namespace types
