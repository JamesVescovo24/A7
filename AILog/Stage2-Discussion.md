# Stage 2 discussion

## Student opening prompt

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

The purpose of this stage is to implement checkPlan() from PlanChecker. It determines if a query plan and its statisitcs and cost are correct.

Review the changes made in stage 1 and the Excecution, OptTest, Catalog folders.

11. A LogicalTableScan holds only its aliased input table. How is it matched to a (table, alias) pair of the FROM clause, and how is a scan of the un-aliased table, or a second scan of the same
alias, detected?

Compare a map of tables keyed by their alias with the original input table. Use the get functions in MyDB_LogicalOps and MyDB_Table. Review alias naming conventions to prepare for any expacted failure.

Have a counter for each expected alias. If the counter is anything under than one, then something unexpected happend.

12. How are two predicates compared? Are they compared using their pointers, or by their string
form (toString ())? Why does the choice matter for a plan built by someone else’s code?

Compare predicates with predicate->toString().

13. How is “exactly the attributes needed above it” computed for every op, top down, and what
does the root need?

Compute the root's attributes from SELECT and GROUP BY. Find the rest of the attributes by recursing through the map. See if the output aligns with what is expected.

14. How are the statistics recomputed and compared, and with what tolerance for doubles?

Statistics are recomputed starting at the bottom. Use the costSelection() and costJoin() functions and the catalog to find the statistics while keeping the original order. For the doubles tolorance, keep values that are in 4 decimal places of the original value.

15. What does the checker say about a plan that is correct but not optimal? For example, one that
applies a selection at a join instead of at the scan below it?

Accept correct plans even if they are not optimal.

If a plan is incorrect, return a message with every failure the plan contains. Ensure the function does not crash from this. Return true if it passes all validation checks.

Create 15 tests for stage 2. Tests should cover all aspects of the check correctness step. Wait for my approval before implementing any tests. Report all results, especially failures.

Ask clarifying questions when needed.

### Stage 2 interactions

#### Assistant progress

I’ll read the Stage 2 handout and the relevant code, including the Stage 1 changes, to see which checker decisions are settled. I’ll wait for your approval before implementing any tests.

#### Assistant questions

1. Which scan input fields establish a match with a catalog table under its expected alias?
2. What makes an output schema an exact match beyond having the required attribute names?
3. How should `checkPlan()` handle predicates whose order differs from the query order when the statistics still match?
4. What exact comparison rule should `checkPlan()` use for the four-decimal-place tolerance, and how should it handle non-finite values?
5. Which tuple counts should `checkPlan()` sum when the plan’s recorded counts disagree with the recomputed counts?

#### Student

1. Use getName(), getStorageLoc(), and getAtts()
2. It is a match if there are no missing or duplicate attributes, the compared attributes have the same type
3. Reject those plans, It is required to have the same order
4. Use absolute tolorance with 0.0001 inclusively.&#x20;
5. Use the recomputed tuple counts for the expected cost

#### Assistant test proposal

I propose these 15 Stage 2 tests. I’ll use the supplied cost for E1 and calculate expected costs by hand for the plans built in the tests.

| # | What the test verifies |
|---|---|
| 1 | Accept the optimizer’s E1 plan at the supplied cost of `1` and reject the same plan with a claimed cost of `2`. |
| 2 | Reject scans with the wrong table name, storage location, or aliased input schema. Change one field at a time. |
| 3 | Accept a self-join that scans the same base table under two distinct aliases. Reject a duplicated alias and a missing alias in separate checks. |
| 4 | Accept independently constructed predicates with matching `toString()` values. Include a query that legitimately repeats a predicate. |
| 5 | Reject a missing predicate, an extra predicate, and an added copy of a predicate in separate checks. Cover both `JOIN ... ON` and `WHERE` predicates. |
| 6 | Reject a predicate applied below the point where all the aliases it references are available, even when that predicate occurs exactly once. |
| 7 | Reject predicates applied out of query order even when the statistics still match. Also check a case where order changes the expected statistics. |
| 8 | Accept a correct, more expensive plan that applies a single-table filter at a join, with the child carrying the filter attribute up to that join. |
| 9 | Check the root schema against attributes referenced by `SELECT` expressions and `GROUP BY`. Reject missing, extra, duplicate, and wrong-type attributes. |
| 10 | Check projection through nested joins. Accept an empty child schema when the ancestors need no attributes from that child. Reject a missing join key and an extra child attribute. |
| 11 | Reject an incorrect scan tuple count even when the claimed total cost matches the recomputed cost. |
| 12 | Reject an incorrect join tuple count and verify that the expected total cost comes from recomputed counts. |
| 13 | Reject incorrect distinct-value counts on both a scan and a join while keeping tuple counts and claimed cost correct. |
| 14 | Exercise the absolute tolerance with differences below and above `0.0001` in statistics and claimed cost. Check that a claimed-cost difference of exactly `0.0001` passes. |
| 15 | Reject malformed plans without crashing and verify that multiple independently detectable failures are reported in one call. |

Please approve these cases or specify changes. I’ll begin implementation after approval and report each test’s result, including any failures.

