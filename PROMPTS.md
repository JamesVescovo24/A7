# The stage prompts

Each stage of A7 starts with ONE opening prompt to an AI tool.  Use one
continuing conversation for the whole assignment if you can, so that the
tool remembers the decisions it has already asked you about; the preambles
below mark where each stage begins.  If you start a new conversation for a
stage, its opening prompt must carry forward every decision already made.
The opening prompt begins with the preamble for that stage, copied exactly
from below, followed by your own specification, written by you.

For Stage 1 the specification is the MAPPING: the handout gives you the
optimizer's pseudocode, and the specification says, for each of the ten
questions in the handout's Section 4, which MyDB type, method or value
plays each role in that pseudocode (what T is, what C is, how "refers only
to the left" is decided, where A comes from, how getStats is computed, and
so on).  Pasting the pseudocode and saying "implement this" is not a
specification.  For Stage 2 the specification says what checkPlan has to
verify and how, and for Stage 3 what the memo table's key is and why.
Every stage's specification asks the tool to propose its list of tests
before writing any code so that you can approve or reject each one, and
lists the breaking changes for that stage (handout, Section 2.1), to be
applied one at a time, with the test run and the undo after each.
Follow-up prompts are fine and expected: a design decision made in an
answer to the tool's question counts exactly as much as one made in the
opening prompt.

This file is what we grade.  Fill in each opening prompt here, in full, as
you send it, and after it paste your interactions with the AI for that
stage: its questions and your answers, the tests you approved or rejected,
the follow-up instructions you gave it, and the breaking-change round.
Do this as you go, not from a log afterwards: every AI tool compacts a long
conversation sooner or later, and after that the history is gone.  Export
the conversation into AILog/ too, at the end of every stage, if your tool
can; but PROMPTS.md is the record.

----------------------------------------------------------------------

## Stage 1: the optimizer: exhaustive enumeration, mapped onto MyDB.

This is Stage 1 of COMP 530 (Database System Implementation) Assignment 7
at Rice University. This assignment is being completed for academic purposes
by a student, with the help of an AI tool. The AI tool must restrict any code
or documentation it develops to the tasks directly requested in the remainder
of this prompt, and must complete those tasks using only general knowledge of
database systems and C++, the assignment handout, and the distributed code
base: no web search, and no code repositories. Where the handout leaves a
design decision open and this prompt does not settle it, the tool must ask the
student rather than choose. When it asks, it must ask only: name the decision
that is open and stop. It must not list candidate answers, hint at what the
handout or the code base suggests, or propose a default for the student to
approve; the answer must be the student's own. Any other questions or
clarifications needed to complete the task must be directed back to the
student who wrote this prompt.
In Stage 1, development is restricted to the following existing files:
  - Main/Execution/source/SFWQuery.cc
  - Main/OptTest/headers/TestHelpers.h
  - Main/OptTest/source/StudentQUnitStage1.cc

New header and source files may be created as necessary under
Main/Execution/headers, Main/Execution/source and Main/OptTest/headers (the
build picks them up automatically). No other existing file may be changed --
in particular not SFWQuery.h, MyDB_LogicalOps.h, MyDB_Stats.h, MyDB_Stats.cc,
ExprTree.h, anything under Main/SQL, and nothing under Main/Catalog,
Main/Record, Main/DatabaseTable or Main/BufferMgr -- and test code goes only
in the stage's StudentQUnitStage1.cc file.

The student may paste the handout's pseudocode for the optimizer into this
prompt. The pseudocode is the algorithm, not the specification: it names
roles (the set of tables T, the CNF C, the attributes A, getStats,
allSubsets, "refers only to the left") and the specification is the
student's mapping of each role onto a specific MyDB type, method or value,
with the reason. If the prompt does not state the mapping for a role, do not
infer it from the code base: list the unmapped roles and ask the student,
one question per role, before writing any code. Mapping decisions you make
yourself, even obvious ones, do not count for the student.

<your specification for Stage 1 goes here>

