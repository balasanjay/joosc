#!/usr/bin/env bash

set -e
javac `find runtime -name "*.java"`
java -classpath "runtime" joostests.InheritanceCheckerTest

