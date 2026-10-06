#!/bin/bash
# test_batch.sh — automated batch mode tests for mysh

MYSH="${1:-./mysh}"
PASS=0
FAIL=0
TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

run() {
  # run <test_name> <script_text> <expected_stdout> [expected_exit]
  local name="$1" script="$2" expected="$3" want_exit="${4:-0}"
  local script_file="$TMPDIR/script_$$"
  printf '%s' "$script" > "$script_file"
  actual=$("$MYSH" "$script_file" 2>/dev/null)
  actual_exit=$?
  if [ "$actual" = "$expected" ] && [ "$actual_exit" -eq "$want_exit" ]; then
    echo "  PASS  $name"
    PASS=$((PASS + 1))
  else
    echo "  FAIL  $name"
    if [ "$actual" != "$expected" ]; then
      echo "        expected stdout: $(printf '%q' "$expected")"
      echo "          actual stdout: $(printf '%q' "$actual")"
    fi
    if [ "$actual_exit" -ne "$want_exit" ]; then
      echo "        expected exit: $want_exit  actual exit: $actual_exit"
    fi
    FAIL=$((FAIL + 1))
  fi
}

run_stderr() {
  # run_stderr <test_name> <script_text> <expected_stderr_substring>
  local name="$1" script="$2" needle="$3"
  local script_file="$TMPDIR/script_$$"
  printf '%s' "$script" > "$script_file"
  actual_err=$("$MYSH" "$script_file" 2>&1 >/dev/null)
  if echo "$actual_err" | grep -qF "$needle"; then
    echo "  PASS  $name"
    PASS=$((PASS + 1))
  else
    echo "  FAIL  $name"
    echo "        expected stderr to contain: $needle"
    echo "        actual stderr: $actual_err"
    FAIL=$((FAIL + 1))
  fi
}


section() { echo; echo "── $1"; }

# verify binary exists

if [ ! -x "$MYSH" ]; then
  echo "ERROR: cannot execute '$MYSH'" >&2
  exit 1
fi

echo "Testing: $MYSH"

section "Batch mode meta"

# No welcome/goodbye messages when reading a script file
run "no welcome message in batch mode" \
  "echo hello" \
  "hello"

# No prompt characters in output
run "no prompt in batch mode" \
  "pwd" \
  "$(pwd)"

run_stderr "catches syntax errors" \
  "echo error >>" \
  "Syntax error"

run_stderr "catches syntax errors" \
  "echo error | | | " \
  "Syntax error"

# mysh exits 0 normally
run "exits EXIT_SUCCESS" \
  "echo ok" \
  "ok" 0

section "Built-ins"

run "pwd prints working directory" \
  "pwd" \
  "$(pwd)"

run "cd then pwd" \
  "cd /tmp
pwd" \
  "/tmp"

run "cd with no args goes to HOME" \
  "cd /tmp
cd
pwd" \
  "$HOME"

run "which finds echo" \
  "which echo" \
  "$(which echo)"

run "exit stops execution" \
  "exit
echo should_not_print" \
  ""

run "exit is EXIT_SUCCESS" \
  "exit" \
  "" 0

section "Built-in errors (stderr)"

run_stderr "cd into nonexistent dir prints error" \
  "cd /nonexistent_dir_xyz" \
  "cd:"

run_stderr "cd too many args prints error" \
  "cd /tmp /usr" \
  "Too many arguments"

run_stderr "which on unknown command fails" \
  "which no_such_command_xyz" \
  "no_such_command_xyz"

run_stderr "which wrong arg count fails" \
  "which" \
  "Incorrect number"

section "External commands"

run "echo via external command" \
  "echo hello world" \
  "hello world"

run "absolute path execution" \
  "/bin/echo absolute" \
  "absolute"

run "multiple commands execute in order" \
  "echo first
echo second
echo third" \
  "first
second
third"

run "continue after failed command" \
  "no_such_command_xyz
echo after_error" \
  "after_error"

section "Comments and empty lines"

run "comment-only line is skipped" \
  "# this is a comment
echo after" \
  "after"

