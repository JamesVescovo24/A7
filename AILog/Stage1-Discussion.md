# Stage 1 discussion

## Student opening prompt

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

Optimize (CNF C, set of tables T, set of atts A, stats S)
returns (RAexp, stats for that expression, cost of that expression)
bestExp = empty; bestStats = empty; bestCost = huge
if (|T| = 1)
// one table: scan it, apply C, project onto A
let R be the one table in T
bestExp = project_A (select_C (R))
bestStats = getStats (bestExp, S) // T and V for bestExp, from S
bestCost = bestStats.T
else
// more than one table: try every way to break T into a left and a right
for each T’ in allSubsets (T), T’ not empty and not all of T:
left = T’, right = T - T’
leftCNF = the disjunctions in C that refer only to tables in left
rightCNF = the disjunctions in C that refer only to tables in right
topCNF = all of the other disjunctions in C
leftAtts = atts (left) intersect (A union atts (topCNF))
rightAtts = atts (right) intersect (A union atts (topCNF))
(leftExp, leftStats, leftCost) = Optimize (leftCNF, left, leftAtts, S)
(rightExp, rightStats, rightCost) = Optimize (rightCNF, right, rightAtts, S)
myExp = project_A (leftExp join_topCNF rightExp)
myStats = getStats (myExp, leftStats union rightStats)
if (myStats.T + leftCost + rightCost < bestCost)
bestExp = myExp; bestStats = myStats
bestCost = myStats.T + leftCost + rightCost
return (bestExp, bestStats, bestCost)

The purpose of this stage is to implement an exhaustive enumeration algorithm in order tp find the query with the lowest cost by recursively splitting the input tables. Each subset of tables would be optimized and calculate the cost of joining them together. Build the optimizeQueryPlan() in SFWQuery, look at any files it references before coding.

1. The tables are stored as vector<pair<string, string>> tablesToProccess, the first string is the table name and the second is the table's alias. The tables are keyed to their orignal name inside of allTables. Create a new map keying the tables by their aliases. The aliases are used to distinguish queries to the same tables.

2. CNF is held by vector<ExprTreePtr>, each element is a disjunction and theyare all joined together. The top-level CNF comes from vector<ExprTreePtr> predicates = getAllPredicates(). For each recursive split, create a leftCNF, rightCNF, and topCNF. Iterate through the current CNF in order to add each disjunction to the correct CNF. Order matters when it is called in MyDB_Stats.

3. Iterate through the alias in the left and right tables for every disjunction. Use the alias to see if it references either side. Find the predicate inside the table references. If only reference left -> leftCNF. If only reference right -> rightCNF. if references both -> topCNF. If no table is specified send it to the smallest table alias stored.

4. The set of attributes is stored in MyDB_SchemaPtr totSchema. The attribute names are formatted as [alias]_[attributeName]. Top level A comes from valuesToSelect (SELECT) and groupingClauses (GROUP BY). Create a recursive function to find the attributes referenced, use the following calculations:

leftAtts = atts (left) intersect (A union atts (topCNF))
rightAtts = atts (right) intersect (A union atts (topCNF))

Use the alias and attribute order when creating schemas.

5. For scans construct MyDB_Stats from alias input table and call costSelection() with the current CNF. For joins, call costJoin() on the left child, passing topCNF and right child's statistics. costSelection() and costJoin() should be ordered by predicates. The output handles projection.

6. T represents the set of tables and T' is a subset of T.  For each T' that is not empty and doesn't span all of T, the left split equals T' and the right split equals T - T'. Create a new map <string, MyDB_TablePtr> for each side, with the predicates moved into leftCNF, rightCNF, and topCNF. Recursively call on optimizeQueryPlan().

7. Use MyDB_Table to create the output table specifications. Name the outputs based on the aliases, ensure there is no change for collisions.

LogicalTableScan: MyDB_TablePtr input, MyDB_TablePtr output, MyDB_StatsPtr stats, disjunctions
LogicalJoin: left, right, top, outputTable, joinStats
output tables are named based on the aliases of the joined tables.

8. optimizeQueryPlan returns pair<LogicalOpPtr, double>. The first is the cheapest plan, the second is the estimated cost. Calculate best cost by calculating every candidate cost and keeping the one with the lowest.