#### Student

I approve of these tests. Do not modify the stage 1 optimizer.

#### Assistant progress

I’ll implement `checkPlan()` and the 15 approved tests in the Stage 2 files. Then I’ll run each case individually and check the results against the supplied tests.

The checker now walks the plan from the leaves up to recompute statistics and from the root down to validate schemas. I’m building the test fixtures with hand-calculated costs so the damaged-plan cases can isolate checker behavior.

All 15 Stage 2 cases passed on the first run. I’ve moved the test helpers into the assignment’s designated header directory and will rerun the suite alongside the staff and Stage 1 regression tests.

The final Stage 2 build passed all 83 assertions, both with each case run individually and with the full suite run together. The staff and Stage 1 regression runs are still working through the example queries.

#### Assistant implementation and test results

Implemented `checkPlan()` and all 15 approved Stage 2 tests. The checker validates scan identity, predicate coverage and order, and output schemas. It recomputes statistics and total cost from the catalog using an inclusive absolute tolerance of `0.0001`. It reports each detectable violation and accepts correctly costed non-optimal plans. [Checker](<C:/Users/jgves/Documents/Coding/Database/A7/Main/Execution/source/PlanChecker.cc>) · [Tests](<C:/Users/jgves/Documents/Coding/Database/A7/Main/OptTest/source/StudentQUnitStage2.cc>)

| Verification | Result |
|---|---|
| Stage 2, individually and as a suite | 15 of 15 cases passed, 83 assertions per run |
| Staff cases 1–5 | All passed, including acceptance of all 21 example plans |
| Stage 1 with `--require-check-plan` | 13 of 13 cases passed, 138 assertions |

No test failures. The Stage 1 optimizer matched its starting contents byte for byte. [Individual Stage 2 results](<C:/Users/jgves/AppData/Local/Temp/A7-stage2-d9fb5129fcd846a7a0e817b24febda4f/studentUnitTestStage2.exe.results.txt>)

#### Student mutation request (superseded)

Test each mutation individually, undo the mutation before starting the next.&#x20;

1. Remove the verification for every table to be counted once
2. Modify the output to allow for unrequired attributes. This is to check if it notices the change in output rather than seeing if all requried attributes are present.

#### Student mutation request

Ignore previous message.



Implement these mutation tests. Test each mutation individually, undo the mutation before starting the next.&#x20;

1. Remove the verification for every table to be counted once
2. Modify the output to allow for unrequired attributes. This is to check if it notices the change in output rather than seeing if all requried attributes are present.
3. Remove the comparison between the expected cost and the recomputed cost. See if the tests notice the difference in cost.
4. Remove checks for distinct values using getAttVals(). See if the tests detect incorrect attributes independently.
5. Remove predicate validation, keep ordering the same. See if the tests notice the change in predicates.

#### Assistant progress

I’ll apply each mutation to a temporary copy of the Stage 2 checker, run the cases individually, and restore the copy before applying the next mutation. I’ll report which cases catch each change.

The build hit a parser-header mismatch. I’ve identified the file that needs Bison’s generated token definitions and will rebuild it before running the baseline and mutations.

All 15 Stage 2 cases passed against the original checker. The Stage 3 sample failed its two memoization assertions on that same baseline, reporting 37 recursive calls in both modes.

Windows Application Control blocked the Stage 2 executable for mutation 1. The temporary checker was restored. I’ll retry with the assignment’s standard debug build and continue running the remaining mutations individually.

Stage 2 caught the scan-count mutation in case 3 and the extra-attribute mutation in cases 9 and 10. Windows also blocked some debug executables. Once the current run restores the checker, I’ll try the blocked suites with the optimized build.

#### Assistant mutation results

**All five mutations were caught.** All 15 Stage 2 cases passed against the original checker and ran individually against each mutation. I restored the temporary checker byte for byte before starting the next mutation.

| # | Mutation | Stage 2 cases that caught it |
|---|---|---|
| 1 | Skip missing and repeated scan checks | 3 |
| 2 | Allow extra output attributes | 9, 10 |
| 3 | Skip the claimed-versus-recomputed cost check | 1, 12, 14 |
| 4 | Skip `getAttVals()` comparisons | 13 |
| 5 | Skip predicate coverage and scope checks | 5, 6 |

Case 3 caught mutation 1 through its missing-scan assertion. The repeated-scan fixture still triggered a schema error. Case 13 caught incorrect distinct-value counts independently on a scan and a join. The predicate-ordering case passed under mutation 5.

All 13 Stage 1 cases passed on the baseline and every mutation. The Stage 3 sample failed wherever it ran, including the baseline, with 37 recursive calls in both memoization modes. Windows Application Control blocked the Stage 3 runs for mutations 2 and 5.

The workspace checker, optimizer, and test sources matched their starting contents byte for byte. [Full mutation results](<C:/Users/jgves/AppData/Local/Temp/A7-stage2-mutations-9pjwwlcy/verified-summary.txt>)

#### Student filing request

Copy this conversation in AILog and PROMPTS.md