run "inline comment is stripped" \
  "echo hello # not printed" \
  "hello"

run "empty lines are skipped" \
  "echo one

echo two" \
  "one
two"

section "Redirection"

INFILE="$TMPDIR/input.txt"
OUTFILE="$TMPDIR/output.txt"
echo "hello from file" > "$INFILE"

run "output redirect creates file" \
  "echo redirected > $OUTFILE
cat $OUTFILE" \
  "redirected"

run "input redirect reads file" \
  "cat < $INFILE" \
  "hello from file"

run "output redirect truncates existing file" \
  "echo first > $OUTFILE
echo second > $OUTFILE
cat $OUTFILE" \
  "second"

run "both redirects in one command" \
  "cat < $INFILE > $OUTFILE
cat $OUTFILE" \
  "hello from file"

run_stderr "redirect to unwritable path prints error" \
  "echo x > /nonexistent_dir_xyz/out.txt" \
  "nonexistent_dir_xyz"

section "Pipes"

run "two-stage pipe" \
  "echo hello world | tr a-z A-Z | cat" \
  "HELLO WORLD"

run "pipe into grep" \
  "printf 'foo\nbar\nbaz\n' | grep ba" \
  "bar
baz"

run "cat sort uniq pipeline" \
  "cat test/pipeline_test.txt | sort | uniq" \
  "$(cat test/pipeline_test.txt | sort | uniq)"

section "Wildcards"

WDIR="$TMPDIR/wtest"
mkdir "$WDIR"
touch "$WDIR/foo.txt" "$WDIR/bar.txt" "$WDIR/baz.txt" "$WDIR/.hidden.txt"

run "wildcard expands matching files" \
  "cd $WDIR
echo *.txt" \
  "bar.txt baz.txt foo.txt"

run "wildcard does not match hidden files" \
  "cd $WDIR
echo *.txt" \
  "bar.txt baz.txt foo.txt"

run "wildcard with no match passes token through" \
  "cd $WDIR
echo *.xyz" \
  "*.xyz"

run "prefix wildcard" \
  "cd $WDIR
echo foo*" \
  "foo.txt"

run "suffix wildcard" \
  "cd $WDIR
ls *.txt | grep bar" \
  "bar.txt"

section "Pipe via stdin (no filename argument)"

pipe_run() {
  local name="$1" script="$2" expected="$3"
  actual=$(printf '%s' "$script" | "$MYSH" 2>/dev/null)
  if [ "$actual" = "$expected" ]; then
    echo "  PASS  $name"
    PASS=$((PASS + 1))
  else
    echo "  FAIL  $name"
    echo "        expected: $(printf '%q' "$expected")"
    echo "          actual: $(printf '%q' "$actual")"
    FAIL=$((FAIL + 1))
  fi
}

pipe_run "stdin batch: echo" \
  "echo from_stdin" \
  "from_stdin"

pipe_run "stdin batch: no welcome message" \
  "echo hi" \
  "hi"

pipe_run "stdin batch: exit stops execution" \
  "exit
echo nope" \
  ""

section "mysh argument errors"

actual_exit=$("$MYSH" /nonexistent_file_xyz 2>/dev/null; echo $?)
if [ "$actual_exit" -eq 1 ]; then
  echo "  PASS  nonexistent file arg exits nonzero"
  PASS=$((PASS + 1))
else
  echo "  FAIL  nonexistent file arg exits nonzero (got $actual_exit)"
  FAIL=$((FAIL + 1))
fi

actual_exit=$("$MYSH" a b c 2>/dev/null; echo $?)
if [ "$actual_exit" -eq 0 ]; then
  echo "  PASS  too many args exits EXIT_SUCCESS"
  PASS=$((PASS + 1))
else
  echo "  FAIL  too many args exits EXIT_SUCCESS (got $actual_exit)"
  FAIL=$((FAIL + 1))
fi


echo
TOTAL=$((PASS + FAIL))
echo "Results: $PASS/$TOTAL passed"
[ $FAIL -eq 0 ] && exit 0 || exit 1
