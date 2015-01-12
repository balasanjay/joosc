#include "base/file.h"
#include "base/file_impl.h"
#include "base/fileset.h"
#include <algorithm>

using std::sort;

namespace base {

bool FileSet::Builder::Build(FileSet** fs, ErrorList* errors) const {
  // TODO: for assignment 2+, we should report *which* file fails to load,
  // rather than just returning false.

  // Take local snapshot.
  vector<string> diskspecs = diskfiles_;
  vector<pair<string, string>> stringspecs = stringfiles_;

  // Preallocate some space.
  vector<File*> files;
  files.reserve(stringspecs.size() + diskspecs.size());

  // Handle StringFiles first, because they can never fail.
  for (auto stringspec : stringspecs) {
    files.push_back(new StringFile(stringspec.first, stringspec.second));
  }

  // Handle the DiskFiles, being careful to not leak memory when failing to read
  // a file.
  for (auto path : diskspecs) {
    File* file = nullptr;
    if (DiskFile::LoadFile(path, &file, errors)) {
      files.push_back(file);
    }
  }

  // Cleanup, if we had a failure.
  if (errors->IsFatal()) {
    for (auto file : files) {
      delete file;
    }
    *fs = nullptr;
    return false;
  }

  // Sort entries, so that compilation order is deterministic.
  // TODO: replace this with a lambda if we can get C++11 on the school servers.
  struct {
    bool operator()(File* left, File* right) {
      if (left->Dirname() != right->Dirname()) {
        return left->Dirname() < right->Dirname();
      }
      return left->Basename() < right->Basename();
    }
  } cmp;
  sort(files.begin(), files.end(), cmp);

  // TODO: figure out what we should do with duplicate file names, invalid file
  // names, etc. It's possible that some of these will be caught by later
  // stages, but we're not sure if that's supposed to be what happens.

  // Finally, create and return fileset.
  *fs = new FileSet(files);
  return true;
}

}  // namespace base
