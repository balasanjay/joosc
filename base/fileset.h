#ifndef BASE_FILESET_H
#define BASE_FILESET_H

#include "base/file.h"

namespace base {

class ErrorList;

class FileSet final {
 public:
  class Builder final {
   public:
    Builder& AddDiskFile(string path) {
      diskfiles_.push_back(path);
      return *this;
    }
    Builder& AddStringFile(string path, string contents) {
      stringfiles_.push_back(make_pair(path, contents));
      return *this;
    }

    bool Build(FileSet** fs, ErrorList* errors) const;

   private:
    vector<string> diskfiles_;
    vector<pair<string, string>> stringfiles_;
  };

  ~FileSet() {
    for (auto file : files_) {
      delete file;
    }
  }

  int Size() const { return files_.size(); }
  File* Get(int i) const {
    return files_.at(i);  // Crash on out-of-bounds.
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(FileSet);

  friend FileSet::Builder;

  FileSet(vector<File*> files) : files_(files) {}

  const vector<File*> files_;
};

}  // namespace base

#endif