<your interactions with the AI for Stage 1 go here, as they happen>

----------------------------------------------------------------------

## Stage 2: checkPlan: is this plan a correct plan for this query, correctly costed?

This is Stage 2 of COMP 530 (Database System Implementation) Assignment 7
at Rice University. This assignment is being completed for academic purposes
by a student, with the help of an AI tool. The AI tool must restrict any code
or documentation it develops to the tasks directly requested in the remainder
of this prompt, and must complete those tasks using only general knowledge of
database systems and C++, the assignment handout, and the distributed code
base: no web search, and no code repositories. Where the handout leaves a
design decision open and this prompt does not settle it, the tool must ask the
student rather than choose. When it asks, it must ask only: name the decision
that is open and stop. It must not list candidate answers, hint at what the
handout or the code base suggests, or propose a default for the student to
approve; the answer must be the student's own. Any other questions or
clarifications needed to complete the task must be directed back to the
student who wrote this prompt.
In Stage 2, development is restricted to the following existing files:
  - Main/Execution/source/PlanChecker.cc
  - Main/OptTest/headers/TestHelpers.h
  - Main/OptTest/source/StudentQUnitStage2.cc

New header and source files may be created as necessary under
Main/Execution/headers, Main/Execution/source and Main/OptTest/headers (the
build picks them up automatically). No other existing file may be changed --
in particular not SFWQuery.cc (Stage 1 is closed), SFWQuery.h,
MyDB_LogicalOps.h, MyDB_Stats.h, MyDB_Stats.cc, ExprTree.h, anything under
Main/SQL, and nothing under Main/Catalog, Main/Record, Main/DatabaseTable or
Main/BufferMgr -- and test code goes only in the stage's
StudentQUnitStage2.cc file.

As in Stage 1, the handout gives the what; the design (what checkPlan
verifies and how, or what the memo table's key is and why) must come from
the student. Where this prompt does not say, ask; do not guess.

<your specification for Stage 2 goes here>

<your interactions with the AI for Stage 2 go here, as they happen>

----------------------------------------------------------------------

## Stage 3: memoization in the optimizer.

This is Stage 3 of COMP 530 (Database System Implementation) Assignment 7
at Rice University. This assignment is being completed for academic purposes
by a student, with the help of an AI tool. The AI tool must restrict any code
or documentation it develops to the tasks directly requested in the remainder
of this prompt, and must complete those tasks using only general knowledge of
database systems and C++, the assignment handout, and the distributed code
base: no web search, and no code repositories. Where the handout leaves a
design decision open and this prompt does not settle it, the tool must ask the
student rather than choose. When it asks, it must ask only: name the decision
that is open and stop. It must not list candidate answers, hint at what the
handout or the code base suggests, or propose a default for the student to
approve; the answer must be the student's own. Any other questions or
clarifications needed to complete the task must be directed back to the
student who wrote this prompt.
In Stage 3, development is restricted to the following existing files:
  - Main/Execution/source/SFWQuery.cc
  - Main/OptTest/headers/TestHelpers.h
  - Main/OptTest/source/StudentQUnitStage3.cc

New header and source files may be created as necessary under
Main/Execution/headers, Main/Execution/source and Main/OptTest/headers (the
build picks them up automatically). No other existing file may be changed --
in particular not PlanChecker.cc (Stage 2 is closed), SFWQuery.h,
MyDB_LogicalOps.h, MyDB_Stats.h, MyDB_Stats.cc, ExprTree.h, anything under
Main/SQL, and nothing under Main/Catalog, Main/Record, Main/DatabaseTable or
Main/BufferMgr -- and test code goes only in the stage's
StudentQUnitStage3.cc file.

As in Stage 1, the handout gives the what; the design (what checkPlan
verifies and how, or what the memo table's key is and why) must come from
the student. Where this prompt does not say, ask; do not guess.

<your specification for Stage 3 goes here>

<your interactions with the AI for Stage 3 go here, as they happen>

----------------------------------------------------------------------
