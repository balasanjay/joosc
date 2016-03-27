#include "base/error.h"

#include "std.h"

using std::ostream;

namespace base {
namespace {

inline string IfColor(const OutputOptions& opt, string color) {
  if (!opt.colorize) {
    return "";
  }
  return "\033[" + color;
}

}  // namespace

const OutputOptions OutputOptions::kSimpleOutput(false, true);
const OutputOptions OutputOptions::kUserOutput(true, false);

string OutputOptions::Red() const { return IfColor(*this, "31m"); }
string OutputOptions::Magenta() const { return IfColor(*this, "35m"); }
string OutputOptions::DarkGray() const { return IfColor(*this, "30m"); }
string OutputOptions::Green() const { return IfColor(*this, "32m"); }
string OutputOptions::ResetColor() const { return IfColor(*this, "39m"); }
string OutputOptions::BoldOn() const { return IfColor(*this, "1m"); }
string OutputOptions::BoldOff() const { return IfColor(*this, "22m"); }

std::ostream& operator<<(std::ostream& out, const Error& e) {
  e.PrintTo(&out, OutputOptions::kSimpleOutput, nullptr);
  return out;
}
Error* MakeSimplePosRangeError(PosRange pos, string name, string msg) {
  return MakeError([=](std::ostream* out, const OutputOptions& opt, const FileSet* fs) {
    if (opt.simple) {
      *out << name << '(' << pos << ')';
      return;
    }

    PrintDiagnosticHeader(out, opt, fs, pos, DiagnosticClass::ERROR, msg);
    PrintRangePtr(out, opt, fs, pos);
  });
}

Error* MakeError(PrintFn printfn) {
  class Err : public Error {
   public:
    Err(PrintFn printfn) : printfn_(printfn) {}

    void PrintTo(std::ostream* out, const OutputOptions& opt, const FileSet* fs) const override {
      printfn_(out, opt, fs);
    }

   private:
    PrintFn printfn_;
  };
  return new Err(printfn);
}

void PrintDiagnosticHeader(ostream* out, const OutputOptions& opt,
                           const FileSet* fs, PosRange pos, DiagnosticClass cls,
                           string msg) {
  const File* file = fs->Get(pos.fileid);

  // Get line and column info.
  int line = -1;
  int col = -1;
  file->IndexToLineCol(pos.begin, &line, &col);

  *out << opt.BoldOn();

  if (file->Dirname() != "") {
    *out << file->Dirname() << '/';
  }
  *out << file->Basename() << ":" << line + 1 << ":" << col + 1 << ": ";

  // Write the correct color.
  switch (cls) {
    case DiagnosticClass::ERROR:
      *out << opt.Red() << "error";
      break;
    case DiagnosticClass::WARNING:
      *out << opt.Magenta() << "warning";
      break;
    case DiagnosticClass::INFO:
      *out << opt.DarkGray() << "info";
      break;
  }

  *out << ": " << opt.ResetColor() << msg << opt.BoldOff() << '\n';
}

void PrintRangePtr(std::ostream* out, const OutputOptions& opt,
                   const FileSet* fs, const PosRange& pos) {
  const int MAX_CONTEXT = 40;
  const File* file = fs->Get(pos.fileid);

  // TODO(sjy): this will be slightly-broken for PosRanges that start on a
  // newline character. Figure out what to do here.

  // Walk backwards until we find a newline, so we know where to start printing.
  int begin = pos.begin;
  while (begin > 0 && pos.begin - begin < MAX_CONTEXT) {
    u8 c = file->At(begin - 1);
    if (c == '\n' || c == '\r') {
      break;
    }
    --begin;
  }

  // TODO(sjy): handle large ranges, by printing both the begining and the end
  // of the range with a "…" character in the middle.

  // Walk forwards until we find a newline, so we know where to start printing.
  // Note that we purposely start this at pos.begin, not pos.end. If the error
  // comprises an extremely large portion of text, we prefer to not spit it all
  // out here.
  int end = pos.begin;
  while (end < file->Size() && end - pos.begin < MAX_CONTEXT) {
    u8 c = file->At(end);
    if (c == '\n' || c == '\r') {
      break;
    }
    ++end;
  }

  // Now that we have both begin and end, we print the user's code.
  for (int i = begin; i < end; ++i) {
    u8 c = file->At(i);
    *out << c;
  }
  *out << '\n';

  // Finally, we print our pointer characters.
  *out << opt.Green();
  for (int i = begin; i < end; ++i) {
    if (i == pos.begin) {
      *out << '^';
    } else if (pos.begin < i && i < pos.end) {
      *out << '~';
    } else if (file->At(i) == '\t') {
      *out << '\t';
    } else {
      *out << ' ';
    }
  }
  *out << opt.ResetColor();
}

}  // namespace base