9. If no WHERE or ON, return an empty vector. The optimizer shouldn't skip a split because it's missing a predicate. If a table that appears in the FROM and WHERE clauses but not in SELECT
or GROUP BY have it's output schema be empty. If the same table under two aliases, so long as the alias' are different there should be no overlap.

10. Increment numCalls once in the beginning of every call to the recursive optimizeQueryPlan. Do not increment it in the top level optimizer or sepreatly during splits.

Do not implement the plan checker or memorization in stage 1. Before writing any code, review all relevant files for this prompt and the already exisitng tests such as in StudentQUnitStage1.cc, OptQUnit.cc, Queries.h, TestHelpers.h. Check the optimizer works by comparing it to Examples.sql. Tests should cover all aspects of the optimizer step. Wait for my approval before implementing any tests. Report all results, especially failures.

Ask clarifying questions when needed.

### Stage 1 interactions

#### Assistant progress

I’ll read the full prompt, handout, and existing optimizer tests before editing. I’ll wait for your approval before implementing any tests.

I’m using the PDF skill to read the local handout and check its pseudocode against the pasted prompt.

#### Assistant questions

1. Which `ExprTree` method should determine whether a disjunction references a given alias?
2. Which `ExprTree` methods should the recursive attribute collector use to identify attribute references and visit child expressions?
3. How should the optimizer enumerate the nonempty proper subsets of the alias map?
4. What argument order should the `LogicalJoin` constructor call use?
5. What naming and storage-location convention should the scan and join output tables use?
6. What should a scan project when its table has no attributes in `SELECT` or `GROUP BY`, but a join above the scan needs one of its attributes?
7. How should the optimizer handle distinct attribute references that produce the same `alias_attribute` name?

#### Student

1. predicate->referenceTable(alias)
2. isId(), getId(), getLHS(), getRHS(), getChild()
3. Store aliases in a vector based on the iteration order, use bitmask enumeration.
4. LogicanJoin: leftInput, rightInput, output, topCNF, stats
5. Create names based on the current table alias(es). For example, the function Scan A is given the output table name scan_a_1 and the storage location scan_a_1.bin. The 1 is because this is the first time referencing table A. A join between table A and table B would be formatted as join_a_1_b_1. There is no need to increment table A because the two operations differ by scan and join.
6. The scan shoul project the attributes in the operations above it, including the attributes in the join predicate.&#x20;
7. If it's a repeated reference have it only appear once in the output. If they're different attributes with the same name, detect the issue before the recursion and report it. Return a error statement and \<pair>(nullptr, -1)

#### Assistant question

How should the optimizer disambiguate output names and storage paths when the alias-based format gives two outputs the same name or conflicts with an existing table?

#### Student

In case quantifying the names fails, use length prefix endcoding to disambiguate names.

#### Assistant test proposal

I propose keeping the existing E2 test and adding these Stage 1 cases.

| Case | What it verifies |
|---|---|
| 2 | All 21 queries in `Examples.sql` produce the expected costs. Each `JOIN ... ON` example also matches its comma-join equivalent. |
| 3 | E2 keeps each referenced attribute once in schema order. A separate query keeps an attribute used only in `GROUP BY`. |
| 4 | M3 keeps its two-table disjunction at the join and preserves predicate order. M4 keeps the join key in the `nation` scan’s output and drops the scan’s filter-only column. |
| 5 | M1 uses empty predicate lists. A variant with `WHERE 1 < 2` sends the predicate only to the smallest alias and costs 55 under the supplied model. A filtered cross product also checks that a scan can produce an empty schema. |
| 6 | M5 scans the same base table under distinct aliases. Separate cases check that colliding attribute names are rejected before recursion and that colliding output names use length-prefixed encoding. |
| 7 | M8 produces the expected bushy plan. One-, two-, three-, and four-table queries make exactly 1, 5, 37, and 365 recursive calls, respectively. |
| 8 | `checkPlan()` accepts a plan with a known cost. This assertion is expected to fail during Stage 1 because the supplied checker is a Stage 2 stub. |

Do you approve this test list?

#### Student

I approve all of these tests, also add tests for order sensitive predicates, verify attribute count using getAttVals(), and if both the left and right have the same cost. Make sure the checkPlan() failure is differentiated from actual failures. Run each test individually.

#### Assistant progress

I’ll implement the optimizer and the approved tests, including predicate order, distinct-value counts, and equal-cost splits. I’ll run each case individually and report the Stage 2 checker result separately.

The local MSYS2 toolchain has Flex, Bison, and G++. I’ll use it to build both runners in a temporary directory so the generated files stay outside the assignment source tree.

The optimizer is in place. The temporary build initially picked up the wrong parser header, and correcting the include order produced the SQL runner. I’ll now compare its output with `Examples.sql` and run the student tests.

The individual projection, naming, predicate-order, distinct-value, and equal-cost cases passed. The three staff Stage 1 cases matched their expected costs and failed only on the Stage 2 checker assertion. Both runs of the full example set are still in progress.

The SQL driver finished all 21 examples. Each seven-table query made 1,149,877 recursive calls. The individual student case that checks all example costs at higher precision is still running.

#### Assistant implementation and test results

Implemented the Stage 1 exhaustive optimizer and added the approved tests. Equal-cost plans keep the first enumerated split. Repeated base tables are numbered in `FROM` order, and output names are checked for case-insensitive collisions. [Optimizer](<C:/Users/jgves/Documents/Coding/Database/A7/Main/Execution/source/SFWQuery.cc>) · [Stage 1 tests](<C:/Users/jgves/Documents/Coding/Database/A7/Main/OptTest/source/StudentQUnitStage1.cc>)

- Ran all **13 student cases individually** with MSYS2 GCC. All 137 optimizer assertions passed, including predicate order, `getAttVals()`, naming collisions, and equal-cost splits.
- Matched **all 21 costs** in `Examples.sql`. E7 and J7 each made **1,149,877** recursive calls.
- Student case 8 reported the Stage 2 `checkPlan()` stub as `XFAIL`. The strict checker run and staff cases 1–3 exited with a failure on that checker assertion. All corresponding optimizer cost assertions passed.

#### Student mutation request

Implement these 5 mutation tests that inserts a breaking change inside the optimizer. Apply them individually and record if the tests catch each mutation. Undo each mutation before doing the next.

1. Missing topCNF attributes when calculating join attributes
2. Return the incorrect subset for exhaustive enumeration
3. Add disjunctions to leftCNF or rightCNF instead of topCNF
4. Mess up the order of the predicate disjunctions
5. Do not include rightCost when calculating candidate cost

#### Assistant progress

I’ll apply one mutation at a time to a temporary copy of the optimizer, run every Stage 1 case individually, and restore the copy before the next run. I’ll report which cases catch each mutation.

All 13 baseline cases exited successfully. Case 4 caught the missing join attribute, and cases 7 and 13 caught the skipped subset through their recursive call counts. The remaining three mutations are running in sequence.

#### Assistant mutation results

**All five mutations were caught.** I ran all 13 Stage 1 cases individually against the baseline and each mutation. The baseline exited successfully in every case.

| # | Breaking change applied | Cases that caught it |
|---|---|---|
| 1 | Omitted `topCNF` attributes from child projections | 4 |
| 2 | Skipped the final proper subset at each recursive join | 7, 13 |
| 3 | Sent cross-table disjunctions to `leftCNF` | 2, 4, 8, 10, 12 |
| 4 | Reversed predicate order before recursion | 4, 11, 12 |
| 5 | Omitted the right child’s cost from candidate cost | 2, 5, 7, 8, 10, 12, 13 |

Case 8 caught mutations 3 and 5 through its optimizer cost assertion. The Stage 2 checker rejection was recorded separately as the expected `XFAIL`.

I restored the optimizer copy before each subsequent run and verified that the workspace source files matched their starting contents byte for byte. [Run summary](<C:/Users/jgves/AppData/Local/Temp/A7-stage1-mutations-iqk9zjrk/summary.txt>)

#### Student filing request

Add the mutation summary as a seperate file in AILog. Record this discussion inside the AILog and PROMPTS.md.

[Saved mutation summary](Stage1-Mutation-Summary.txt)
